<template>
  <div class="page-config container mx-auto">
    <div class="row">
      <div class="col sm12 mb-6 flex items-center">
        <h1 class="text-h2">Config</h1>
        <div class="v-spacer"></div>
        <v-dropdown right="0" left="unset" top="0">
          <template #activator="{ on }">
            <v-icons icon="menu" @click="on.click"></v-icons>
          </template>
          <v-list :list="listMenu" @click="onUploadFile"></v-list>
        </v-dropdown>
      </div>
    </div>
    {{ config }}
    <div class="row">
      <div v-for="(item, key) in config" :key="key" label="key" class="col sm12">
        <h2 class="text-h5 mb-6">{{ key }}</h2>

        <div class="row">
          <div v-for="(field, k) in item" :key="`${k}`" class="col sm12 md6">
            <v-input :model-value="getValue(field, k)" :label="`Name`" @update:model-value="onInput(key, k, $event)" />
          </div>
        </div>
      </div>
      <div class="col sm12">
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

<style lang="scss">
.page-main {
  position: relative;
  &__list {
    height: 400px;
    overflow: auto;
  }
  &__item {
    border-bottom: 1px solid var(--border-1);
  }
  &__time {
    min-width: 150px;
  }
  &__date {
    font-size: 16px;
    height: 32px;
  }
  &__card {
    border: 1px solid, var(--primary);
    border-radius: 16px;
  }
}
</style>
