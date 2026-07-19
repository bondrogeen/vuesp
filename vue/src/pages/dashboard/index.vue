<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>{{ $t('menu.dashboard') }}</h1>

      <v-select :items="listMenu" @change="onMenu">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
          </v-button>
        </template>
      </v-select>
    </div>

    <VDragDrop :items="dashboard" class="grid grid-cols-[repeat(auto-fill,_minmax(130px,_1fr))] gap-4" @update:items="dashboard = $event">
      <template #default="{ item }">
        <component :is="getComponent(item)" v-bind="getState(item)" @setState="setState(item, $event)" @edit="onDialog(getState(item))" @update="onUpdate">
          <template #icon="{ icon, value }">
            <div class="size-full" :class="value ? 'animate-shake' : ''">
              <icon-ri-shut-down-line v-if="icon === 'shut'" class="size-full" />

              <icon-ri-plant-line v-if="icon === 'plant'" class="size-full" />
              
              <VIcons :name="icon" />
            </div>
          </template>
        </component>
      </template>
    </VDragDrop>

    <v-dialog v-if="dialogItem" size="md" :title="item?.id ? 'Edit' : $t('add')" @close="dialogItem = false">
      <item-edit class="min-h-[330px]" :item="item" :object="main" :listDashboard="listDashboard" :listIcons="listIcons" @button="onButton">
        <template #icon="{icon}">
          <div class="size-full">
            <icon-ri-shut-down-line v-if="icon === 'shut'" class="size-full" />

            <icon-ri-plant-line v-if="icon === 'plant'" class="size-full" />

            <VIcons :name="icon" />
          </div>
        </template>

        <template #default="{ item }">
          <component :is="getComponent(item)" v-bind="getState(item)" @setState="setState(item, $event)"></component>
        </template>
      </item-edit>
    </v-dialog>
  </div>
</template>

<script setup lang="ts">
import type { IListItem, IDashboardItem } from '@/types';
import type { Ref } from 'vue';

import { ref } from 'vue';
import { setStateItem, getStateItem, dashboardCards } from 'vuesp-components/dashboard';
import { useFetch } from '@vueuse/core';

import { KEYS } from '@/types';
import { COMMAND } from '@/utils/gpio';

import { useConnection } from '@/composables/useConnection';
import { pathListDef } from '@/utils/const';

import { VDragDrop } from 'vuesp-components';

import { useLocale } from '@/composables/useLocale';

import CardChart from '@/components/dashboard/CardChart.vue';
import VIcons from '@/components/ui/icons/VIcons.vue';

const { $t } = useLocale();

const { dashboard, main, onSend } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
  send(KEYS.DEVICE);
  send(KEYS.BUFFER);
});

const dialogItem = ref(false);
const item: Ref<IDashboardItem | null> = ref(null);

const getComponent = ({ type = 'info' }) => {
  if (type === 'chart') {
    return CardChart;
  }
  return `card-${type}`;
};

const onUpdate = () => {
  onSend(KEYS.BUFFER);
};

const onButton = (key: string, item: IDashboardItem) => {
  if (key === 'add') dashboard.value.push(item);
  if (key === 'save') dashboard.value = dashboard.value.map((i) => (i.id === item.id ? item : i));
  if (key === 'remove') dashboard.value = dashboard.value.filter((i) => i.id !== item.id);
  dialogItem.value = false;
};

const listMenu: IListItem[] = [
  { name: $t('add'), value: 1 },
  { name: $t('save'), value: 2 },
  { name: $t('restore'), value: 3 },
  { name: $t('default'), value: 4 },
];

const onCreate = () => {
  item.value = null;
  dialogItem.value = true;
};
const onRestore = async () => {
  const { data } = await useFetch(pathListDef).json();
  dashboard.value = data.value.dashboard;
};

const onSave = async () => {
  const body = new FormData();
  body.append('file[0]', new Blob([JSON.stringify(dashboard.value)], { type: 'application/json' }), '/tmp/dashboard.json');
  return await useFetch('/fs', { body }).post();
};

const onDefault = () => {
  onSend(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_SAVE });
  onSend(KEYS.DEVICE, { command: 254 });
};

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

const icons = [
  'bulb',
  'video',
  'windy',
  'camera',
  'light',
  'tv',
  'therm',
  'store',
  'socket',
  'plug',
  'noti',
  'lock',
  'chart',
  'battery',
  'ram',
  'map',
  'speaker',
  'timer',
  'snow',
  'blaze',
  'door',
  'shut',
  'plant',
];

const cards = [...(dashboardCards || []), 'chart'];

const listIcons: IListItem[] = icons.map((item, i) => ({ id: ++i, name: item, value: item }));
const listDashboard: IListItem[] = cards.map((item, i) => ({ id: ++i, name: item, value: item }));
</script>
