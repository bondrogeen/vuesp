import {
  BLOCK_ELSE,
  BLOCK_HANDLER,
  BLOCK_IF,
  BLOCK_TERNARY,
  BLOCK_WHILE,
  CONST_ARRAY,
  constantAsArray,
  constantAsString,
  readContainer,
  readF32,
  readU16,
  readU32,
} from './container.ts';
import type { BytecodeContainer } from './container.ts';
import { OPCODES } from './opcodes.ts';
import type { OpcodeName } from './opcodes.ts';
import { decodeVar } from './types.ts';

interface DecodedInstruction {
  readonly name: string;
  readonly info: (typeof OPCODES)[OpcodeName] | null;
  readonly operands: readonly string[];
  readonly size: number;
}

const CODE_TO_NAME = new Map<number, OpcodeName>();
for (const [name, info] of Object.entries(OPCODES) as Array<[OpcodeName, (typeof OPCODES)[OpcodeName]]>) {
  CODE_TO_NAME.set(info.code, name);
}

const KIND_NAMES = ['INT', 'UINT', 'FLOAT', 'STRING'];

function formatConstant(container: BytecodeContainer, index: number): string {
  const entry = container.constants[index];
  if (!entry) return `#${index}`;
  if (entry.tag === CONST_ARRAY) {
    const bytes = constantAsArray(container, index);
    return `#${index} [${Array.from(bytes).join(', ')}]`;
  }
  if (entry.tag === 4 || entry.tag === 5) {
    const text = constantAsString(container, index);
    return `#${index} '${text.replace(/'/g, "\\'")}'`;
  }
  return `#${index}`;
}

function decodeVarOperand(code: number): string {
  try {
    const { type, index } = decodeVar(code);
    return `$${type}${index}`;
  } catch {
    return `0x${code.toString(16)}`;
  }
}

function decodeInstruction(container: BytecodeContainer, offset: number): DecodedInstruction {
  const code = container.code;
  const opcode = code[offset];
  const name = CODE_TO_NAME.get(opcode);
  if (!name) {
    return { name: '.byte', info: null, operands: [`0x${opcode.toString(16).padStart(2, '0')}`], size: 1 };
  }
  const info = OPCODES[name];
  const operands: string[] = [];
  let cursor = offset + 1;
  for (const type of info.operands) {
    if (type === 'i8') {
      const value = (code[cursor] << 24) >> 24;
      operands.push(String(value));
      cursor += 1;
    } else if (type === 'u8' || type === 'argc8' || type === 'func8' || type === 'event8') {
      operands.push(String(code[cursor]));
      cursor += 1;
    } else if (type === 'kind8') {
      operands.push(KIND_NAMES[code[cursor]] ?? String(code[cursor]));
      cursor += 1;
    } else if (type === 'var8') {
      operands.push(decodeVarOperand(code[cursor]));
      cursor += 1;
    } else if (type === 'i16') {
      const value = (readU16(code, cursor) << 16) >> 16;
      operands.push(String(value));
      cursor += 2;
    } else if (type === 'u16') {
      operands.push(String(readU16(code, cursor)));
      cursor += 2;
    } else if (type === 'const16' || type === 'name16') {
      operands.push(formatConstant(container, readU16(code, cursor)));
      cursor += 2;
    } else if (type === 'i32') {
      operands.push(String(readU32(code, cursor) | 0));
      cursor += 4;
    } else if (type === 'u32') {
      operands.push(String(readU32(code, cursor)));
      cursor += 4;
    } else if (type === 'f32') {
      operands.push(String(readF32(code, cursor)));
      cursor += 4;
    } else {
      operands.push(`?${type}`);
      cursor += 1;
    }
  }
  return { name, info, operands, size: cursor - offset };
}

function blockName(kind: number): string {
  if (kind === BLOCK_IF) return 'IF';
  if (kind === BLOCK_ELSE) return 'ELSE';
  if (kind === BLOCK_WHILE) return 'WHILE';
  if (kind === BLOCK_HANDLER) return 'HANDLER';
  if (kind === BLOCK_TERNARY) return 'TERNARY';
  return `BLOCK_${kind}`;
}

export function disassemble(input: BytecodeContainer | Uint8Array): string {
  const container = input instanceof Uint8Array ? readContainer(input) : input;
  const lines: string[] = [];
  let offset = 0;
  while (offset < container.code.length) {
    const decoded = decodeInstruction(container, offset);
    const operands = decoded.operands.length > 0 ? ` ${decoded.operands.join(' ')}` : '';
    lines.push(`${offset.toString(16).padStart(4, '0')}  ${decoded.name}${operands}`);
    offset += decoded.size;
  }

  if (container.blocks.length > 0) {
    lines.push('');
    lines.push('; blocks');
    for (const block of container.blocks) {
      lines.push(
        `; ${blockName(block.kind)} start=${block.start.toString(16)} middle=${block.middle.toString(16)} end=${block.end.toString(16)}`,
      );
    }
  }

  if (container.handlers.length > 0) {
    lines.push('');
    lines.push('; handlers');
    for (const handler of container.handlers) {
      lines.push(
        `; event=${handler.eventId} name=${formatConstant(container, handler.nameConst)} entry=${handler.entry.toString(16)} end=${handler.end.toString(16)}`,
      );
    }
  }

  return `${lines.join('\n')}\n`;
}
