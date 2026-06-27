import type { IMessageSettings, IStateApp, IStateWebSocket, IStateWebSocketStore, IStateMain, TypeConstMessage, IPackage } from 'vuesp-components/types';

export type * from 'vuesp-components/types';

export enum KEYS {
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
  BUFFER = 'BUFFER',
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

export interface IMessageBuffer {
  key?: number;
  head: number;
  tail: number;
  count: number;
  data: number[];
}

interface IKeyMessageBuffer {
  key: 'BUFFER';
  object?: IMessageBuffer | Partial<IMessageBuffer>;
}
interface IKeyMessageDevice {
  key: 'DEVICE';
  object?: IMessageDevice | Partial<IMessageDevice>;
}

export type TypeMessage = (TypeConstMessage & {}) | IKeyMessageDevice | IKeyMessageBuffer;

type TypeMessageMap = { [K in TypeMessage['key']]: Extract<TypeMessage, { key: K }>['object'] };

export type TypeSend = <K extends keyof TypeMessageMap>(key: K, object?: TypeMessageMap[K]) => void;

export interface IStateDevice {
  key?: number;
  now: number;
  pwm: number;
  analog: number;
}

export interface IMyMessageSettings extends IMessageSettings {
  EthIp?: [];
  EthSubnet?: [];
  EthGateway?: [];
  EthDns?: [];
}

export interface IMyIStateMain extends IStateMain {
  device: IStateDevice;
  dallas: any;
}

export interface IStoreApp extends IStateApp {
  dialogInfo: boolean;
  pkg: IPackage;
}
export interface IStoreWebSocket extends IStateWebSocket {}

export interface IStoreWebSocketStore extends IStateWebSocketStore {
  settings: IMyMessageSettings;
  main: IMyIStateMain;
}
