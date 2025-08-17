export interface IWSSend {
  comm: string;
  data?: any;
}

export interface IMenuChild {
  name: string;
  path?: string;
}

export interface IMenuItem {
  name: string;
  icon: string;
  path: string;
  children?: IMenuChild[];
}

export interface IDialog {
  message?: string;
  title?: string;
  value?: boolean;
  callback?: () => void;
}

export interface IListMenu {
  id: number;
  name: string;
}

export interface ITextFieldFile {
  name: string;
  size: number;
}
export interface ITextFieldInfo {
  files: ITextFieldFile[];
  totalSize: number;
}
export interface ITextFieldEvent {
  files: FileList;
  info?: ITextFieldInfo;
}

export interface IStateInfo {
  id?: number;
  firmware?: number[];
  totalBytes?: number;
  usedBytes?: number;
  uptime?: number;
  name?: string;
}
export interface IStateFile {
  key: number;
  isDir: number;
  isFile: number;
  command?: number;
  size?: number;
  name?: string;
}

export interface IGpio {
  key: number;
  command: number;
  gpio: number;
  data: number;
}

export interface IStateGpio {
  [key: string]: IGpio;
}

export interface IListWiFi {
  name: string;
  value: number;
}

export interface IStateScan {
  key: number;
  id: number;
  isHidden: number;
  encryptionType: number;
  channel: number;
  rssi: number;
  ssid: string;
}
export interface IStateSettings {
  key?: number;
  wifiDhcp?: number;
  wifiMode?: number;
  authMode?: number;
  version?: number;
  device?: number;
  wifiIp?: number[];
  wifiSubnet?: number[];
  wifiGateway?: number[];
  wifiDns?: number[];
  wifiSsid?: string;
  wifiPass?: string;
  authLogin?: string;
  authPass?: string;
}

export interface IStateMain {
  info?: IStateInfo;
  device: any;
  dallas: any;
}

export interface IStateWebSocket {
  fileList: IStateFile[];
  gpio: IStateGpio;
  scanList: IStateScan[];
  path: string[];
  settings: IStateSettings;
  main: IStateMain;
  progress: any;
  unknown: any;
}

export interface INotificationItem {
  id?: number;
  text?: string;
  timeout?: number;
}
export interface IStateApp {
  menu: IMenuItem[];
  isLoading: boolean;
  theme: string;
  dialog: IDialog;
  notifications: INotificationItem[];
  struct: {};
}
