<template>
  <div>
    <div class="mb-4 flex items-center justify-between">
      <h1>{{ page }}</h1>

      <div data-slot="device"></div>
    </div>

    <ServiceSettings v-if="isPage('settings')" v-model="settings" :scan-list="scanList" @save="onSave" @scan="onScan" />

    <ServiceStorage v-if="isPage('storage')" v-model="path" :files="fileList" :info="info" @send="onSend" />

    <ServiceService v-if="isPage('service')" @reboot="onSureReboot" @reset="onSureReset" />

    <ServiceGPIO v-if="isPage('gpio')" :gpio="gpio" @reboot="onSureReboot" @send="onSend" />
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, inject, nextTick } from 'vue';
import { storeToRefs } from 'pinia';
import event from '@/assets/js/event';

import ServiceGPIO from '@/components/pages/service/ServiceGPIO.vue';
import ServiceStorage from '@/components/pages/service/ServiceStorage.vue';
import ServiceSettings from '@/components/pages/service/ServiceSettings.vue';

import ServiceService from '@/components/pages/service/ServiceService.vue';

import type { TypeStateSettings } from '@/types/types.ts';

import { useWebSocketStore } from '@/stores/WebSocketStore.ts';
import { useRoute } from 'vue-router';

import { DialogKey, NotificationKey } from '@/simbol/index.ts';

const webSocketStore = useWebSocketStore();

const { fileList, info, path, settings, scanList, gpio } = storeToRefs(webSocketStore);

const route = useRoute();

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

const onSave = (settings: TypeStateSettings) => {
  webSocketStore.onSend('SETTINGS', settings);
  dialog({ value: true, title: 'Done', message: 'Do you want to restart your device?', callback: onReboot });
};

const onScan = (value: boolean) => {
  if (value) scanList.value = [];
  if (!scanList.value.length) webSocketStore.onSend('SCAN');
};

event.on('init', () => {
  if (!settings?.value?.key) webSocketStore.onSend('SETTINGS');
});

onMounted(() => {
  webSocketStore.onSend('SETTINGS');
  webSocketStore.onSend('INFO');
});
</script>
