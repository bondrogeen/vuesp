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

    <div class="grid grid-cols-[repeat(auto-fill,_minmax(130px,_1fr))] gap-4">
      <div v-for="item of items" :key="item.id">
        <component :is="getComponent(item)" v-bind="getBind(item)" @setState="setState(item, $event)" @edit="onDialog(item)"></component>
      </div>
    </div>

    <app-dialog v-if="dialogItem" size="md" :title="isNew ? 'Add item' : 'Edit item'" @close="dialogItem = false">
      <BlockItemEdit :item="item" :isNew="isNew" @save="onSave" @remove="onRemove"></BlockItemEdit>
    </app-dialog>

    <app-dialog v-if="dialogObject" size="md" title="Edit item" @close="dialogObject = false">
      <div class="relative min-h-[200px] max-h-[400px] overflow-auto scrollbar">
        <VListObject :items="main" @click="onSelectId"></VListObject>
      </div>
    </app-dialog>
  </div>
</template>

<script setup lang="ts">
import type { IListItem, IDashboardItem, IMyIStateMain } from '@/types';
import type { Ref } from 'vue';

import { onMounted, ref } from 'vue';

import { KEYS } from '@/types';

import { VListObject } from 'vuesp-components';

import { useFetch } from 'vuesp-components/helpers';
import { pathListDef, pathList } from '@/utils/const.ts';

import { useConnection } from '@/composables/useConnection.js';

import { COMMAND } from '@/utils/gpio';

const dialogItem = ref(false);
const dialogObject = ref(false);
const isNew = ref(false);

const { main, onSend } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
  send(KEYS.DEVICE);
});

const items: Ref<IDashboardItem[]> = ref([]);

const setState = (item: IDashboardItem, value: unknown) => {
  const [_, ...args] = getParamsData(item, main.value);
  let data = null;
  const setFunc = getFunction(item.set);
  if (setFunc) data = setFunc(value, ...args);
  const key = data.key;
  if (key && data) onSend(key, data);
};

// ++++++++++++++++++++++++



const onRestore = async () => {
  // const module = await loadModule(pathListDef);
  // items.value = new Dashboard(module.default);
  // onSend(KEYS.DEVICE);
};

const onSaveModule = async () => {};

const onSaveDef = () => {
  // const device = main.value.device;
  // onSend(KEYS.DEVICE, { ...device, command: 4 });
};

const loadList = async (path: string) => {
  try {
    const res = await useFetch.get(path);
    if (!res.ok) return null;
    return await res.json();
  } catch (error) {
    console.log(error);
    return null;
  }
};

onMounted(async () => {
  let data = await loadList(`/fs?file=${pathList}`);
  if (!data) {
    data = await loadList(pathListDef);
  }
  items.value = data;
});

const item: Ref<IDashboardItem | null> = ref(null);

const listMenu: IListItem[] = [
  { id: 1, name: 'Add item' },
  { id: 2, name: 'Restore items' },
  { id: 3, name: 'Save items' },
  { id: 4, name: 'Save default state' },
];

const onCreate = () => {
  dialogObject.value = true;
  isNew.value = true;
  // item.value = { id: 'device.test.1', name: 'device.test', type: 'info', icon: 'light' };
};

const onMenuEvent = ({ id }: IListItem) => {
  if (id === 1) onCreate();
  if (id === 2) onRestore();
  if (id === 3) onSaveModule();
  if (id === 4) onSaveDef();
};

const getComponent = ({ type = 'info' }) => `card-${type}`;

const onDialog = (data: IDashboardItem) => {
  // item.value = functionToString({ ...data });
  console.log(data);

  dialogItem.value = true;
  isNew.value = false;
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

const onSelectId = (id: string) => {
  console.log(id);
  // item.value.id = id;
  // item.value.keyValue = id;
  // item.value.name = id;
  // item.value.get = '(value) => value';

  dialogObject.value = false;
  dialogItem.value = true;
};
</script>
