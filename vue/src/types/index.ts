import type {
  IMessageInfo,
  IMessageSettings,
  IMessageFile,
  IMessageScan,
  IMessagePort,
  IMessageProgress,
  IMessageReboot,
  IStateApp,
  IStateWebSocket,
  IStateWebSocketStore,
} from 'vuesp-components/types';

export const enum KEYS {
  INIT = 'INIT',
  SETTINGS = 'SETTINGS',
  INFO = 'INFO',
  PING = 'PING',
  SCAN = 'SCAN',
  PROGRESS = 'PROGRESS',
  FILES = 'FILES',
  REBOOT = 'REBOOT',
  PINS = 'PINS',
  PORT = 'PORT',
  DEVICE = 'DEVICE',
  DALLAS = 'DALLAS',
}

export type * from 'vuesp-components/types';

interface IKeyMessageSettings {
  key: KEYS.SETTINGS;
  object: IMessageSettings;
}

interface IKeyMessageInfo {
  key: KEYS.INFO;
  object: IMessageInfo;
}

interface IKeyMessagePing {
  key: KEYS.PING;
  object: IMessageFile;
}

interface IKeyMessageScan {
  key: KEYS.SCAN;
  object: IMessageScan;
}

interface IKeyMessageProgress {
  key: KEYS.PROGRESS;
  object: IMessageProgress;
}

interface IKeyMessageFiles {
  key: KEYS.FILES;
  object: IMessageFile;
}

interface IKeyMessageReboot {
  key: KEYS.REBOOT;
  object: IMessageReboot;
}

interface IKeyMessagePort {
  key: KEYS.PORT;
  object: IMessagePort;
}

export type TypeMessage = IKeyMessageSettings | IKeyMessageInfo | IKeyMessagePing | IKeyMessageScan | IKeyMessageProgress | IKeyMessageFiles | IKeyMessageReboot | IKeyMessagePort;

export interface IMyMessageSettings extends IMessageSettings {
  EthIp?: [];
  EthSubnet?: [];
  EthGateway?: [];
  EthDns?: [];
}

export interface IStoreApp extends IStateApp {
  dialogInfo: boolean;
}
export interface IStoreWebSocket extends IStateWebSocket {}

export interface IStoreWebSocketStore extends IStateWebSocketStore {
  settings: IMyMessageSettings;
}
