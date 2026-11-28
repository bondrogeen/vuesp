import {
  BLOCK_ELSE,
  BLOCK_IF,
  BLOCK_TERNARY,
  BLOCK_WHILE,
  CONST_ARRAY,
  ContainerError,
  constantAsString,
  readContainer,
  readF32,
  readU16,
  readU32,
} from './container.ts';
import type { BlockEntry, BytecodeContainer } from './container.ts';
import { ScriptError } from './lexer.ts';
import { Opcode, OPCODES } from './opcodes.ts';
import type { OpcodeName } from './opcodes.ts';
import { printProgram } from './printer.ts';
import type {
  BinaryOp,
  CallExpr,
  CapabilityManifest,
  CompileError,
  DataVarSpec,
  Expr,
  LValue,
  NumberExpr,
  Program,
  SourcePos,
  Stmt,
  StringExpr,
  VarSlot,
  OnParam,
} from './types.ts';
import { DEFAULT_MANIFEST, decodeVar, getEventArgNames } from './types.ts';

export interface DecompileOptions {
  readonly manifest?: CapabilityManifest;
}

export interface DecompileResult {
  readonly ok: boolean;
  readonly source?: string;
  readonly program?: Program;
  readonly errors: readonly CompileError[];
}

interface RawInstruction {
  readonly name: OpcodeName;
  readonly operands: readonly number[];
  readonly size: number;
}

interface SimpleStatement {
  readonly stmt: Stmt | null;
  readonly end: number;
}

interface LoopContext {
  readonly continueTarget: number;
  readonly breakTarget: number;
}

const CODE_TO_NAME = new Map<number, OpcodeName>();
for (const [name, info] of Object.entries(OPCODES) as Array<[OpcodeName, (typeof OPCODES)[OpcodeName]]>) {
  CODE_TO_NAME.set(info.code, name);
}

const BINARY_OPS: Partial<Record<OpcodeName, BinaryOp>> = {
  ADD: '+',
  SUB: '-',
  MUL: '*',
  DIV: '/',
  MOD: '%',
  BIT_AND: '&',
  BIT_OR: '|',
  BIT_XOR: '^',
  SHL: '<<',
  SHR: '>>',
  EQ: '==',
  NE: '!=',
  LT: '<',
  LE: '<=',
  GT: '>',
  GE: '>=',
  AND: '&&',
  OR: '||',
};

const UNARY_OPS: Partial<Record<OpcodeName, '-' | '!' | '~'>> = {
  NEG: '-',
  NOT: '!',
  BIT_NOT: '~',
};

function operandSize(name: string): number {
  if (
    name === 'i8' ||
    name === 'u8' ||
    name === 'var8' ||
    name === 'kind8' ||
    name === 'argc8' ||
    name === 'func8' ||
    name === 'event8'
  ) {
    return 1;
  }
  if (name === 'i16' || name === 'u16' || name === 'const16' || name === 'name16') return 2;
  if (name === 'i32' || name === 'u32' || name === 'f32') return 4;
  throw new ScriptError('E_OPERAND', `unknown operand type ${name}`, { line: 1, column: 1, offset: 0 });
}

class Decompiler {
  private readonly container: BytecodeContainer;
  private readonly manifest: CapabilityManifest;
  private readonly blocksByStart = new Map<number, BlockEntry[]>();
  private readonly sortedBlocks: BlockEntry[];
  private handlerArgNames: readonly string[] = [];

  constructor(container: BytecodeContainer, manifest: CapabilityManifest) {
    this.container = container;
    this.manifest = manifest;
    this.sortedBlocks = [...container.blocks].sort((a, b) => a.start - b.start || a.end - b.end);
    for (const block of this.sortedBlocks) {
      const list = this.blocksByStart.get(block.start) ?? [];
      list.push(block);
      this.blocksByStart.set(block.start, list);
    }
  }

