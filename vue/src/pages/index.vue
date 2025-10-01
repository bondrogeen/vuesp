<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Home</h1>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <v-icons name="Dots" class="rotate-90"></v-icons>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenu"></v-list>
      </v-dropdown>
    </div>

    <VDragDrop :items="items" class="grid grid-cols-[repeat(auto-fill,_minmax(130px,_1fr))] gap-4" @update:items="items = $event">
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
import type { IDashboardItem } from '@/types';

import { KEYS } from '@/types';
import { COMMAND } from '@/utils/gpio';

import { setStateItem, getStateItem } from 'vuesp-components/dashboard';

import { useConnection } from '@/composables/useConnection';
import { useDashboard } from '@/composables/useDashboard';

import { VDragDrop } from 'vuesp-components';

const { main, onSend } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
  send(KEYS.DEVICE);
});

const { item, items, dialogItem, listMenu, getComponent, onMenu, onDialog, onButton } = useDashboard();

const getState = (item: IDashboardItem) => getStateItem(item, main.value);

const setState = (item: IDashboardItem, value: unknown) => {
  const data = setStateItem(item, value, main.value);
  const key = data.key;
  if (key && data) onSend(key, data);
};
</script>
