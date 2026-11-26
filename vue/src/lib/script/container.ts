// VBC1 container reader/writer and CRC32.
// The container is intentionally small and allocation-light so the C++ reader
// can mirror these structures without a schema library.

export const CONTAINER_MAGIC = 'VBC1';
export const CONTAINER_VERSION = 1;
export const HEADER_SIZE = 28;

export const CONST_INT32 = 1;
export const CONST_UINT32 = 2;
export const CONST_FLOAT32 = 3;
export const CONST_STRING = 4;
export const CONST_NAME = 5;
export const CONST_ARRAY = 6;

export const BLOCK_IF = 1;
export const BLOCK_ELSE = 2;
export const BLOCK_WHILE = 3;
export const BLOCK_HANDLER = 4;
export const BLOCK_TERNARY = 5;

export interface ConstantEntry {
  readonly tag: number;
  readonly value: number | string | Uint8Array;
}

export interface BlockEntry {
  readonly kind: number;
  readonly start: number;
  readonly middle: number;
  readonly end: number;
}

export interface HandlerEntry {
  readonly eventId: number;
  readonly nameConst: number;
  readonly entry: number;
  readonly end: number;
}

export interface BytecodeHeader {
  readonly magic: string;
  readonly version: number;
  readonly flags: number;
  readonly headerSize: number;
  readonly codeLength: number;
  readonly constCount: number;
  readonly blockCount: number;
  readonly handlerCount: number;
  readonly entryOffset: number;
  readonly stackMax: number;
  readonly sourceHash: number;
  readonly crc32: number;
}

export interface BytecodeContainer {
  readonly header: BytecodeHeader;
  readonly constants: readonly ConstantEntry[];
  readonly code: Uint8Array;
  readonly blocks: readonly BlockEntry[];
  readonly handlers: readonly HandlerEntry[];
}

export class ContainerError extends Error {
  readonly code: string;

  constructor(code: string, message: string) {
    super(message);
    this.name = 'ContainerError';
    this.code = code;
  }
}

const CRC_TABLE = (() => {
  const table = new Uint32Array(256);
  for (let i = 0; i < 256; i++) {
    let c = i;
    for (let j = 0; j < 8; j++) {
      c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    }
    table[i] = c >>> 0;
  }
  return table;
})();

export function crc32(data: Uint8Array): number {
  let crc = 0xffffffff;
  for (const byte of data) {
    crc = CRC_TABLE[(crc ^ byte) & 0xff] ^ (crc >>> 8);
  }
  return (crc ^ 0xffffffff) >>> 0;
}

export function writeU16(target: number[], value: number): void {
  target.push(value & 0xff, (value >>> 8) & 0xff);
}

export function writeU32(target: number[], value: number): void {
  target.push(value & 0xff, (value >>> 8) & 0xff, (value >>> 16) & 0xff, (value >>> 24) & 0xff);
}

export function writeF32(target: number[], value: number): void {
  const buf = new ArrayBuffer(4);
  new DataView(buf).setFloat32(0, value, true);
  const bytes = new Uint8Array(buf);
  target.push(bytes[0], bytes[1], bytes[2], bytes[3]);
}

export function readU16(bytes: Uint8Array, offset: number): number {
  return bytes[offset] | (bytes[offset + 1] << 8);
}

export function readU32(bytes: Uint8Array, offset: number): number {
  return (
    (bytes[offset] |
      (bytes[offset + 1] << 8) |
      (bytes[offset + 2] << 16) |
      (bytes[offset + 3] << 24)) >>>
    0
  );
}

export function readF32(bytes: Uint8Array, offset: number): number {
  return new DataView(bytes.buffer, bytes.byteOffset + offset, 4).getFloat32(0, true);
}