  decompileProgram(): Program {
    const handlerEntries = this.container.handlers.map((handler) => handler.entry);
    const mainEnd = handlerEntries.length > 0 ? Math.min(...handlerEntries) : this.container.code.length;
    const main = this.decompileStatements(0, mainEnd);
    const handlers: Stmt[] = [];
    for (const handler of this.container.handlers) {
      const end = handler.end > handler.entry ? handler.end - 1 : handler.entry;
      const event = constantAsString(this.container, handler.nameConst);
      const argNames = getEventArgNames(event);
      this.handlerArgNames = argNames;
      const body = [...this.decompileStatements(handler.entry, end)];
      this.handlerArgNames = [];
      while (body.length > 0 && body[body.length - 1].kind === 'return') body.pop();
      const params: OnParam[] = argNames.map((name) => ({ name, pos: this.pos(handler.entry) }));
      handlers.push({
        kind: 'on',
        event,
        params,
        body,
        pos: this.pos(handler.entry),
      });
    }
    return { body: [...main, ...handlers] };
  }

  private decompileStatements(start: number, end: number, loop?: LoopContext): Stmt[] {
    const statements: Stmt[] = [];
    let cursor = start;
    while (cursor < end) {
      const opcode = this.container.code[cursor];
      if (opcode === Opcode.HALT) break;

      const ifBlock = this.blockAt(cursor, BLOCK_IF);
      if (ifBlock) {
        const cond = this.parseOne(ifBlock.start, ifBlock.middle);
        const elseBlock = this.elseFor(ifBlock, end);
        const thenEnd = ifBlock.end;
        const thenStmts = this.decompileStatements(ifBlock.middle + 3, thenEnd, loop);
        let elseStmts: Stmt[] | undefined;
        let next: number;
        if (elseBlock) {
          elseStmts = this.decompileStatements(elseBlock.start, elseBlock.end, loop);
          next = elseBlock.end;
        } else {
          next = ifBlock.end;
        }
        statements.push({
          kind: 'if',
          cond,
          then: thenStmts,
          else: elseStmts,
          pos: this.pos(cursor),
        });
        cursor = next;
        continue;
      }

      const whileBlock = this.blockAt(cursor, BLOCK_WHILE);
      if (whileBlock) {
        const cond = this.parseOne(whileBlock.start, whileBlock.middle);
        const bodyEnd = whileBlock.end - 3;
        if (bodyEnd < whileBlock.middle + 3 || this.container.code[bodyEnd] !== Opcode.JUMP) {
          throw new ScriptError('E_DECOMPILE', 'while back-jump is missing', this.pos(whileBlock.start));
        }
        const body = this.decompileStatements(whileBlock.middle + 3, bodyEnd, {
          continueTarget: whileBlock.start,
          breakTarget: whileBlock.end,
        });
        statements.push({
          kind: 'while',
          cond,
          body,
          pos: this.pos(cursor),
        });
        cursor = whileBlock.end;
        continue;
      }

      const simple = this.decompileSimple(cursor, end, loop);
      if (simple.stmt) statements.push(simple.stmt);
      if (simple.end <= cursor) {
        throw new ScriptError('E_DECOMPILE', 'decompiler did not advance', this.pos(cursor));
      }
      cursor = simple.end;
    }
    return statements;
  }

