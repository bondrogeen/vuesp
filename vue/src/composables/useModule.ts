import type { IDashboard, IDashboardItemString, TypeSend } from '@/types';
import { KEYS } from '@/types';

import type { Ref } from 'vue';

import { onMounted, ref, computed, watch } from 'vue';
import { storeToRefs } from 'pinia';

import { Dashboard } from 'vuesp-components/dashboard';

import { useFetch, uploadBinary } from 'vuesp-components/helpers';
import { pathListDef, pathList } from '@/utils/const.ts';

import { useWebSocketStore } from '@/stores/WebSocketStore.ts';

export const useModule = () => {
  const webSocketStore = useWebSocketStore();
  const { main } = storeToRefs(webSocketStore);

  const data: Ref<IDashboard | null> = ref(null);

  const getList = computed(() => data.value?.getList?.());

  const setState = (id: string, value: any, options?: any) => {
    data.value?.set(id, value);
    const { device } = data.value?.getData();
    onSend(KEYS.DEVICE, { ...device, ...options });
  };

  const getState = (id: string) => {
    return data.value?.get(id);
  };

  const onEditItem = (item: IDashboardItemString) => {
    if (!item.id) return;
    data.value?.editItem(item.id, { ...item });
  };

  const onRemoveItem = (item: IDashboardItemString) => {
    if (!item.id) return;
    data.value?.removeItem(item.id);
  };

  const loadModule = async (path: string, init?: RequestInit) => {
    const text = await useFetch.get(path, init).then((res) => res.text());
    return await import('data:text/javascript,' + text);
  };

  const onRestore = async () => {
    const module = await loadModule(pathListDef);
    data.value = new Dashboard(module.default);
    onSend(KEYS.DEVICE);
  };

  const onSaveModule = async () => {
    const content = data?.value?.saveList();
    if (content) {
      await uploadBinary(pathList, content);
    }
  };

  const onSaveDef = () => {
    const device = main.value.device;
    onSend(KEYS.DEVICE, { ...device, command: 4 });
  };

  const onSend: TypeSend = (key, object) => {
    webSocketStore.onSend(key, object);
  };

  watch(
    () => main.value,
    (main) => {
      data.value?.setData(main);
    }
  );

  const initModule = async (path: string) => {
    try {
      let module: { default: any };
      module = await loadModule(path);
      return module;
    } catch (error) {
      console.warn(error);
      return;
    }
  };

  onMounted(async () => {
    let module: { default: any } | undefined;

    module = await initModule(`/fs?file=${pathList}`);
    if (!module) {
      module = await initModule(pathListDef);
    }
    data.value = new Dashboard(module?.default);
    webSocketStore.onSend(KEYS.DEVICE);
  });

  return { main, data, getList, getState, setState, onSaveModule, onRemoveItem, onEditItem, onRestore, onSaveDef, onSend };
};
