<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Home</h1>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <VButton color="" type="icon" @click="on.click">
            <VIcons name="IconDots" class="rotate-90"></VIcons>
          </VButton>
        </template>

        <v-list :list="listMenu" @click="onMenuEvent"></v-list>
      </v-dropdown>
    </div>

    <VJs />

    <VDragDrop v-slot="{ item }" :items="items" class="grid grid-cols-[repeat(auto-fill,_minmax(130px,_1fr))] gap-4" @update:items="items = $event">
      <component :is="getComponent(item)" v-bind="getBind(item)" @setState="setState(item, $event)" @edit="onDialog(item)"></component>
    </VDragDrop>

    <app-dialog v-if="dialogItem" size="md" :title="item?.id ? 'Add item' : 'Edit item'" @close="dialogItem = false">
      <BlockItemEdit v-slot="{ item }" :item="item" :object="main" @save="onSave" @remove="onRemove">
        <component :is="getComponent(item)" v-bind="getBind(item)" @setState="setState(item, $event)"></component>
      </BlockItemEdit>
    </app-dialog>
  </div>
</template>

<script setup lang="ts">
import type { IListItem, IDashboardItem } from '@/types';
import type { Ref } from 'vue';

import { onMounted, ref, toRaw } from 'vue';

import { KEYS } from '@/types';

import { VListObject, VDragDrop } from 'vuesp-components';

import { useFetch } from 'vuesp-components/helpers';
import { pathListDef, pathList } from '@/utils/const';
import { getParamsData, getFunction } from 'vuesp-components/dashboard';

import { useConnection } from '@/composables/useConnection';

import { COMMAND } from '@/utils/gpio';
import VJs from '@/components/VJs.vue';

const dialogItem = ref(false);

const { main, onSend } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
  send(KEYS.DEVICE);
});

const item: Ref<IDashboardItem | null> = ref(null);
const items: Ref<IDashboardItem[]> = ref([]);

const setState = (item: IDashboardItem, value: unknown) => {
  const [_, ...args] = getParamsData(item, main.value);
  let data = null;
  const setFunc = getFunction(item.set);
  if (setFunc) data = setFunc(value, ...args);
  const key = data.key;
  if (key && data) onSend(key, data);
};

const getBind = (item: IDashboardItem) => {
  const args = getParamsData(item, main.value);
  let value = null;
  const getFunc = getFunction(item.get);
  if (getFunc) value = getFunc(...args);
  let convert = null;
  const modifyFunc = getFunction(item.modify);
  if (modifyFunc) convert = modifyFunc(...args);
  return { ...item, value, convert };
};

const onRestore = async () => {};

const onSaveModule = async () => {};

const onSaveDef = () => {};

onMounted(async () => {
  let data = await useFetch.$get(`/fs?file=${pathList}`);
  if (!data) {
    data = await useFetch.$get(pathListDef);
  }
  items.value = data;

  item.value = window.structuredClone(toRaw(data[0]));
  dialogItem.value = true;
});

const listMenu: IListItem[] = [
  { id: 1, name: 'Add', value: 1 },
  { id: 2, name: 'Save', value: 3 },
  { id: 3, name: 'Restore', value: 2 },
  { id: 4, name: 'Default', value: 4 },
];

const onCreate = () => {
  dialogItem.value = true;
};

const onMenuEvent = ({ id }: IListItem) => {
  if (id === 1) onCreate();
  if (id === 2) onSaveModule();
  if (id === 3) onRestore();
  if (id === 4) onSaveDef();
};

const getComponent = ({ type = 'info' }) => `card-${type}`;

const onDialog = (data: IDashboardItem) => {
  item.value = window.structuredClone(toRaw(data));
  console.log(data);

  dialogItem.value = true;
};

const onSave = (item: IDashboardItem) => {
  console.log(item);
  // onEditItem(item);
  dialogItem.value = false;
};

const onRemove = (item: IDashboardItem) => {
  console.log(item);
  // onRemoveItem(item);
  dialogItem.value = false;
};
</script>
