import type { IMessageSettings, IStateApp, IStateWebSocket, IStateWebSocketStore, TypeConstMessage } from 'vuesp-components/types';

export type * from 'vuesp-components/types';

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

export interface IMessageDevice {
  key?: number;
  command: number;
  gpio12: number;
  gpio14: number;
  analog: number;
  pwm: number;
  now: number;
  message: string;
}

interface IKeyMessageDevice {
  key: 'DEVICE';
  object?: IMessageDevice | Partial<IMessageDevice>;
}

export type TypeMessage = (TypeConstMessage & {}) | IKeyMessageDevice;

type TypeMessageMap = { [K in TypeMessage['key']]: Extract<TypeMessage, { key: K }>['object'] };

export type TypeSend = <K extends keyof TypeMessageMap>(key: K, object?: TypeMessageMap[K]) => void;

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
