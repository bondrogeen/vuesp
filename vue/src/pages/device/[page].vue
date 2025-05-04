<template>
  <div>
    <div class="mb-4 flex items-center justify-between">
      <h1 class="first-letter:uppercase text-gray-800 font-semibold dark:text-white/90">{{ page }}</h1>

      <!-- <div x-data="{openDropDown: false}" class="relative">
        <button
          :class="openDropDown ? 'text-gray-700 dark:text-white' : 'text-gray-400 hover:text-gray-700 dark:hover:text-white'"
          class="text-gray-400 hover:text-gray-700 dark:hover:text-white"
          @click="openDropDown = !openDropDown"
        >
          <svg class="fill-current" width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
            <path
              fill-rule="evenodd"
              clip-rule="evenodd"
              d="M10.2441 6C10.2441 5.0335 11.0276 4.25 11.9941 4.25H12.0041C12.9706 4.25 13.7541 5.0335 13.7541 6C13.7541 6.9665 12.9706 7.75 12.0041 7.75H11.9941C11.0276 7.75 10.2441 6.9665 10.2441 6ZM10.2441 18C10.2441 17.0335 11.0276 16.25 11.9941 16.25H12.0041C12.9706 16.25 13.7541 17.0335 13.7541 18C13.7541 18.9665 12.9706 19.75 12.0041 19.75H11.9941C11.0276 19.75 10.2441 18.9665 10.2441 18ZM11.9941 10.25C11.0276 10.25 10.2441 11.0335 10.2441 12C10.2441 12.9665 11.0276 13.75 11.9941 13.75H12.0041C12.9706 13.75 13.7541 12.9665 13.7541 12C13.7541 11.0335 12.9706 10.25 12.0041 10.25H11.9941Z"
              fill=""
            ></path>
          </svg>
        </button>
      </div> -->
    </div>

    <ServiceSettings v-if="isPage('settings')" v-model="settings" :scan-list="scanList" @save="onSave" @scan="onScan" />

    <ServiceStorage v-if="isPage('storage')" v-model="path" :files="fileList" :progress="progress" :info="info" @send="onSend" />

    <ServiceService v-if="isPage('service')" @reboot="onSureReboot" @reset="onSureReset" />

    <ServiceGPIO v-if="isPage('gpio')" :gpio="gpio" @send="onSend" @reboot="onSureReboot" />
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

const { fileList, info, path, progress, settings, scanList, gpio } = storeToRefs(webSocketStore);

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
