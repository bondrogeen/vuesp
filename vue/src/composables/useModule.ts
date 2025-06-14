import type { TypeVuespData, TypePropertyString } from '@/utils/VuespData.ts';
import type { Ref } from 'vue';

import { onMounted, ref, computed, watch } from 'vue';
import { storeToRefs } from 'pinia';

import { loadModule, saveModule } from '@/utils/fs.ts';
import { VuespData } from '@/utils/VuespData.ts';
import { pathListDef, pathList } from '@/utils/const.ts';

import { useWebSocketStore } from '@/stores/WebSocketStore.ts';

export const useModule = () => {
  const webSocketStore = useWebSocketStore();
  const { dallas, device } = storeToRefs(webSocketStore);

  const data: Ref<TypeVuespData | null> = ref(null);

  const getList = computed(() => data.value?.getList?.());

  const setState = (id: string, value: number, command: number) => {
    data.value?.set(id, value);
    const { device } = data.value?.getData();
    onSend('DEVICE', { ...device, command });
  };

  const getState = (id: string) => {
    return data.value?.get(id);
  };

  const onEditItem = (item: TypePropertyString) => {
    data.value?.editItem(item.id, { ...item });
  };

  const onRemoveItem = (id: string) => {
    data.value?.removeItem(id);
  };

  const onRestore = async () => {
    const module = await loadModule(pathListDef);
    data.value = new VuespData(module.default);
    onSend('DEVICE');
  };

  const onSaveModule = async () => {
    const content = data?.value?.saveList();
    if (content) {
      await saveModule(pathList, content);
    }
  };

  const onSaveDef = () => {
    onSend('DEVICE', { ...device.value, command: 4 });
  };

  const onSend = (key: string, data?: any) => {
    webSocketStore.onSend(key, data);
  };

  watch(
    () => [dallas.value, device.value],
    ([dallas, device]) => {
      data.value?.setData({ dallas, device });
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
    data.value = new VuespData(module?.default);
    webSocketStore.onSend('DEVICE');

    setTimeout(() => {
      console.log(dallas.value);
      
      console.log(data.value?.getPaths());
    }, 1000);
  });

  return { data, getList, getState, setState, onSaveModule, onRemoveItem, onEditItem, onRestore, onSaveDef, onSend };
};