  private decompileSimple(cursor: number, end: number, loop?: LoopContext): SimpleStatement {
    let offset = cursor;
    while (offset < end) {
      const ternary = this.ternaryAt(offset);
      if (ternary) {
        const elseBlock = this.requireElse(ternary, end);
        offset = elseBlock.end;
        continue;
      }

      const decoded = this.decode(offset);
      const { name, operands, size } = decoded;

      switch (name) {
        case 'STORE_VAR': {
          const value = this.parseOne(cursor, offset);
          const target = this.varLValue(operands[0], offset);
          return {
            stmt: { kind: 'assign', target, value, pos: this.pos(cursor) },
            end: offset + size,
          };
        }
        case 'STORE_PORT': {
          const value = this.parseOne(cursor, offset);
          const target = this.portLValue(operands[0], offset);
          return {
            stmt: { kind: 'assign', target, value, pos: this.pos(cursor) },
            end: offset + size,
          };
        }
        case 'STORE_DATA': {
          const value = this.parseOne(cursor, offset);
          const nameConst = operands[0];
          const target: LValue = {
            kind: 'data',
            name: constantAsString(this.container, nameConst),
            pos: this.pos(cursor),
          };
          return {
            stmt: { kind: 'assign', target, value, pos: this.pos(cursor) },
            end: offset + size,
          };
        }
        case 'ARRAY_SET': {
          const stack = this.evalRange(cursor, offset);
          if (stack.length !== 3) {
            throw new ScriptError('E_DECOMPILE', 'ARRAY_SET needs 3 values', this.pos(offset));
          }
          const target: LValue = {
            kind: 'index',
            array: stack[0],
            index: stack[1],
            pos: this.pos(cursor),
          };
          return {
            stmt: { kind: 'assign', target, value: stack[2], pos: this.pos(cursor) },
            end: offset + size,
          };
        }
        case 'SET': {
          const stack = this.evalRange(cursor, offset);
          if (stack.length !== 3) {
            throw new ScriptError('E_DECOMPILE', 'SET needs 3 values', this.pos(offset));
          }
          return {
            stmt: {
              kind: 'expr',
              expr: this.call('set', stack, cursor),
              pos: this.pos(cursor),
            },
            end: offset + size,
          };
        }
        case 'LOG': {
          const stack = this.evalRange(cursor, offset);
          return {
            stmt: {
              kind: 'expr',
              expr: this.call('log', stack, cursor),
              pos: this.pos(cursor),
            },
            end: offset + size,
          };
        }
        case 'WAIT': {
          const ms = operands[0];
          return {
            stmt: {
              kind: 'expr',
              expr: this.call('wait', [this.number(ms, false, offset), this.string('ms', offset)], cursor),
              pos: this.pos(cursor),
            },
            end: offset + size,
          };
        }
        case 'CALL_SCRIPT': {
          const id = operands[0];
          const argc = operands[1];
          const stack = this.evalRange(cursor, offset);
          if (stack.length !== argc) {
            throw new ScriptError('E_DECOMPILE', 'CALL_SCRIPT argument count mismatch', this.pos(offset));
          }
          return {
            stmt: {
              kind: 'expr',
              expr: this.call('call', [this.number(id, false, offset), ...stack], cursor),
              pos: this.pos(cursor),
            },
            end: offset + size,
          };
        }
        case 'CALL_SCRIPT_NAME': {
          const name = constantAsString(this.container, operands[0]);
          const argc = operands[1];
          const stack = this.evalRange(cursor, offset);
          if (stack.length !== argc) {
            throw new ScriptError('E_DECOMPILE', 'CALL_SCRIPT_NAME argument count mismatch', this.pos(offset));
          }
          return {
            stmt: {
              kind: 'expr',
              expr: this.call('call', [this.string(name, offset), ...stack], cursor),
              pos: this.pos(cursor),
            },
            end: offset + size,
          };
        }
        case 'CALL_EXT': {
          const fn = this.functionById(operands[0], offset);
          if (fn.ret !== 'none') {
            offset += size;
            continue;
          }
          const stack = this.evalRange(cursor, offset);
          if (stack.length !== operands[1]) {
            throw new ScriptError('E_DECOMPILE', 'CALL_EXT argument count mismatch', this.pos(offset));
          }
          return {
            stmt: {
              kind: 'expr',
              expr: this.call(fn.name, stack, cursor),
              pos: this.pos(cursor),
            },
            end: offset + size,
          };
        }
        case 'POP': {
          const stack = this.evalRange(cursor, offset);
          if (stack.length === 0) {
            return { stmt: null, end: offset + size };
          }
          return {
            stmt: { kind: 'expr', expr: stack[stack.length - 1], pos: this.pos(cursor) },
            end: offset + size,
          };
        }
        case 'JUMP': {
          const target = operands[0];
          if (loop && target === loop.continueTarget) {
            return { stmt: { kind: 'continue', pos: this.pos(cursor) }, end: offset + size };
          }
          if (loop && target === loop.breakTarget) {
            return { stmt: { kind: 'break', pos: this.pos(cursor) }, end: offset + size };
          }
          throw new ScriptError(
            'E_DECOMPILE',
            `unexpected jump to ${target.toString(16)} outside a loop`,
            this.pos(cursor),
          );
        }
        case 'RETURN':
          return { stmt: { kind: 'return', pos: this.pos(cursor) }, end: offset + size };
        case 'HALT':
          return { stmt: null, end: offset + size };
        default:
          offset += size;
          break;
      }
    }
    throw new ScriptError('E_DECOMPILE', 'statement has no terminator', this.pos(cursor));
  }

