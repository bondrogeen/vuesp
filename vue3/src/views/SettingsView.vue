<template>
  <div class="page-settings container">
    <div class="row">
      <div class="col sm12">
        <v-tabs>
          <v-tab label="Ethernet">
            <div class="row">
              <div class="col sm12 mb-6">
                <v-checkbox v-model="settings.ethDhcp">DHCP</v-checkbox>
              </div>
              <div class="col sm12 md7">
                <v-input v-model="settings.ethIp" label="IP" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12 md7">
                <v-input v-model="settings.ethSubnet" label="Subnet" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12 md7">
                <v-input v-model="settings.ethGeteway" label="Geteway" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
              </div>
              <div class="col sm12 md7">
                <v-input v-model="settings.ethDns" label="DNS" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
              </div>
            </div>
          </v-tab>
          <v-tab label="Wi-Fi">
            <div class="col sm12">
              <div class="row">
                <div class="col sm12 md7">
                  <v-select :value="getMode" label="Mode" :list="listWiFi" @change="onChange"></v-select>
                </div>
                <div class="col sm12">
                  <div class="row">
                    <div class="col sm12 md7">
                      <v-input v-model="settings.wifiSsid" label="SSID" :disabled="isWifi" :append-button="!isWifi" :rules="[rules.required, rules.max]" @on-icon="onScan">
                        <template #icon>
                          <v-icons icon="search"></v-icons>
                        </template>
                      </v-input>
                    </div>
                    <div class="col sm12 md7">
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
                  </div>
                </div>
                <div class="col sm12 mb-4">
                  <v-checkbox v-model="settings.wifiDhcp" :disabled="!settings.wifiMode">DHCP</v-checkbox>
                </div>
                <div class="col sm12 md7">
                  <v-input v-model="settings.wifiIp" label="IP" :disabled="isWifiDHCP" :rules="[rules.ip]" />
                </div>
                <div class="col sm12 md7">
                  <v-input v-model="settings.wifiSubnet" label="Subnet" :disabled="isWifiDHCP" :rules="[rules.ip]" />
                </div>
                <div class="col sm12 md7">
                  <v-input v-model="settings.wifiGeteway" label="Geteway" :disabled="isWifiDHCP" :rules="[rules.ip]" />
                </div>
                <div class="col sm12 md7">
                  <v-input v-model="settings.wifiDns" label="DNS" :disabled="isWifiDHCP" :rules="[rules.ip]" />
                </div>
              </div>
            </div>
          </v-tab>
          <v-tab label="Server">
            <div class="row">
              <div class="col sm12 md7">
                <v-input v-model="settings.serverUrl" label="IP" :rules="[rules.required, rules.max]" />
              </div>
              <div class="col sm12 md7">
                <v-input v-model.number="settings.serverPort" label="Port" :rules="[rules.required, rules.isPort]" />
              </div>
            </div>
          </v-tab>
          <v-tab label="Authentication" icon="icon-lock">
            <div class="row">
              <div class="col sm12 mb-6">
                <v-checkbox v-model="settings.httpMode">AUTHENTICATION</v-checkbox>
              </div>
              <div class="col sm12 md7">
                <v-input v-model="settings.httpLogin" label="Login" :disabled="isAuth" :rules="[rules.required, rules.max12]" />
              </div>
              <div class="col sm12 md7">
                <v-input v-model="settings.httpPass" label="Password" :type="showPass ? 'text' : 'password'" :disabled="isAuth" :rules="[rules.required, rules.max12]" @on-icon="showPass = !showPass">
                  <template #icon>
                    <v-icons :icon="`${showPass ? 'eye-open' : 'eye-close'}`"></v-icons>
                  </template>
                </v-input>
              </div>
            </div>
          </v-tab>
          <v-tab label="Maintenance">
            <!-- <Maintenance /> -->
          </v-tab>
        </v-tabs>
      </div>
      <div class="col sm12">
        <v-button @click="onSave">Save</v-button>
      </div>
    </div>
    <AppDialog title="SCAN" :value="showDialog" @close="showDialog = false">
      <v-list v-slot="{ item }" :list="scanList">
        <div class="list-wifi d-flex" @click="onSelectSsid(item)">
          <div class="py-2">
            <div class="text-title1">{{ item.ssid }}</div>
            <div class="text-body-2 grey-base">Channel: {{ item.channel }}, rssi: {{ item.rssi }}</div>
            <div class="text-body-2 grey-base">Security: {{ listEncryption[item.encryptionType] || 'unknown' }}</div>
          </div>
          <div></div>
        </div>
      </v-list>
      <div v-if="!scanList.length">
        <v-loader></v-loader>
      </div>
      <template #footer>
        <v-button @click="onScanDialog">Scan</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue';
import { storeToRefs } from 'pinia';
import { validateIP, min, max } from '@/utils/validate/';

import AppDialog from '@/components/app/AppDialog';

import { useWebSocketStore } from '@/stores/WebSocketStore';

const webSocketStore = useWebSocketStore();
const { settings, scanList, isConnect } = storeToRefs(webSocketStore);

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

const listEncryption = ['OPEN', 'WEP', 'WPA_PSK', 'WPA2_PSK', 'WPA_WPA2_PSK', 'MAX', '', 'NO', 'AUTO'];

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

onMounted(() => {
  webSocketStore.onSend('SETTINGS');
});
watch(isConnect, isConnect => {
  console.log(isConnect);
  if (!settings?.key && isConnect) webSocketStore.onSend('SETTINGS');
});
</script>

<style lang="scss">
.page-settings {
  position: relative;
}
.list-wifi {
  border-bottom: 1px solid color('grey', 'lighten-1');
}
</style>
