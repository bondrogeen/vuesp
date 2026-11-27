import type { BytecodeExample, CapabilityManifest, CompileResult, EventSpec, FunctionSpec } from '@/lib/script';
import type { IStatusMeta, IRuntimeMetric, IScriptListItem } from '@/types';

export interface IScriptStudioToolbarProps {
  name: string;
  status: IStatusMeta;
  isBusy: boolean;
  isVmAvailable: boolean;
  isSettingsOpen: boolean;
  isDownloadAvailable: boolean;
  isCurrentSelected: boolean;
}

export interface IScriptStudioToolbarEmits {
  save: [];
  rename: [];
  reload: [];
  download: [];
  'stop-all': [];
  remove: [];
  'toggle-settings': [];
}

export interface IScriptStudioMetricsProps {
  metrics: IRuntimeMetric[];
  isBusy: boolean;
  isDownloadAvailable: boolean;
  isCurrentSelected: boolean;
  instrPerTick: number;
  usPerTick: number;
}

export interface IScriptStudioMetricsEmits {
  reload: [];
  download: [];
  'stop-all': [];
}

export interface IScriptStudioActionBarProps {
  logsCount: number;
  lineCount: number;
  byteCount: number;
  stackMax: number | null;
}

export interface IScriptStudioActionBarEmits {
  insert: [];
  examples: [];
  details: [];
  'show-logs': [];
}

export interface IScriptStudioTabsProps {
  scripts: IScriptListItem[];
  activeName: string;
  draftName: string | null;
  isBusy: boolean;
}

export interface IScriptStudioTabsEmits {
  select: [script: IScriptListItem];
  'toggle-run': [script: IScriptListItem];
  remove: [name: string];
  create: [];
}

export interface IScriptStudioLogsEmits {
  'update:count': [count: number];
}

export interface IScriptSnippetsDialogProps {
  manifest: CapabilityManifest;
}

export interface IScriptSnippetsDialogEmits {
  'insert-function': [fn: FunctionSpec];
  'insert-event': [event: EventSpec];
  close: [];
}

export interface IScriptExamplesDialogProps {
  examples: readonly BytecodeExample[];
}

export interface IScriptExamplesDialogEmits {
  load: [example: BytecodeExample];
  close: [];
}

export interface IScriptDetailsDialogProps {
  result: CompileResult | null;
  hex: string;
}

export interface IScriptDetailsDialogEmits {
  close: [];
}

export interface IScriptDeleteDialogProps {
  name: string;
  isBusy: boolean;
}

export interface IScriptDeleteDialogEmits {
  confirm: [];
  close: [];
}

export interface IScriptRenameDialogProps {
  name: string;
  isBusy: boolean;
}

export interface IScriptRenameDialogEmits {
  save: [name: string];
  close: [];
}
