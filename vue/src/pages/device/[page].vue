<template>
  <div>
    <div class="mb-4 flex items-center justify-between">
      <h1>{{ title }}</h1>

      <div data-slot="device"></div>
    </div>

    <template v-if="show">
      <ServiceSettings v-if="isPage('settings')" v-model="settings" :scan-list="scanList" @save="onSave" @scan="onScan" @reboot="onSureReboot" @reset="onSureReset" />

      <ServiceStorage v-if="isPage('storage')" v-model="path" :files="fileList" :info="main.info" @send="onSend" />

      <ServiceGPIO v-if="isPage('gpio')" :gpio="gpio" @reboot="onSureReboot" @send="onSend" />
    </template>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, inject, nextTick } from 'vue';
import { storeToRefs } from 'pinia';

import type { IStateSettings } from '@/utils/types/types.ts';

import { getPageTitle } from '@/utils/helpers';

import { useConnection } from '@/composables/useConnection.js';

import ServiceGPIO from '@/components/pages/service/ServiceGPIO.vue';
import ServiceStorage from '@/components/pages/service/ServiceStorage.vue';
import ServiceSettings from '@/components/pages/service/ServiceSettings.vue';

import { useAppStore } from '@/stores/AppStore.js';
import { useWebSocketStore } from '@/stores/WebSocketStore.ts';

import { useRoute } from 'vue-router';

import { DialogKey, NotificationKey } from '@/utils/types/simbol';

useConnection(() => {
  webSocketStore.onSend('SETTINGS');
  webSocketStore.onSend('INFO');
});

const appStore = useAppStore();
const { menu } = storeToRefs(appStore);

const title = computed(() => getPageTitle(menu.value, route.fullPath).name);

const webSocketStore = useWebSocketStore();
const { fileList, main, path, settings, scanList, gpio } = storeToRefs(webSocketStore);

const route = useRoute();

const show = ref(false);

const page = computed(() => route.params.page);
const isSettings = computed(() => settings.value?.key);

const dialog = inject(DialogKey, ({}) => {});
const notification = inject(NotificationKey, ({}) => {});

const isPage = (value: string) => isSettings.value && page.value === value;

const onReboot = () => {
  webSocketStore.onSend('REBOOT');
  nextTick(() => {
    notification({ text: 'Reboot' });
  });
};

const onReset = () => {
  settings.value.version = Math.floor(Math.random() * 65000);
  nextTick(() => onSave(settings.value));
};

const onSureReboot = () => dialog({ value: true, message: 'Do you want to restart your device?', callback: onReboot });
const onSureReset = () => dialog({ value: true, message: 'The configuration will be reset to default. <br/>Are you sure?', callback: onReset });

const onSend = ({ comm, data }: any) => {
  fileList.value = [];
  webSocketStore.onSend(comm, data);
};

const onSave = (settings: IStateSettings) => {
  webSocketStore.onSend('SETTINGS', settings);
  dialog({ value: true, title: 'Done', message: 'Do you want to restart your device?', callback: onReboot });
};

const onScan = (value: boolean) => {
  if (value) scanList.value = [];
  if (!scanList.value.length) webSocketStore.onSend('SCAN');
};

onMounted(() => {
  setTimeout(() => {
    show.value = true;
  }, 100);
});
</script>
