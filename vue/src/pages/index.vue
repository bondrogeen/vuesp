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
      <div v-for="(item, i) of getList" :key="item.id" :class="i === 2 ? '' : ''">
        <component :is="getComponent(item)" v-bind="item" :value="getState(item.id)" @setState="setStateValue" @edit="onDialog(item)"></component>
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
import type { IDashboardItem, IDashboardItemString } from 'vuesp-components/types';
import type { Ref } from 'vue';

import { ref } from 'vue';

import { useModule } from '@/composables/useModule.ts';

import { functionToString } from 'vuesp-components/dashboard';

import { VListObject } from 'vuesp-components';

const dialogItem = ref(false);
const dialogObject = ref(false);
const isNew = ref(false);

const item: Ref<IDashboardItemString> = ref({ id: '', name: '', keyValue: '' });

interface IListMenu {
  id: number;
  name: string;
}

const listMenu: IListMenu[] = [
  { id: 1, name: 'Add item' },
  { id: 2, name: 'Restore items' },
  { id: 3, name: 'Save items' },
  { id: 4, name: 'Save default state' },
];

const { main, getList, getState, setState, onSaveModule, onRemoveItem, onEditItem, onRestore, onSaveDef } = useModule();

const onMenuEvent = ({ id }: IListMenu) => {
  if (id === 1) {
    dialogObject.value = true;
    isNew.value = true;
    item.value = { id: 'device.test.1', name: 'device.test', keyValue: 'device.test', type: 'info', icon: 'light' };
  }
  if (id === 2) onRestore();
  if (id === 3) onSaveModule();
  if (id === 4) onSaveDef();
};

const getComponent = ({ type = 'info' }) => `card-${type}`;

const setStateValue = ({ id, value }: IDashboardItem) => {
  setState(id, value);
};

const onDialog = (data: IDashboardItem) => {
  item.value = functionToString({ ...data });
  dialogItem.value = true;
  isNew.value = false;
};

const onSave = (item: IDashboardItemString) => {
  onEditItem(item);
  dialogItem.value = false;
};

const onRemove = (item: IDashboardItemString) => {
  onRemoveItem(item);
  dialogItem.value = false;
};

const onSelectId = (id: string) => {
  item.value.id = id;
  item.value.keyValue = id;
  item.value.name = id;
  item.value.get = '(value) => value';

  dialogObject.value = false;
  dialogItem.value = true;
};
</script>
