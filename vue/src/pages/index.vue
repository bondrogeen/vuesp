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

    <div class="grid grid-cols-[repeat(auto-fit,_minmax(120px,_1fr))] gap-4">
      <div v-for="(item, i) of getList" :key="item.id" :class="i === 2 ? '' : ''">
        <component :is="getComponent(item)" v-bind="item" :value="getState(item.id)" @setState="setStateValue" @edit="onDialog(item)"></component>
      </div>
    </div>

    <AppDialog v-if="dialogItem" size="md" title="Edit item" @close="dialogItem = false">
      <div class="mt-6">
        <v-text-field v-model="item.id" label="id" :disabled="!isNew" @click="dialogObject = true"></v-text-field>
        <v-text-field v-model="item.name" label="name"></v-text-field>
        <v-text-field v-model="item.key" label="key"></v-text-field>
        <v-text-field v-if="item.set" v-model="item.set" label="set"></v-text-field>
        <v-text-field v-model="item.get" label="get"></v-text-field>
      </div>

      <template #footer>
        <div class="flex gap-4">
          <v-button class="px-8" color="red" @click="onRemove">Remove</v-button>
          <v-button class="px-8" color="gry" @click="onSave">Cancel</v-button>
          <v-button class="px-8" color="blue" @click="onSave">Save</v-button>
        </div>
      </template>
    </AppDialog>

    <AppDialog v-if="dialogObject" size="md" title="Edit item" @close="dialogObject = false">
      <div class="relative min-h-[200px] max-h-[400px] overflow-auto scrollbar">
        <VListObject :items="main" @click="onSelectId"></VListObject>
      </div>
    </AppDialog>
  </div>
</template>

<script setup lang="ts">
import type { TypeProperty } from '@/utils/VuespData.ts';
import type { Ref } from 'vue';

import { ref } from 'vue';
import AppDialog from '@/components/app/AppDialog.vue';

import { useModule } from '@/composables/useModule.ts';

import VListObject from '@/components/general/VListObject.vue';
import CardButton from '@/components/dashboard/CardButton.vue';
import CardInfo from '@/components/dashboard/CardInfo.vue';
import CardSwitch from '@/components/dashboard/CardSwitch.vue';
import CardDimmer from '@/components/dashboard/CardDimmer.vue';

const item: Ref<TypePropertyString> = ref({ id: 'device.test.1', name: 'device.test', key: 'device.test' });

const dialogItem = ref(false);
const dialogObject = ref(false);
const isNew = ref(false);

interface TypeList {
  id?: number;
  name: string;
  key?: string;
  className?: string;
}

const listMenu: TypeList[] = [
  { id: 1, name: 'Add' },
  { id: 2, name: 'Restore' },
  { id: 3, name: 'Save' },
  { id: 2, name: 'Save default' },
];
const listMenuItem: TypeList[] = [
  { id: 1, name: 'Edit' },
  { id: 2, name: 'Remove' },
];

const { main, getList, getState, setState, onSaveModule, onRemoveItem, onEditItem, onRestore, onSaveDef } = useModule();

const onMenuEvent = async ({ id }: TypeList) => {
  if (id === 1) {
    dialogObject.value = true;
    isNew.value = true;
    item.value = { id: 'device.test.1', name: 'device.test', key: 'device.test' };
  }
  if (id === 2) onRestore();
  if (id === 3) onSaveModule();
  if (id === 4) onSaveDef();
};

const getComponent = ({ type = 'info' }) => {
  const components: { [key: string]: any } = {
    button: CardButton,
    switch: CardSwitch,
    dimmer: CardDimmer,
    info: CardInfo,
  };
  return components?.[type] || CardInfo;
};

const setStateValue = ({ id, value, type }) => {
  console.log(item);
  if (type === 'button') {
    setState(id, value ? 0 : 1, { command: 2 });
  }
  if (type === 'dimmer') {
    setState(id, value, { command: 3 });
  }
};
const onDialog = (data: TypeProperty) => {
  item.value = data;
  dialogItem.value = true;
  isNew.value = false;
};

const onSave = () => {
  onEditItem(item.value);
  dialogItem.value = false;
};
const onRemove = () => {
  onRemoveItem(item.value.id);
  dialogItem.value = false;
};

const onSelectId = (id: string) => {
  item.value.id = id;
  item.value.key = id;
  item.value.name = id;
  item.value.get = '(value) => value';

  dialogObject.value = false;
  dialogItem.value = true;
};
</script>
