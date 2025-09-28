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

        <v-list :list="listMenu" @click="onMenu"></v-list>
      </v-dropdown>
    </div>

    <VDragDrop :items="items" class="grid grid-cols-[repeat(auto-fill,_minmax(130px,_1fr))] gap-4" @update:items="items = $event">
      <template #default="{ item }">
        <component :is="getComponent(item)" v-bind="getBind(item)" @setState="setState(item, $event)" @edit="onDialog(item)"></component>
      </template>
    </VDragDrop>

    <app-dialog v-if="dialogItem" size="md" :title="item?.id ? 'Edit item' : 'Add item'" @close="dialogItem = false">
      <BlockItemEdit class="min-h-[330px]" :item="item" :object="main" @button="onButton">
        <template #default="{ item }">
          <component :is="getComponent(item)" v-bind="getBind(item)" @setState="setState(item, $event)"></component>
        </template>
      </BlockItemEdit>
    </app-dialog>
  </div>
</template>

<script setup lang="ts">
import type { IDashboardItem } from '@/types';

import { KEYS } from '@/types';

import { VDragDrop } from 'vuesp-components';

import { getParamsData, getFunction } from 'vuesp-components/dashboard';

import { useConnection } from '@/composables/useConnection';
import { useDashboard } from '@/composables/useDashboard';

import { COMMAND } from '@/utils/gpio';

const { main, onSend } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
  send(KEYS.DEVICE);
});

const { item, items, dialogItem, listMenu, onMenu, onDialog, onButton } = useDashboard();

const getComponent = ({ type = 'info' }) => `card-${type}`;

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
</script>
