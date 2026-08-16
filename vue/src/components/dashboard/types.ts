import type { IMessageBuffer, IDashboardItem } from '@/types';

export interface ICardChartProps extends Omit<IDashboardItem, 'value'> {
  value?: IMessageBuffer;
}

export interface ICardChartEmit {
  (e: 'click', event: Event): void;
  (e: 'setState', item: any): void;
  (e: 'update', event: Event): void;
}
