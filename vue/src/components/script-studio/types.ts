import type { CompileResult } from '@/lib/script';
import type { IRuntimeMetric, IScriptListItem } from '@/types';

export interface IScriptStudioToolbarProps {
  name: string;
  isBusy: boolean;
  isVmAvailable: boolean;
  isSettingsOpen: boolean;
  isDownloadAvailable: boolean;
  isDirty: boolean;
}

export interface IScriptStudioToolbarEmits {
  save: [];
  rename: [];
  reload: [];
  import: [];
  download: [];
  'stop-all': [];
  'toggle-settings': [];
}

export interface IScriptStudioMetricsProps {
  metrics: IRuntimeMetric[];
}

export interface IScriptStudioActionBarProps {
  logsCount: number;
  lineCount: number;
  byteCount: number;
  stackMax: number | null;
}

export interface IScriptStudioActionBarEmits {
  details: [];
  'show-logs': [];
}

export interface IScriptStudioTabsProps {
  scripts: IScriptListItem[];
  activeName: string;
  draftName: string | null;
  isBusy: boolean;
  isDirty: boolean;
  maxScripts: number;
  ranName: string | null;
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

export interface IScriptDiscardDialogEmits {
  save: [];
  discard: [];
  cancel: [];
}
