<template>
  <div>
    <v-expansion label="Wi-Fi" value>
      <div class="row">
        <div class="col sm12 md6">
          <v-select :value="getMode" label="Mode" :list="listWiFi" @change="onSureOffWifi"></v-select>
        </div>
        <div class="col sm12"></div>
        <div class="col sm12 md6">
          <v-input v-model="settings.wifiSsid" label="SSID" :disabled="isWifi" :append-button="!isWifi" :rules="[rules.required, rules.max]" @on-icon="onScan(false)">
            <template #icon>
              <v-icons icon="search"></v-icons>
            </template>
          </v-input>
        </div>
        <div class="col sm12 md6">
          <v-input id="wifiPass" v-model="settings.wifiPass" label="Password" :disabled="isWifi" :type="showPass ? 'text' : 'password'" :rules="[rules.min, rules.max]" @on-icon="showPass = !showPass">
            <template #icon>
              <v-icons :icon="`${showPass ? 'eye-open' : 'eye-close'}`"></v-icons>
            </template>
          </v-input>
        </div>
        <div class="col sm12 mb-4">
          <v-checkbox v-model="settings.wifiDhcp" :disabled="!settings.wifiMode">DHCP</v-checkbox>
        </div>
        <div class="col sm12 md6">
          <v-input v-model="settings.wifiIp" label="IP" :disabled="isWifiDHCP" :rules="[rules.ip]" />
        </div>
        <div class="col sm12 md6">
          <v-input v-model="settings.wifiSubnet" label="Subnet" :disabled="isWifiDHCP" :rules="[rules.ip]" />
        </div>
        <div class="col sm12 md6">
          <v-input v-model="settings.wifiGeteway" label="Geteway" :disabled="isWifiDHCP" :rules="[rules.ip]" />
        </div>
        <div class="col sm12 md6">
          <v-input v-model="settings.wifiDns" label="DNS" :disabled="isWifiDHCP" :rules="[rules.ip]" />
        </div>
      </div>
    </v-expansion>
    <!-- <v-expansion label="Ethernet">
      <div class="row">
        <div class="col sm12 mb-6">
          <v-checkbox v-model="settings.ethDhcp">DHCP</v-checkbox>
        </div>
        <div class="col sm12 md6">
          <v-input v-model="settings.ethIp" label="IP" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
        </div>
        <div class="col sm12 md6">
          <v-input v-model="settings.ethSubnet" label="Subnet" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
        </div>

        <div class="col sm12 md6">
          <v-input v-model="settings.ethGeteway" label="Geteway" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
        </div>
        <div class="col sm12 md6">
          <v-input v-model="settings.ethDns" label="DNS" :disabled="isEthernetDHCP" :rules="[rules.ip]" />
        </div>
      </div>
    </v-expansion> -->
    <!-- <v-expansion label="Server">
      <div class="row">
        <div class="col sm12 md6">
          <v-input v-model="settings.serverUrl" label="URL" :rules="[rules.required, rules.max]" />
        </div>
        <div class="col sm12">
          <v-input v-model.number="settings.serverPort" label="Port" :rules="[rules.required, rules.isPort]" />
        </div>
      </div>
    </v-expansion> -->
    <v-expansion label="Security">
      <div class="row">
        <div class="col sm12 mb-6">
          <v-checkbox v-model="settings.authMode">AUTHENTICATION</v-checkbox>
        </div>
        <div class="col sm12 md6">
          <v-input v-model="settings.authLogin" label="Login" :disabled="isAuth" :rules="[rules.required, rules.max12]" />
        </div>
        <div class="col sm12 md6">
          <v-input v-model="settings.authPass" label="Password" :type="showPass ? 'text' : 'password'" :disabled="isAuth" :rules="[rules.required, rules.max12]" @on-icon="showPass = !showPass">
            <template #icon>
              <v-icons :icon="`${showPass ? 'eye-open' : 'eye-close'}`"></v-icons>
            </template>
          </v-input>
        </div>
      </div>
    </v-expansion>
    <div class="row mt-6">
      <div class="col sm12 d-flex j-end">
        <v-button @click="onSave">Save</v-button>
      </div>
    </div>
    <AppDialog title="SCAN" :value="showDialog" @close="showDialog = false">
      <WifiList :list="scanList" @select="onSelectSsid" />
      <template #footer>
        <v-button @click="onScan(true)">Scan</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup>
import { computed, ref, defineProps, defineEmits, inject } from 'vue';
import { validateIP, min, max } from '@/utils/validate/';

import AppDialog from '@/components/app/AppDialog';
import WifiList from '@/components/pages/service/WifiList';

const props = defineProps({
  modelValue: { type: Object, default: () => ({}) },
  scanList: { type: Array, default: () => [] },
});

const emit = defineEmits(['update:modelValue', 'scan', 'save']);
const dialog = inject('dialog');

const showPass = ref(false);
const showDialog = ref(false);

const settings = computed({
  set: value => emit('update:modelValue', value),
  get: () => props.modelValue,
});

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
  // { name: 'STA + AP', value: 3 },
];

const getMode = computed(() => listWiFi.find(i => i.value === settings.value.wifiMode)?.name || '');
const isWifiDHCP = computed(() => Boolean(settings.value.wifiDhcp || !settings.value.wifiMode));
const isWifi = computed(() => Boolean(!settings.value.wifiMode));
const isAuth = computed(() => Boolean(!settings.value.authMode));

const onSave = () => emit('save', settings.value);

const onSelectSsid = ({ ssid }) => {
  settings.value.wifiMode = 1;
  settings.value.wifiSsid = ssid;
  const input = document.querySelector('#wifiPass input');
  input.select();
  input.focus();
  showDialog.value = false;
};
const onScan = value => {
  showDialog.value = true;
  emit('scan', value);
};

const onChange = value => (settings.value.wifiMode = value);
const onSureOffWifi = ({ value }) => (!value ? dialog({ message: 'You are about to disable Wi-Fi. Are you sure?', callback: onChange.bind(this, value) }) : onChange(value));
</script>
