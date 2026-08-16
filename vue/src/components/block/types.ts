import type { IListItem, IMessageInfo, IMessageScan, IPackage, IDialog } from '@/types';

export interface IBlockStatusProps extends IMessageInfo {
  pkg?: IPackage;
}

export interface IBlockScanProps {
  items: Partial<IMessageScan>[];
}

export interface IBlockUpdateProps {
  onDialog: (data: IDialog) => void;
}

export interface IBlockServiceProps {
  locale: string;
  locales: IListItem[];
}

export interface IBlockServiceEmit {
  (e: 'reboot' | 'reset', event: Event): void;
  (e: 'locale', item: IListItem): void;
}
