import type { IMessageSettings, IStateApp, IStateWebSocket, IStateWebSocketStore, IStateMain, TypeConstMessage, IPackage, IMessageMessage } from 'vuesp-components/types';

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
  PORT = 'PORT',
  MESSAGE = 'MESSAGE',
  DALLAS = 'DALLAS',
  BUFFER = 'BUFFER',
  DEVICE = 'DEVICE',
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
  key: number;
  command: number;
  voltage: number;
  current: number;
  capacity: number;
  fccRaw: number;
  cycle: number;
  empty: number;
  dccRaw: number;
  SOC: number;
  protectionStatus: number;
  cell1: number;
  cell2: number;
  cell3: number;
  cell4: number;
  temp1: number;
  temp2: number;
  temp3: number;
  temp4: number;
  cellStatus1: number;
  cellStatus2: number;
  cellStatus3: number;
  cellStatus4: number;
  tempStatus1: number;
  tempStatus2: number;
  tempStatus3: number;
  tempStatus4: number;
  chgCurrStatus: number;
  totalVoltStatus: number;
  dsgCurrStatus: number;
  cellCount: number;
  status1: number;
  status2: number;
  status3: number;
  status4: number;
  status5: number;
  status6: number;
  status7: number;
  status8: number;
  status9: number;
  tempCount: number;
  empty1: number;
  empty2: number;
  version: string;
}

export interface IDiscovery {
  key?: number;
  status: number;
  empty: number;
  id: number;
  ip: number;
  lastSeen: number;
}

export interface IMyMessageSettings extends IMessageSettings {
  discovery: number;
  discoveryInterval: number;
  discoveryPort: number;
  EthIp?: [];
  EthSubnet?: [];
  EthGateway?: [];
  EthDns?: [];
}

export interface IMyIStateMain extends IStateMain {
  device: IStateDevice;
  dallas: any;
  discovery: Record<string, IDiscovery>;
  slots: Record<number, IMessageMessage>;
}

export interface IStoreApp extends IStateApp {
  dialogInfo: boolean;
  pkg: IPackage;
}
export interface IStoreWebSocket extends IStateWebSocket {}

export interface IStoreWebSocketStore extends IStateWebSocketStore {
  settings: IMyMessageSettings;
  main: IMyIStateMain;
  message: IMessageMessage;
}