  private evalRange(start: number, end: number, skipTernaryStart?: number): Expr[] {
    const stack: Expr[] = [];
    let cursor = start;
    while (cursor < end) {
      const ternary = cursor === skipTernaryStart ? undefined : this.ternaryAt(cursor);
      if (ternary) {
        const elseBlock = this.requireElse(ternary, end);
        const cond = this.parseOne(ternary.start, ternary.middle, ternary.start);
        const thenExpr = this.parseOne(ternary.middle + 3, ternary.end);
        const elseExpr = this.parseOne(elseBlock.start, elseBlock.end);
        stack.push({
          kind: 'ternary',
          cond,
          then: thenExpr,
          else: elseExpr,
          pos: this.pos(cursor),
        });
        cursor = elseBlock.end;
        continue;
      }

      const decoded = this.decode(cursor);
      const { name, operands, size } = decoded;

      switch (name) {
        case 'PUSH_I8':
        case 'PUSH_I16':
        case 'PUSH_I32':
        case 'PUSH_U8':
        case 'PUSH_U16':
        case 'PUSH_U32':
          stack.push(this.number(operands[0], false, cursor));
          break;
        case 'PUSH_F32':
          stack.push(this.number(operands[0], true, cursor));
          break;
        case 'PUSH_STR':
          stack.push(this.string(constantAsString(this.container, operands[0]), cursor));
          break;
        case 'PUSH_ARRAY': {
          const entry = this.container.constants[operands[0]];
          if (!entry || entry.tag !== CONST_ARRAY) {
            throw new ScriptError('E_DECOMPILE', 'PUSH_ARRAY expects an array constant', this.pos(cursor));
          }
          const bytes = entry.value as Uint8Array;
          stack.push({
            kind: 'array',
            items: Array.from(bytes, (value) => this.number(value, false, cursor)),
            pos: this.pos(cursor),
          });
          break;
        }
        case 'LOAD_VAR':
          stack.push(this.varExpr(operands[0], cursor));
          break;
        case 'LOAD_DATA':
          stack.push(this.dataVar(operands[0], operands[1], cursor));
          break;
        case 'LOAD_PORT':
          stack.push(this.portExpr(operands[0], cursor));
          break;
        case 'ADD':
        case 'SUB':
        case 'MUL':
        case 'DIV':
        case 'MOD':
        case 'BIT_AND':
        case 'BIT_OR':
        case 'BIT_XOR':
        case 'SHL':
        case 'SHR':
        case 'EQ':
        case 'NE':
        case 'LT':
        case 'LE':
        case 'GT':
        case 'GE':
        case 'AND':
        case 'OR': {
          const right = this.pop(stack, cursor);
          const left = this.pop(stack, cursor);
          stack.push({
            kind: 'binary',
            op: BINARY_OPS[name] as BinaryOp,
            left,
            right,
            pos: this.pos(cursor),
          });
          break;
        }
        case 'NEG':
        case 'NOT':
        case 'BIT_NOT': {
          const expr = this.pop(stack, cursor);
          stack.push({
            kind: 'unary',
            op: UNARY_OPS[name] as '-' | '!' | '~',
            expr,
            pos: this.pos(cursor),
          });
          break;
        }
        case 'ARRAY_GET': {
          const index = this.pop(stack, cursor);
          const array = this.pop(stack, cursor);
          stack.push({ kind: 'index', array, index, pos: this.pos(cursor) });
          break;
        }
        case 'GET': {
          const index = this.pop(stack, cursor);
          const array = this.pop(stack, cursor);
          stack.push(this.call('get', [array, index], cursor));
          break;
        }
        case 'LEN': {
          const arg = this.pop(stack, cursor);
          stack.push(this.call('len', [arg], cursor));
          break;
        }
        case 'CHR': {
          const arg = this.pop(stack, cursor);
          stack.push(this.call('chr', [arg], cursor));
          break;
        }
        case 'ORD': {
          const arg = this.pop(stack, cursor);
          stack.push(this.call('ord', [arg], cursor));
          break;
        }
        case 'CALL_EXT': {
          const fn = this.functionById(operands[0], cursor);
          const argc = operands[1];
          if (stack.length < argc) {
            throw new ScriptError('E_DECOMPILE', 'CALL_EXT stack underflow', this.pos(cursor));
          }
          if (fn.ret === 'none') {
            throw new ScriptError('E_DECOMPILE', 'void function used in an expression', this.pos(cursor));
          }
          const args = stack.splice(stack.length - argc, argc);
          stack.push(this.call(fn.name, args, cursor));
          break;
        }
        case 'DUP': {
          const top = stack[stack.length - 1];
          if (!top) throw new ScriptError('E_DECOMPILE', 'DUP on empty stack', this.pos(cursor));
          stack.push(top);
          break;
        }
        case 'SWAP': {
          const b = this.pop(stack, cursor);
          const a = this.pop(stack, cursor);
          stack.push(b);
          stack.push(a);
          break;
        }
        case 'POP':
          stack.pop();
          break;
        default:
          throw new ScriptError('E_DECOMPILE', `unsupported ${name} in expression`, this.pos(cursor));
      }
      cursor += size;
    }
    return stack;
  }

