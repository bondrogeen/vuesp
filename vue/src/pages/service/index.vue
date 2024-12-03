<template>
  <div class="container">
    <div class="row">
      <h1 class="col sm12 text-h2 mb-6">Service</h1>

      <div class="col sm12 lg8 xl7">
        <v-tabs>
          <v-tab label="Settings" icon="connect">
            <ServiceSettings v-model="settings" :scan-list="scanList" @save="onSave" @scan="onScan" />
          </v-tab>

          <v-tab label="Storage" icon="storage">
            <div class="row">
              <h2 class="col sm12 text-h5 mb-4">File system</h2>

              <div class="col sm12">
                <ServiceStorage v-model="path" :files="fileList" :progress="progress" :info="info" @send="onSend" />
              </div>
            </div>
          </v-tab>

          <v-tab label="System" icon="update">
            <div class="row">
              <div class="col sm12">
                <ServiceSystem @reboot="onSureReboot" @reset="onSureReset" />
              </div>
            </div>
          </v-tab>

          <!-- <v-tab label="Other" icon="onher">
            <div class="row"></div>
          </v-tab> -->

          <!-- <v-tab label="GPIO" icon="onher">
            <div class="row">
              <div class="col sm12">
                <ServiceGPIO :gpios="gpios" @send="onSend" @reboot="onSureReboot" />
              </div>
            </div>
          </v-tab> -->
        </v-tabs>
      </div>

      <div class="col sm12 lg4 xl4 offset-xl1 mt-10">
        <ServiceInfo v-bind="info" class="page-main__file-info" />
      </div>
    </div>
  </div>
</template>

<script setup>
import { onMounted, inject, nextTick } from 'vue';
import { storeToRefs } from 'pinia';
import event from '@/assets/js/event';

import ServiceGPIO from '@/components/pages/service/ServiceGPIO';
import ServiceStorage from '@/components/pages/service/ServiceStorage';
import ServiceSettings from '@/components/pages/service/ServiceSettings';
import ServiceInfo from '@/components/pages/service/ServiceInfo';
import ServiceSystem from '@/components/pages/service/ServiceSystem';

import { useWebSocketStore } from '@/stores/WebSocketStore';

const dialog = inject('dialog');
const webSocketStore = useWebSocketStore();
const { fileList, info, path, progress, settings, scanList, gpios } = storeToRefs(webSocketStore);

const onReboot = () => {
  webSocketStore.onSend('REBOOT');
  nextTick(() => {
    dialog({ value: true, title: 'Done', message: 'Reboot...' });
    setTimeout(() => {
      dialog({});
    }, 2000);
  });
};
const onReset = () => {
  settings.value.version = Math.floor(Math.random() * 65000);
  nextTick(() => onSave(settings.value));
};

const onSureReboot = () => dialog({ value: true, message: 'Do you want to restart your device?', callback: onReboot });
const onSureReset = () => dialog({ value: true, message: 'The configuration will be reset to default. <br/>Are you sure?', callback: onReset });

const onSend = ({ comm, data }) => {
  fileList.value = [];
  webSocketStore.onSend(comm, data);
};

const onSave = settings => {
  webSocketStore.onSend('SETTINGS', settings);
  dialog({ value: true, title: 'Done', message: 'Do you want to restart your device?', callback: onReboot });
};

const onScan = value => {
  if (value) scanList.value = [];
  if (!scanList.value.length) webSocketStore.onSend('SCAN');
};

event.on('init', () => {
  if (!settings?.key) webSocketStore.onSend('SETTINGS');
});

onMounted(() => {
  webSocketStore.onSend('SETTINGS');
});
</script>
