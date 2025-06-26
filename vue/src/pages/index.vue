<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Home</h1>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-dots class="rotate-90"></icon-dots>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenuEvent"></v-list>
      </v-dropdown>
    </div>

    <div class="grid grid-cols-[repeat(auto-fill,_minmax(120px,_1fr))] gap-4">
      <div v-for="(item, i) of getList" :key="item.id" :class="i === 2 ? '' : ''">
        <component :is="getComponent(item)" v-bind="item" :value="getState(item.id)" @setState="setStateValue" @edit="onDialog(item)"></component>
      </div>
    </div>

    <AppDialog v-if="dialogItem" size="md" :title="isNew ? 'Add item' : 'Edit item'" @close="dialogItem = false">
      <BlockItemEdit :item="item" :isNew="isNew" @save="onSave" @remove="onRemove"></BlockItemEdit>
    </AppDialog>

    <AppDialog v-if="dialogObject" size="md" title="Edit item" @close="dialogObject = false">
      <div class="relative min-h-[200px] max-h-[400px] overflow-auto scrollbar">
        <VListObject :items="main" @click="onSelectId"></VListObject>
      </div>
    </AppDialog>
  </div>
</template>

<script setup lang="ts">
import type { TypeProperty, TypePropertyString } from '@/vuesp-data/types.ts';
import type { Ref } from 'vue';

import { ref } from 'vue';

import { useModule } from '@/composables/useModule.ts';

import { functionToString } from '@/vuesp-data/utils.ts';

import VListObject from '@/components/general/VListObject.vue';

const dialogItem = ref(false);
const dialogObject = ref(false);
const isNew = ref(false);

const item: Ref<TypePropertyString> = ref({ id: '', name: '', keyValue: '' });

interface TypeList {
  id: number;
  name: string;
}

const listMenu: TypeList[] = [
  { id: 1, name: 'Add item' },
  { id: 2, name: 'Restore items' },
  { id: 3, name: 'Save items' },
  { id: 2, name: 'Default state' },
];

const { main, getList, getState, setState, onSaveModule, onRemoveItem, onEditItem, onRestore, onSaveDef } = useModule();

const onMenuEvent = async ({ id }: TypeList) => {
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

const setStateValue = ({ id, value }: TypeProperty) => {
  setState(id, value);
};

const onDialog = (data: TypeProperty) => {
  item.value = functionToString({ ...data });
  dialogItem.value = true;
  isNew.value = false;
};

const onSave = (item: TypePropertyString) => {
  onEditItem(item);
  dialogItem.value = false;
};

const onRemove = (item: TypePropertyString) => {
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
