export type {
  IStatusMeta,
  IRuntimeMetric,
  IScriptListItem,
  TLogMode,
  TStatusTone,
  TTranslator,
} from 'vuesp-components/types';

export interface IScriptPayload {
  name: string;
  bytes: Uint8Array;
  source: string;
  disassembly: string;
}