function writeConstant(target: number[], entry: ConstantEntry): void {
  target.push(entry.tag);
  if (entry.tag === CONST_INT32 || entry.tag === CONST_UINT32) {
    writeU16(target, 4);
    writeU32(target, entry.value as number);
  } else if (entry.tag === CONST_FLOAT32) {
    writeU16(target, 4);
    writeF32(target, entry.value as number);
  } else if (entry.tag === CONST_STRING || entry.tag === CONST_NAME) {
    const bytes = new TextEncoder().encode(entry.value as string);
    writeU16(target, bytes.length);
    for (const byte of bytes) target.push(byte);
  } else if (entry.tag === CONST_ARRAY) {
    const bytes = entry.value as Uint8Array;
    writeU16(target, bytes.length);
    for (const byte of bytes) target.push(byte);
  } else {
    throw new ContainerError('E_CONST_TAG', `unknown constant tag ${entry.tag}`);
  }
}

export function writeContainer(container: BytecodeContainer): Uint8Array {
  const payload: number[] = [];
  for (const entry of container.constants) writeConstant(payload, entry);

  const bytes: number[] = [];
  for (const ch of CONTAINER_MAGIC) bytes.push(ch.charCodeAt(0));
  bytes.push(CONTAINER_VERSION);
  bytes.push(container.header.flags & 0xff);
  writeU16(bytes, HEADER_SIZE);
  writeU16(bytes, container.code.length);
  writeU16(bytes, container.constants.length);
  writeU16(bytes, container.blocks.length);
  writeU16(bytes, container.handlers.length);
  writeU16(bytes, container.header.entryOffset);
  writeU16(bytes, container.header.stackMax);
  writeU32(bytes, container.header.sourceHash);
  writeU32(bytes, 0); // CRC placeholder

  for (const byte of payload) bytes.push(byte);
  for (const byte of container.code) bytes.push(byte);
  for (const block of container.blocks) {
    bytes.push(block.kind);
    writeU16(bytes, block.start);
    writeU16(bytes, block.middle);
    writeU16(bytes, block.end);
  }
  for (const handler of container.handlers) {
    bytes.push(handler.eventId);
    writeU16(bytes, handler.nameConst);
    writeU16(bytes, handler.entry);
    writeU16(bytes, handler.end);
  }

  const result = new Uint8Array(bytes);
  const crc = crc32(result);
  result[24] = crc & 0xff;
  result[25] = (crc >>> 8) & 0xff;
  result[26] = (crc >>> 16) & 0xff;
  result[27] = (crc >>> 24) & 0xff;
  return result;
}

