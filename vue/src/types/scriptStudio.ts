export interface ITranslator {
  (key: string): string;
}

export type TStatusTone = 'amber' | 'red' | 'green' | 'blue' | 'gray';

export type TLogMode = 'text' | 'hex';

export interface IScriptListItem {
  name: string;
  id: number;
  size: number;
  boot: number;
  loaded: number;
  active: number;
  error: number;
  flags: number;
}

export interface IScriptStatus {
  active: number;
  scripts: number;
  saved: number;
  cacheSlots: number;
  packError: number;
  packErrorName: string;
  error: number;
  errorName: string;
  errorScript: number;
  errorScriptName: string;
  errorPc: number;
}

export interface IStatusMeta {
  label: string;
  tone: TStatusTone;
}

export interface IRuntimeMetric {
  key: string;
  label: string;
  value: string;
  isError?: boolean;
  title?: string;
}

export interface IScriptPayload {
  name: string;
  bytes: Uint8Array;
  source: string;
  disassembly: string;
}
