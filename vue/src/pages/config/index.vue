<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Config</h1>
      <VDropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <IconMenu @click="on.click"></IconMenu>
        </template>

        <VList :list="listMenu" class="rounded" @click="onUploadFile"></VList>
      </VDropdown>
    </div>

    <div class="grid grid-cols-1 gap-4">
      <div v-for="(item, key) in config" :key="key" label="key">
        <h3 class="mb-6 first-letter:uppercase">{{ key }}</h3>

        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-6 gap-4">
          <div v-for="(field, k) in item" :key="`${k}`" class="flex flex-col gap-2">
            <VTextField :model-value="getValue(field, k)" label="Name" @blur="onName(key, k, $event.target.value)" />

            <VTextarea v-if="isFunction(field)" :model-value="getFun(field)" label="Value" @blur="onFunction(key, k, $event.target.value)" />
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

import AppDialog from '@/components/app/AppDialog.vue';

const webSocketStore = useWebSocketStore();
const { dallas } = storeToRefs(webSocketStore);

const showDialog = ref(false);
const config = ref({});

const listMenu = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Remove' },
];

const getFun = (field) => field?.fun || '(v)=>v*10';

// console.log(eval(text)(15));

const onSave = async () => {
  await saveConfig(config.value);
};
const onClose = () => {
  showDialog.value = false;
};
const getValue = (field, k) => field?.name || k;
const isFunction = (field) => typeof field?.fun !== 'undefined';

const onName = (name, key, value) => (config.value[name][key].name = value);
const onFunction = (name, key, value) => (config.value[name][key].fun = value);

onMounted(async () => {
  const object = dallas?.value || {};
  const ds = {};
  for (const key in object) {
    ds[key] = {};
  }

  config.value = await getConfig({ ds });
});
</script>