export function readContainer(bytes: Uint8Array): BytecodeContainer {
  if (bytes.length < HEADER_SIZE) {
    throw new ContainerError('E_SHORT', 'container is shorter than the header');
  }
  const magic = String.fromCharCode(bytes[0], bytes[1], bytes[2], bytes[3]);
  if (magic !== CONTAINER_MAGIC) {
    throw new ContainerError('E_MAGIC', `bad magic '${magic}'`);
  }
  const version = bytes[4];
  if (version !== CONTAINER_VERSION) {
    throw new ContainerError('E_VERSION', `unsupported version ${version}`);
  }
  const flags = bytes[5];
  const headerSize = readU16(bytes, 6);
  if (headerSize !== HEADER_SIZE) {
    throw new ContainerError('E_HEADER', `unsupported header size ${headerSize}`);
  }
  const codeLength = readU16(bytes, 8);
  const constCount = readU16(bytes, 10);
  const blockCount = readU16(bytes, 12);
  const handlerCount = readU16(bytes, 14);
  const entryOffset = readU16(bytes, 16);
  const stackMax = readU16(bytes, 18);
  const sourceHash = readU32(bytes, 20);
  const storedCrc = readU32(bytes, 24);

  const copy = new Uint8Array(bytes);
  copy[24] = 0;
  copy[25] = 0;
  copy[26] = 0;
  copy[27] = 0;
  const actualCrc = crc32(copy);
  if (actualCrc !== storedCrc) {
    throw new ContainerError('E_CRC', `CRC mismatch 0x${storedCrc.toString(16)} != 0x${actualCrc.toString(16)}`);
  }

  let offset = HEADER_SIZE;
  const constants: ConstantEntry[] = [];
  for (let i = 0; i < constCount; i++) {
    if (offset >= bytes.length) throw new ContainerError('E_BOUNDS', 'constant pool is truncated');
    const tag = bytes[offset++];
    if (offset + 2 > bytes.length) throw new ContainerError('E_BOUNDS', 'constant length is truncated');
    const length = readU16(bytes, offset);
    offset += 2;
    if (offset + length > bytes.length) throw new ContainerError('E_BOUNDS', 'constant payload is truncated');
    const payload = bytes.subarray(offset, offset + length);
    offset += length;
    if (tag === CONST_INT32 || tag === CONST_UINT32) {
      if (length !== 4) throw new ContainerError('E_CONST_LEN', 'numeric constant must be 4 bytes');
      constants.push({ tag, value: readU32(payload, 0) });
    } else if (tag === CONST_FLOAT32) {
      if (length !== 4) throw new ContainerError('E_CONST_LEN', 'float constant must be 4 bytes');
      constants.push({ tag, value: readF32(payload, 0) });
    } else if (tag === CONST_STRING || tag === CONST_NAME) {
      constants.push({ tag, value: new TextDecoder().decode(payload) });
    } else if (tag === CONST_ARRAY) {
      constants.push({ tag, value: new Uint8Array(payload) });
    } else {
      throw new ContainerError('E_CONST_TAG', `unknown constant tag ${tag}`);
    }
  }

  if (offset + codeLength > bytes.length) throw new ContainerError('E_BOUNDS', 'code section is truncated');
  const code = bytes.subarray(offset, offset + codeLength);
  offset += codeLength;

  const blocks: BlockEntry[] = [];
  for (let i = 0; i < blockCount; i++) {
    if (offset + 7 > bytes.length) throw new ContainerError('E_BOUNDS', 'block table is truncated');
    const kind = bytes[offset];
    const start = readU16(bytes, offset + 1);
    const middle = readU16(bytes, offset + 3);
    const end = readU16(bytes, offset + 5);
    offset += 7;
    blocks.push({ kind, start, middle, end });
  }

  const handlers: HandlerEntry[] = [];
  for (let i = 0; i < handlerCount; i++) {
    if (offset + 7 > bytes.length) throw new ContainerError('E_BOUNDS', 'handler table is truncated');
    const eventId = bytes[offset];
    const nameConst = readU16(bytes, offset + 1);
    const entry = readU16(bytes, offset + 3);
    const end = readU16(bytes, offset + 5);
    offset += 7;
    handlers.push({ eventId, nameConst, entry, end });
  }

  if (offset !== bytes.length) {
    throw new ContainerError('E_TRAILING', 'container has trailing bytes');
  }

  return {
    header: {
      magic,
      version,
      flags,
      headerSize,
      codeLength,
      constCount,
      blockCount,
      handlerCount,
      entryOffset,
      stackMax,
      sourceHash,
      crc32: storedCrc,
    },
    constants,
    code,
    blocks,
    handlers,
  };
}

export function constantAsString(container: BytecodeContainer, index: number): string {
  const entry = container.constants[index];
  if (!entry || (entry.tag !== CONST_STRING && entry.tag !== CONST_NAME)) {
    throw new ContainerError('E_CONST_TYPE', `constant ${index} is not a string`);
  }
  return entry.value as string;
}

export function constantAsArray(container: BytecodeContainer, index: number): Uint8Array {
  const entry = container.constants[index];
  if (!entry || entry.tag !== CONST_ARRAY) {
    throw new ContainerError('E_CONST_TYPE', `constant ${index} is not an array`);
  }
  return entry.value as Uint8Array;
}
