<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Config</h1>
      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-icons icon="menu" @click="on.click"></v-icons>
        </template>
        <v-list :list="listMenu" @click="onUploadFile"></v-list>
      </v-dropdown>
    </div>

    <div class="grid grid-cols-1 gap-4">
      <div v-for="(item, key) in config" :key="key" label="key" class="">
        <h3 class="mb-6">{{ key }}</h3>

        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
          <div v-for="(field, k) in item" :key="`${k}`">
            <VTextField :model-value="getValue(field, k)" :label="`Name`" @update:model-value="onInput(key, k, $event)" />
          </div>
        </div>
      </div>
      <div>
        <v-button @click="onSave">Save</v-button>
      </div>
    </div>

    <AppDialog size="lg" title="Config" :value="showDialog" @close="onClose">
      <template #footer>
        <v-button @click="onSubmit(true)">Scan</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup>
import { onMounted, ref } from 'vue';
import { storeToRefs } from 'pinia';
import { getConfig, saveConfig } from '@/utils/fs/';

import { useWebSocketStore } from '@/stores/WebSocketStore';

import AppDialog from '@/components/app/AppDialog';
import VTextField from '@/components/general/VTextField';

const webSocketStore = useWebSocketStore();
const { dallas } = storeToRefs(webSocketStore);

const showDialog = ref(false);
const config = ref({});

const listMenu = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Remove' },
];

const onSave = async e => {
  await saveConfig(config.value);
};
const onClose = e => {
  showDialog.value = false;
};
const getValue = (field, k) => {
  return typeof field.name === 'undefined' ? k : '';
};

const onInput = (name, key, value) => {
  config.value[name][key].name = value;
  console.log(name, key, value);
};

onMounted(async () => {
  const object = dallas?.value || {};
  const ds = {};
  for (const key in object) {
    ds[key] = {};
  }

  config.value = await getConfig({ ds });
});
</script>
