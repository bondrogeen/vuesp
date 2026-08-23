import type { IMessageSettings, TypeConstMessage, TypeKEYS } from 'vuesp-components/types';

export type MyTypeKEYS = (TypeKEYS & {}) | 'DEVICE';

export interface IMessageDevice {
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