  private parseOne(start: number, end: number, skipTernaryStart?: number): Expr {
    const stack = this.evalRange(start, end, skipTernaryStart);
    if (stack.length !== 1) {
      throw new ScriptError(
        'E_DECOMPILE',
        `expression range ${start.toString(16)}..${end.toString(16)} leaves ${stack.length} values`,
        this.pos(start),
      );
    }
    return stack[0];
  }

  private pop(stack: Expr[], offset: number): Expr {
    const value = stack.pop();
    if (!value) throw new ScriptError('E_DECOMPILE', 'expression stack underflow', this.pos(offset));
    return value;
  }

  private decode(offset: number): RawInstruction {
    const opcode = this.container.code[offset];
    const name = CODE_TO_NAME.get(opcode);
    if (!name) {
      throw new ScriptError(
        'E_DECOMPILE',
        `unknown opcode 0x${opcode.toString(16)} at ${offset.toString(16)}`,
        this.pos(offset),
      );
    }
    const info = OPCODES[name];
    const operands: number[] = [];
    let cursor = offset + 1;
    for (const type of info.operands) {
      const size = operandSize(type);
      if (type === 'i8') operands.push((this.container.code[cursor] << 24) >> 24);
      else if (type === 'i16') operands.push((readU16(this.container.code, cursor) << 16) >> 16);
      else if (type === 'i32') operands.push(readU32(this.container.code, cursor) | 0);
      else if (type === 'f32') operands.push(readF32(this.container.code, cursor));
      else if (size === 1) operands.push(this.container.code[cursor]);
      else if (size === 2) operands.push(readU16(this.container.code, cursor));
      else operands.push(readU32(this.container.code, cursor));
      cursor += size;
    }
    return { name, operands, size: cursor - offset };
  }

  private blockAt(offset: number, kind: number): BlockEntry | undefined {
    return this.blocksByStart.get(offset)?.find((block) => block.kind === kind);
  }

  private ternaryAt(offset: number): BlockEntry | undefined {
    return this.blockAt(offset, BLOCK_TERNARY);
  }

