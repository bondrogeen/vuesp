import type { IStatusMeta, IScriptListItem, ITranslator, TLogMode, TStatusTone } from '@/types';

const BYTES_PER_HEX_LINE = 16;
const HEX_PAD_LENGTH = 2;

export const TONE_TAG_CLASS: Record<TStatusTone, string> = {
  amber: 'bg-amber-main/10 text-amber-main',
  red: 'bg-red-main/10 text-red-main',
  green: 'bg-green-main/10 text-green-main',
  blue: 'bg-blue-main/10 text-blue-main',
  gray: 'bg-gray-main/10 text-gray-main',
};

export const TONE_DOT_CLASS: Record<TStatusTone, string> = {
  amber: 'bg-amber-main',
  red: 'bg-red-main',
  green: 'bg-green-main',
  blue: 'bg-blue-main',
  gray: 'bg-gray-main',
};

export const getToneTagClass = (tone: TStatusTone): string => TONE_TAG_CLASS[tone];

export const getToneDotClass = (tone: TStatusTone): string => TONE_DOT_CLASS[tone];

const STATUS_MATCHERS: Array<{ matches: (script: IScriptListItem) => boolean; tone: TStatusTone; key: string }> = [
  { matches: (script) => script.error > 0, tone: 'red', key: 'scriptStudio.stateError' },
  { matches: (script) => script.active > 0, tone: 'green', key: 'scriptStudio.stateRunning' },
  { matches: (script) => script.loaded > 0, tone: 'blue', key: 'scriptStudio.stateLoaded' },
  { matches: () => true, tone: 'gray', key: 'scriptStudio.stateStopped' },
];

export const getScriptStatusMeta = (script: IScriptListItem | null, t: ITranslator): IStatusMeta => {
  if (!script) return { label: t('scriptStudio.stateNotSaved'), tone: 'amber' };
  const matcher = STATUS_MATCHERS.find((item) => item.matches(script)) ?? STATUS_MATCHERS[STATUS_MATCHERS.length - 1];
  return { label: t(matcher.key), tone: matcher.tone };
};

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

export const toHexText = (text: string): string =>
  Array.from(new TextEncoder().encode(text))
    .map((byte) => byte.toString(16).padStart(HEX_PAD_LENGTH, '0'))
    .join(' ');

export const getLogText = (text: string, mode: TLogMode): string =>
  mode === 'text' ? text : toHexText(text);

export const formatLogLine = (entry: { time: string; text: string }, mode: TLogMode): string =>
  `${entry.time}  ${getLogText(entry.text, mode)}`;

export const toArrayBuffer = (bytes: Uint8Array): ArrayBuffer => {
  const copy = new Uint8Array(bytes.length);
  copy.set(bytes);
  return copy.buffer;
};

export const downloadBlob = (data: BlobPart, fileName: string, type: string): void => {
  const blob = new Blob([data], { type });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = fileName;
  link.rel = 'noopener';
  document.body.appendChild(link);
  link.click();
  link.remove();
  window.setTimeout(() => URL.revokeObjectURL(url), 0);
};

export const SCRIPT_NAME_PATTERN = /^[a-z_][a-z0-9_-]{0,15}$/;

export const isValidScriptName = (name: string): boolean => SCRIPT_NAME_PATTERN.test(name);

export const copyText = async (text: string): Promise<boolean> => {
  if (!navigator.clipboard?.writeText) return false;
  try {
    await navigator.clipboard.writeText(text);
    return true;
  } catch {
    return false;
  }
};

export const getMessageText = (error: unknown): string => {
  if (error instanceof Error && error.message) return error.message;
  if (typeof error === 'string') return error;
  return String(error);
};

export const scrollIntoView = (element: HTMLElement | null): void => {
  element?.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
};
