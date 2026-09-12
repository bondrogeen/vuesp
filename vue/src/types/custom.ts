import type { IMessageSettings, TypeConstMessage, TypeKEYS } from 'vuesp-components/types';

export type MyTypeKEYS = (TypeKEYS & {}) | 'DEVICE';

export interface IMessageDevice {
  key?: number;
  command: number;
  analog: number;
  voltage?: number;
  current?: number;
  power?: number;
  cos?: number;
  frequency?: number;
  address?: number;
  baud?: number;
  format?: number;
  activeEnergy?: number;
  positiveActiveEnergy?: number;
  reverseActiveEnergy?: number;
  reactiveEnergy?: number;
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
