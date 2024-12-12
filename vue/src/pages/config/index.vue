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
        <h3 class="mb-6">{{ key }}</h3>

        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-6 gap-4">
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
import VDropdown from '@/components/general/VDropdown';
import VList from '@/components/general/VList';

import IconMenu from '@/components/icons/IconMenu';

const webSocketStore = useWebSocketStore();
const { dallas } = storeToRefs(webSocketStore);

const showDialog = ref(false);
const config = ref({});

const listMenu = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Remove' },
];

const onSave = async () => {
  await saveConfig(config.value);
};
const onClose = () => {
  showDialog.value = false;
};
const getValue = (field, k) => {
  return typeof field.name === 'undefined' ? k : '';
};

const onInput = (name, key, value) => {
  config.value[name][key].name = value;
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
