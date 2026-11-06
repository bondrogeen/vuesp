import type { IMessageDevice, IMessageSettings, TypeConstMessage, TypeKEYS } from 'vuesp-components/types';

export interface IDraw {
  key?: number;
  command: number;
  effect: number;
  brightness: number;
  buffer: number[];
}

export interface ISensors {
  key?: number;
  command: number;
  status: number;
  bmpTemperature: number;
  bmpPressure: number;
  bmpAltitude: number;
  ahtTemperature: number;
  ahtHumidity: number;
}

export type MyTypeKEYS = (TypeKEYS & {}) | 'DEVICE' | 'DRAW' | 'SENSORS';

interface IKeyMessageDevice {
  key: Extract<MyTypeKEYS, 'DEVICE'>;
  object?: IMessageDevice | Partial<IMessageDevice>;
}

interface IKeyMessageDraw {
  key: Extract<MyTypeKEYS, 'DRAW'>;
  object?: IDraw | Partial<IDraw>;
}

interface IKeyMessageSensors {
  key: Extract<MyTypeKEYS, 'SENSORS'>;
  object?: ISensors | Partial<ISensors>;
}

export type TypeMessage = (TypeConstMessage & {}) | IKeyMessageDevice | IKeyMessageDraw | IKeyMessageSensors;

export interface IMyMessageSettings extends IMessageSettings {
  EthIp?: [];
  EthSubnet?: [];
  EthGateway?: [];
  EthDns?: [];
}
