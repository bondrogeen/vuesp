import type { IMessageDevice, IMessageSettings, TypeConstMessage, TypeKEYS } from 'vuesp-components/types';

declare module 'vuesp-components/types' {
  interface IMessageDevice {
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
}

export type MyTypeKEYS = (TypeKEYS & {}) | 'DEVICE';

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