  private elseFor(block: BlockEntry, end: number): BlockEntry | null {
    const target = readU16(this.container.code, block.middle + 1);
    return (
      this.sortedBlocks.find(
        (candidate) => candidate.kind === BLOCK_ELSE && candidate.start === target && candidate.end <= end,
      ) ?? null
    );
  }

  private requireElse(block: BlockEntry, end: number): BlockEntry {
    const elseBlock = this.elseFor(block, end);
    if (!elseBlock) {
      throw new ScriptError('E_DECOMPILE', 'ternary else block is missing', this.pos(block.start));
    }
    return elseBlock;
  }

  private functionById(id: number, offset: number) {
    const fn = this.manifest.functions.find((candidate) => candidate.id === id);
    if (!fn) {
      throw new ScriptError(
        'E_UNKNOWN_FUNCTION_ID',
        `function id ${id} is not in the capability manifest`,
        this.pos(offset),
      );
    }
    return fn;
  }

  private dataVar(nameConst: number, kind: number, offset: number): Expr {
    const name = constantAsString(this.container, nameConst);
    const spec: DataVarSpec | undefined = this.manifest.dataVars?.find((item) => item.name === name);
    return {
      kind: 'dataVar',
      name,
      dataKind: spec ? (spec.kind === 'string' ? 3 : spec.kind === 'float' ? 2 : spec.kind === 'uint' ? 1 : 0) : kind,
      pos: this.pos(offset),
    };
  }

  private varSlot(varid: number): VarSlot {
    const { type, index } = decodeVar(varid);
    return { type, index, encoded: varid };
  }

  private portSlot(index: number): VarSlot {
    return { type: 'p', index, encoded: 0 };
  }

  private varExpr(varid: number, offset: number): Expr {
    const slot = this.varSlot(varid);
    const argName = slot.type === 'e' ? this.handlerArgNames[slot.index] : undefined;
    return {
      kind: 'var',
      name: argName ?? `$${slot.type}${slot.index}`,
      slot,
      pos: this.pos(offset),
    };
  }

  private portExpr(index: number, offset: number): Expr {
    const slot = this.portSlot(index);
    return { kind: 'var', name: `$${slot.type}${slot.index}`, slot, pos: this.pos(offset) };
  }

  private portLValue(index: number, offset: number): LValue {
    const slot = this.portSlot(index);
    return { kind: 'var', name: `$${slot.type}${slot.index}`, slot, pos: this.pos(offset) };
  }

  private varLValue(varid: number, offset: number): LValue {
    const slot = this.varSlot(varid);
    const argName = slot.type === 'e' ? this.handlerArgNames[slot.index] : undefined;
    return {
      kind: 'var',
      name: argName ?? `$${slot.type}${slot.index}`,
      slot,
      pos: this.pos(offset),
    };
  }

  private number(value: number, isFloat: boolean, offset: number): NumberExpr {
    return { kind: 'number', value, isFloat, pos: this.pos(offset) };
  }

  private string(value: string, offset: number): StringExpr {
    return { kind: 'string', value, pos: this.pos(offset) };
  }

  private call(name: string, args: Expr[], offset: number): CallExpr {
    return { kind: 'call', name, args, pos: this.pos(offset) };
  }

  private pos(offset: number): SourcePos {
    return { line: 1, column: offset + 1, offset };
  }
}

export function decompile(
  input: BytecodeContainer | Uint8Array,
  options: DecompileOptions = {},
): DecompileResult {
  try {
    const container = input instanceof Uint8Array ? readContainer(input) : input;
    const decompiler = new Decompiler(container, options.manifest ?? DEFAULT_MANIFEST);
    const program = decompiler.decompileProgram();
    return {
      ok: true,
      source: printProgram(program),
      program,
      errors: [],
    };
  } catch (err) {
    if (err instanceof ScriptError) {
      return {
        ok: false,
        errors: [{ code: err.code, message: err.message, pos: err.pos }],
      };
    }
    if (err instanceof ContainerError) {
      return {
        ok: false,
        errors: [
          {
            code: err.code,
            message: err.message,
            pos: { line: 1, column: 1, offset: 0 },
          },
        ],
      };
    }
    throw err;
  }
}
