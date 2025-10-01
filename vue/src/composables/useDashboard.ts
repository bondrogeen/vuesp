import type { IListItem, IDashboardItem } from '@/types';
import type { Ref } from 'vue';

import { onMounted, ref } from 'vue';

import { useFetch } from 'vuesp-components/helpers';
import { pathListDef, pathList } from '@/utils/const';

export const useDashboard = () => {
  const listMenu: IListItem[] = [
    { name: 'Add', value: 1 },
    { name: 'Save', value: 3 },
    { name: 'Restore', value: 2 },
    { name: 'Default', value: 4 },
  ];

  const dialogItem = ref(false);

  const item: Ref<IDashboardItem | null> = ref(null);
  const items: Ref<IDashboardItem[]> = ref([]);

  const getComponent = ({ type = 'info' }) => `card-${type}`;

  const onCreate = () => {
    item.value = null;
    dialogItem.value = true;
  };

  const onButton = (key: string, item: IDashboardItem) => {
    if (key === 'add') items.value.push(item);
    if (key === 'save') items.value = items.value.map((i) => (i.id === item.id ? item : i));
    if (key === 'remove') items.value = items.value.filter((i) => i.id !== item.id);
    dialogItem.value = false;
  };

  const onRestore = async () => {};
  const onSaveModule = async () => {};
  const onSaveDef = () => {};

  const onMenu = ({ value }: IListItem) => {
    if (value === 1) onCreate();
    if (value === 2) onSaveModule();
    if (value === 3) onRestore();
    if (value === 4) onSaveDef();
  };

  const onDialog = (data: IDashboardItem) => {
    item.value = data;
    dialogItem.value = true;
  };

  onMounted(async () => {
    let data = await useFetch.$get(`/fs?file=${pathList}`);
    if (!data) {
      data = await useFetch.$get(pathListDef);
    }
    items.value = data;
  });

  return { listMenu, dialogItem, item, items, getComponent, onMenu, onDialog, onButton };
};
