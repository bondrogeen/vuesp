<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Home</h1>

      <v-dropdown right="0" left="unset" top="0" hideOnClick>
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <v-icons name="Dots" class="rotate-90"></v-icons>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenu"></v-list>
      </v-dropdown>
    </div>

    <VDragDrop :items="dashboard" class="grid grid-cols-[repeat(auto-fill,_minmax(130px,_1fr))] gap-4" @update:items="dashboard = $event">
      <template #default="{ item }">
        <component :is="getComponent(item)" v-bind="getState(item)" @setState="setState(item, $event)" @edit="onDialog(getState(item))"></component>
      </template>
    </VDragDrop>

    <app-dialog v-if="dialogItem" size="md" :title="item?.id ? 'Edit' : 'Add'" @close="dialogItem = false">
      <item-edit class="min-h-[330px]" :item="item" :object="main" @button="onButton">
        <template #default="{ item }">
          <component :is="getComponent(item)" v-bind="getState(item)" @setState="setState(item, $event)"></component>
        </template>
      </item-edit>
    </app-dialog>
  </div>
</template>

<script setup lang="ts">
import type { IListItem, IDashboardItem } from '@/types';
import type { Ref } from 'vue';

import { ref } from 'vue';
import { setStateItem, getStateItem } from 'vuesp-components/dashboard';
import { useFetch, uploadJson } from 'vuesp-components/helpers';

import { KEYS } from '@/types';
import { COMMAND } from '@/utils/gpio';

import { useConnection } from '@/composables/useConnection';
import { pathListDef } from '@/utils/const';

import { VDragDrop } from 'vuesp-components';

const { dashboard, main, onSend } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
  send(KEYS.DEVICE);
});

const dialogItem = ref(false);
const item: Ref<IDashboardItem | null> = ref(null);

const getComponent = ({ type = 'info' }) => `card-${type}`;

const onButton = (key: string, item: IDashboardItem) => {
  if (key === 'add') dashboard.value.push(item);
  if (key === 'save') dashboard.value = dashboard.value.map((i) => (i.id === item.id ? item : i));
  if (key === 'remove') dashboard.value = dashboard.value.filter((i) => i.id !== item.id);
  dialogItem.value = false;
};

const listMenu: IListItem[] = [
  { name: 'Add', value: 1 },
  { name: 'Save', value: 2 },
  { name: 'Restore', value: 3 },
  { name: 'Default', value: 4 },
];

const onCreate = () => {
  item.value = null;
  dialogItem.value = true;
};
const onRestore = async () => {
  const res = await useFetch.$get(pathListDef);
  dashboard.value = res.dashboard;
};
const onSave = async () => await uploadJson('/tmp/dashboard.json', dashboard.value);
const onDefault = () => onSend(KEYS.DEVICE, { command: 254 });

const onMenu = ({ value }: IListItem) => {
  if (value === 1) onCreate();
  if (value === 2) onSave();
  if (value === 3) onRestore();
  if (value === 4) onDefault();
};

const onDialog = (data: IDashboardItem) => {
  item.value = data;
  dialogItem.value = true;
};

const getState = (item: IDashboardItem) => getStateItem(item, main.value);

const setState = (item: IDashboardItem, value: unknown) => {
  const data = setStateItem(item, value, main.value);
  if (Array.isArray(data)) {
    data.forEach((el) => {
      const key = el?.key;
      if (key && el) onSend(key, el);
    });
    return;
  }
  const key = data.key;
  if (key && data) onSend(key, data);
};
</script>
