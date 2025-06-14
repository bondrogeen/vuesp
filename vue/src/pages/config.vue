<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Config</h1>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-dots class="rotate-90"></icon-dots>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenuEvent"></v-list>
      </v-dropdown>
    </div>

    <div class="h-[calc(100vh-200px)] overflow-auto scrollbar">
      <VTable :headers="headers" :items="getList">
        <template #name="{ item }">
          <div>
            <div class="text-gray-900 dark:text-white">{{ item.name }}</div>

            <div class="text-sm">{{ item.id }}</div>
          </div>
        </template>

        <template #value="{ item }">
          <div v-if="item.id.includes('input')">
            <v-button class="min-w-16" color="gray" disabled>{{ getState(item.id) ? 'OFF' : 'ON' }}</v-button>
          </div>

          <div v-else-if="item.id.includes('output')">
            <v-button class="min-w-16" color="gray" @click="setState(item.id, getState(item.id) ? 0 : 1, 2)">{{ getState(item.id) ? 'OFF' : 'ON' }}</v-button>
          </div>
          <div v-else-if="item.id.includes('dac')">
            <v-text-field :modelValue="getState(item.id)" class="max-w-20 0" hideMessage :label="item.name" @blur="setState(item.id, $event.target.value, 3)"></v-text-field>
          </div>
          <span v-else>{{ getState(item.id) }}</span>
        </template>

        <template #action="{ item }">
          <div class="flex justify-center flex-auto">
            <VDropdown left="unset" right="0" top="0">
              <template #activator="{ on }">
                <VButton type="icon" color="gray" class="flex" @click.stop="on.click()">
                  <IconDots />
                </VButton>
              </template>

              <VList :list="listMenuItem" @click="onMenuItemEvent($event, item)" />
            </VDropdown>
          </div>
        </template>
      </VTable>
    </div>

    <AppDialog v-if="dialogItem" size="lg" title="Edit item" @close="onClose">
      <div class="mt-6">
        <v-text-field v-model="item.id" label="id" disabled></v-text-field>
        <v-text-field v-model="item.name" label="name"></v-text-field>
        <v-text-field v-model="item.key" label="key"></v-text-field>
        <v-text-field v-if="item.set" v-model="item.set" label="set"></v-text-field>
        <v-text-field v-model="item.get" label="get"></v-text-field>
      </div>

      <template #footer>
        <div class="flex gap-4">
          <v-button class="px-8" color="gry" @click="onSave">Cancel</v-button>
          <v-button class="px-8" color="blue" @click="onSave">Save</v-button>
        </div>
      </template>
    </AppDialog>
  </div>
</template>

<script setup lang="ts">
import type { TypePropertyString } from '@/utils/VuespData.ts';
import type { Ref } from 'vue';

import { ref } from 'vue';
import AppDialog from '@/components/app/AppDialog.vue';

import { useModule } from '@/composables/useModule.ts';

const item: Ref<TypePropertyString> = ref({ id: 'device.test.1', name: 'device.test', key: 'device.test' });

const dialogItem = ref(false);

interface TypeList {
  id?: number;
  name: string;
  key?: string;
  className?: string;
}

const headers: TypeList[] = [
  { key: 'name', name: 'Name', className: 'w-40' },
  { key: 'value', name: 'Value', className: '' },
  { key: 'action', name: 'Action', className: 'w-10 text-left' },
];
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

const { getList, getState, setState, onSaveModule, onRemoveItem, onEditItem, onRestore, onSaveDef } = useModule();

const onMenuEvent = async ({ id }: TypeList) => {
  if (id === 1) dialogItem.value = true;
  if (id === 2) onRestore();
  if (id === 3) onSaveModule();
  if (id === 4) onSaveDef();
};

const onMenuItemEvent = ({ id }: TypeList, item: TypePropertyString) => {
  if (id === 1) onDialog(item);
  if (id === 2) onRemoveItem(item.id);
};

const onClose = () => {
  dialogItem.value = false;
};

const onDialog = (data: TypePropertyString) => {
  item.value = data;
  dialogItem.value = true;
};

const onSave = () => {
  onEditItem(item.value);
  dialogItem.value = false;
};
</script>
