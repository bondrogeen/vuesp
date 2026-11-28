export {
  SCRIPT_STATE,
  TONE_TAG_CLASS,
  TONE_DOT_CLASS,
  getToneTagClass,
  getToneDotClass,
  getScriptStatusMeta,
  isScriptActive,
  isScriptLoaded,
  isScriptError,
  SCRIPT_NAME_MAX_LENGTH,
  SCRIPT_NAME_PATTERN,
  isValidScriptName,
  toHexText,
  getLogText,
  formatLogLine,
  copyText,
  downloadBlob,
  scrollIntoView,
} from 'vuesp-components/components';

const BYTES_PER_HEX_LINE = 16;
const HEX_PAD_LENGTH = 2;

export const hexDump = (bytes: Uint8Array): string => {
  const lines: string[] = [];
  for (let offset = 0; offset < bytes.length; offset += BYTES_PER_HEX_LINE) {
    const chunk = Array.from(bytes.slice(offset, offset + BYTES_PER_HEX_LINE))
      .map((byte) => byte.toString(16).padStart(HEX_PAD_LENGTH, '0'))
      .join(' ');
    lines.push(`${offset.toString(16).padStart(4, '0')}  ${chunk}`);
  }
  return lines.join('\n');
};

export const toArrayBuffer = (bytes: Uint8Array): ArrayBuffer => {
  const copy = new Uint8Array(bytes.length);
  copy.set(bytes);
  return copy.buffer;
};

export const getMessageText = (error: unknown): string => {
  if (error instanceof Error && error.message) return error.message;
  if (typeof error === 'string') return error;
  return String(error);
};

const SCRIPT_RESULT_NAMES = [
  'OK',
  'Not found',
  'Invalid',
  'Busy',
  'I/O error',
  'CRC error',
  'Verify error',
  'No space',
  'No cache',
];

export const getScriptResultName = (code: number): string =>
  SCRIPT_RESULT_NAMES[code] ?? `Result ${code}`;

const PACK_ERROR_NAMES = [
  'none',
  'short pack',
  'bad magic',
  'unsupported version',
  'entry count',
  'toc size',
  'toc crc',
  'bad name',
  'duplicate name',
  'out of bounds',
  'code crc',
  'overlap',
  'capacity',
  'code length',
  'flags',
];

export const getPackErrorName = (code: number): string =>
  PACK_ERROR_NAMES[code] ?? `Pack error ${code}`;

const VM_ERROR_NAMES = [
  'none',
  'parse',
  'verify',
  'opcode',
  'bounds',
  'stack',
  'type',
  'variable',
  'constant',
  'data',
  'runtime',
  'no context',
  'no script',
  'no function',
  'arguments',
  'not ready',
  'code crc',
  'no cache',
  'recursion',
  'depth',
  'no variable memory',
];

export const getVmErrorName = (code: number): string =>
  VM_ERROR_NAMES[code] ?? `VM error ${code}`;
