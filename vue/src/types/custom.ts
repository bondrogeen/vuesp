import type { IMessageDevice, IMessageSettings, TypeConstMessage, TypeKEYS } from 'vuesp-components/types';

export interface IJbdDevice extends IMessageDevice {
  voltage: number;
  current: number;
  balanceCapacity: number;
  rateCapacity: number;
  cycle: number;
  empty: number;
  balanceStatus: number;
  balanceStatusHigh: number;
  protectionStatus: number;
  version: number;
  rsoc: number;
  fet: number;
  series: number;
  temp: number;
  ntc: number;
  ntc1: number;
  ntc2: number;
  ntc3: number;
  cellLow: number;
  cellHigh: number;
  cellDiff: number;
  cellAvg: number;
  dateDay: number;
  dateMonth: number;
  dateYear: number;
  cellVoltage: number[];
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
