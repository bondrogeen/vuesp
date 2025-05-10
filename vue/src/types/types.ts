export interface TypeWSSend {
  comm: string;
  data?: any;
}

export interface TypeMenuChild {
  name: string;
  path?: string;
}

export interface TypeMenuItem {
  name: string;
  icon: string;
  path: string;
  children?: TypeMenuChild[];
}

export interface TypeMenu {
  title: string;
  path: string;
  items: TypeMenuItem[];
}

export interface TypeBreadcrumb {
  name: string;
  path?: string;
}

export interface TypeDialog {
  message?: string;
  title?: string;
  value?: boolean;
  callback?: () => void;
}

export interface TypeListMenu {
  id: number;
  name: string;
}

export interface TypeTextFieldFile {
  name: string;
  size: number;
}
export interface TypeTextFieldInfo {
  files: TypeTextFieldFile[];
  totalSize: number;
}
export interface TypeTextFieldEvent {
  files: FileList;
  info?: TypeTextFieldInfo;
}

export interface TypeStateInfo {
  id?: number;
  firmware?: number[];
  totalBytes?: number;
  usedBytes?: number;
  uptime?: number;
}
export interface TypeStateFile {
  key: number;
  isDir: number;
  isFile: number;
  command?: number;
  size?: number;
  name?: string;
}

export interface TypeGpio {
  key: number;
  command: number;
  gpio: number;
  data: number;
}

export interface TypeStateGpio {
  [key: string]: TypeGpio;
}

export interface TypelistWiFi {
  name: string;
  value: number;
}

export interface TypeStateScan {
  key: number;
  id: number;
  isHidden: number;
  encryptionType: number;
  channel: number;
  rssi: number;
  ssid: string;
}
export interface TypeStateSettings {
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

export interface TypeStateWebSocket {
  info?: TypeStateInfo;
  fileList: TypeStateFile[];
  gpio: TypeStateGpio;
  scanList: TypeStateScan[];
  path: string[];
  settings: TypeStateSettings;
  progress: any;
  unknown: any;
  device: any;
  dallas: any;
}

export interface TypeNotificationItem {
  id?: number;
  text?: string;
  timeout?: number;
}
export interface TypeStateApp {
  menu: TypeMenu[];
  isLoading: boolean;
  theme: string;
  dialog: TypeDialog;
  notifications: TypeNotificationItem[];
  struct: {};
}
