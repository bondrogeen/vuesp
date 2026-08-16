import type { IMessageSettings, TypeConstMessage, TypeKEYS } from 'vuesp-components/types';

export type MyTypeKEYS = (TypeKEYS & {}) | 'DEVICE';

export interface IMessageDevice {
  key?: number;
  command: number;
  analog: number;
  pwm: number;
  now: number;
  list: number;
  message: string;
}

interface IKeyMessageDevice {
  key: Extract<MyTypeKEYS, 'DEVICE'>;
  object?: IMessageDevice | Partial<IMessageDevice>;
}

export type TypeMessage = (TypeConstMessage & {}) | IKeyMessageDevice;

export interface IMyMessageSettings extends IMessageSettings {
  EthIp?: [];
  EthSubnet?: [];
  EthGateway?: [];
  EthDns?: [];
}
