<template>
  <div class="page-service container">
    <div class="row">
      <div class="col sm12 text-h2">Service</div>
      <div class="col sm12">
        <v-tabs>
          <v-tab label="Ethernet">
            <div class="row">
              <div class="col sm12 mb-6">
                <v-checkbox v-model="settings.ethDhcp">DHCP</v-checkbox>
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.ethIp" label="IP" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.ethSubnet" label="Subnet" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
              </div>

              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.ethGeteway" label="Geteway" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.ethDns" label="DNS" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12">
                <v-button @click="onSave">Save</v-button>
              </div>
            </div>
          </v-tab>
          <v-tab label="Wi-Fi">
            <div class="row">
              <div class="col sm12 md6 lg4">
                <v-select :value="getMode" label="Mode" :list="listWiFi" @change="onChange"></v-select>
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.wifiSsid" label="SSID" :disabled="isWifi" :append-button="!isWifi" :rules="[rules.required, rules.max]" @on-icon="onScan">
                  <template #icon>
                    <v-icons icon="search"></v-icons>
                  </template>
                </v-input>
              </div>
              <div class="col sm12 md6 lg4">
                <v-input
                  id="wifiPass"
                  v-model="settings.wifiPass"
                  label="Password"
                  :disabled="isWifi"
                  :type="showPass ? 'text' : 'password'"
                  :rules="[rules.min, rules.max]"
                  @on-icon="showPass = !showPass"
                >
                  <template #icon>
                    <v-icons :icon="`${showPass ? 'eye-open' : 'eye-close'}`"></v-icons>
                  </template>
                </v-input>
              </div>
              <div class="col sm12 mb-4">
                <v-checkbox v-model="settings.wifiDhcp" :disabled="!settings.wifiMode">DHCP</v-checkbox>
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.wifiIp" label="IP" :disabled="isWifiDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.wifiSubnet" label="Subnet" :disabled="isWifiDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.wifiGeteway" label="Geteway" :disabled="isWifiDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.wifiDns" label="DNS" :disabled="isWifiDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12">
                <v-button @click="onSave">Save</v-button>
              </div>
            </div>
          </v-tab>
          <v-tab label="Server">
            <div class="row">
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.serverUrl" label="IP" :rules="[rules.required, rules.max]" />
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model.number="settings.serverPort" label="Port" :rules="[rules.required, rules.isPort]" />
              </div>
              <div class="col sm12">
                <v-button @click="onSave">Save</v-button>
              </div>
            </div>
          </v-tab>
          <v-tab label="Authentication" icon="icon-lock">
            <div class="row">
              <div class="col sm12 mb-6">
                <v-checkbox v-model="settings.httpMode">AUTHENTICATION</v-checkbox>
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.httpLogin" label="Login" :disabled="isAuth" :rules="[rules.required, rules.max12]" />
              </div>
              <div class="col sm12 md6 lg4">
                <v-input v-model="settings.httpPass" label="Password" :type="showPass ? 'text' : 'password'" :disabled="isAuth" :rules="[rules.required, rules.max12]" @on-icon="showPass = !showPass">
                  <template #icon>
                    <v-icons :icon="`${showPass ? 'eye-open' : 'eye-close'}`"></v-icons>
                  </template>
                </v-input>
              </div>
              <div class="col sm12">
                <v-button @click="onSave">Save</v-button>
              </div>
            </div>
          </v-tab>
          <v-tab label="File system">
            <div class="row">
              <div class="col sm12 lg6 xl6">
                <FileManager :files="fileList" :progress="progress" :info="info" @send="onSend" @clear="onClear" @message="setDialog" />
              </div>
              <div class="col sm12 lg5 offset-lg1 xl4 offset-xl2">
                <FileInfo v-bind="info" class="page-main__file-info" />
              </div>
            </div>
          </v-tab>
          <v-tab label="Update">
            <UpdateFlash />
          </v-tab>
        </v-tabs>
      </div>
    </div>
    <AppDialog title="SCAN" :value="showDialog" @close="showDialog = false">
      <WifiList :list="scanList" @select="onSelectSsid" />
      <template #footer>
        <v-button @click="onScanDialog">Scan</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, watch, defineProps } from 'vue';
import { storeToRefs } from 'pinia';
import { validateIP, min, max } from '@/utils/validate/';

import AppDialog from '@/components/app/AppDialog';
import FileManager from '@/components/pages/service/FileManager';
import FileInfo from '@/components/pages/service/FileInfo';
import WifiList from '@/components/pages/service/WifiList';
import UpdateFlash from '@/components/pages/service/UpdateFlash';

import { useWebSocketStore } from '@/stores/WebSocketStore';

defineProps({
  setDialog: { type: Function, default: () => {} },
});

const webSocketStore = useWebSocketStore();
const { fileList, info, progress, settings, scanList, isConnect } = storeToRefs(webSocketStore);

const showPass = ref(false);
const showDialog = ref(false);

const rules = {
  required: value => !!value || 'Required.',
  ip: v => validateIP(v) || 'Invalid ip address',
  min: v => min(8, v) || 'Min 8 characters',
  max: v => max(32, v) || 'Max 32 characters',
  max12: v => max(12, v) || 'Max 12 characters',
  isPort: v => (typeof +v === 'number' && !isNaN(+v) && +v < 65536) || 'Invalid port',
};

const listWiFi = [
  { name: 'OFF', value: 0 },
  { name: 'STA', value: 1 },
  { name: 'AP', value: 2 },
  { name: 'STA + AP', value: 3 },
];

const getMode = computed(() => listWiFi.find(i => i.value === settings.value.wifiMode)?.name || '');
const isWifiDHCP = computed(() => Boolean(settings.value.wifiDhcp || !settings.value.wifiMode));
const isEthernetDHCP = computed(() => Boolean(settings.value.ethDhcp));
const isWifi = computed(() => Boolean(!settings.value.wifiMode));
const isAuth = computed(() => Boolean(!settings.value.httpMode));

// const onReboot = async () => webSocketStore.onSend('REBOOT');

const onSave = () => {
  webSocketStore.onSend('SETTINGS', settings.value);
  // onReboot();
};
const onSelectSsid = ({ ssid }) => {
  settings.value.wifiMode = 1;
  settings.value.wifiSsid = ssid;
  const input = document.querySelector('#wifiPass input');
  input.select();
  input.focus();
  showDialog.value = false;
};
const onScan = () => {
  showDialog.value = true;
  if (!scanList.value.length) webSocketStore.onSend('SCAN');
};

const onScanDialog = () => {
  scanList.value = [];
  if (!scanList.value.length) webSocketStore.onSend('SCAN');
};

const onChange = ({ value }) => {
  console.log(value);
  settings.value.wifiMode = value;
};

const onSend = ({ comm, data }) => webSocketStore.onSend(comm, data);
const onClear = () => (fileList.value = []);

onMounted(() => {
  webSocketStore.onSend('SETTINGS');
});
watch(isConnect, isConnect => {
  console.log(isConnect);
  if (!settings?.key && isConnect) webSocketStore.onSend('SETTINGS');
});
</script>

<style lang="scss">
.page-service {
  position: relative;
}
</style>
