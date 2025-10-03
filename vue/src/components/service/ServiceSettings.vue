<template>
  <div class="grid grid-cols-1 xl:grid-cols-2 2xl:grid-cols-3 gap-4">
    <card-gray title="Wi-Fi">
      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <v-select class="mb-6" :value="settings.wifiMode" label="Mode" :list="listWiFi" @change="onSureOffWifi"></v-select>

        <v-text-field v-model="wifiSsid" v-bind="wifiSsidAttrs" label="SSID" :disabled="isWifi" :append-button="!isWifi" @on-icon="onScan">
          <template #icon>
            <v-icons name="Search"></v-icons>
          </template>
        </v-text-field>

        <v-text-field id="wifiPass" v-model="wifiPass" v-bind="wifiPassAttrs" label="Password" :disabled="isWifi" :type="showPass ? 'text' : 'password'" @on-icon="showPass = !showPass">
          <template #icon>
            <v-icons class="size-5" :name="showPass ? 'EyeOpen' : 'EyeClose'"></v-icons>
          </template>
        </v-text-field>

        <v-text-field v-model="rePassword" v-bind="rePasswordAttrs" label="Repeat password" :disabled="isWifi" :type="showPass ? 'text' : 'password'" @on-icon="showPass = !showPass">
          <template #icon>
            <v-icons class="size-5" :name="showPass ? 'EyeOpen' : 'EyeClose'"></v-icons>
          </template>
        </v-text-field>
      </div>
    </card-gray>

    <card-gray title="IP Settings">
      <template #header>
        <div class="col-span-full">
          <VCheckbox v-model="settings.wifiDhcp">DHCP</VCheckbox>
        </div>
      </template>

      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <v-text-field v-model="wifiIp" v-bind="wifiIpAttrs" label="IP" :disabled="isWifiDHCP" />

        <v-text-field v-model="wifiSubnet" v-bind="wifiSubnetAttrs" label="Subnet" :disabled="isWifiDHCP" />

        <v-text-field v-model="wifiGateway" v-bind="wifiGatewayAttrs" label="Gateway" :disabled="isWifiDHCP" />

        <v-text-field v-model="wifiDns" v-bind="wifiDnsAttrs" label="DNS" :disabled="isWifiDHCP" />
      </div>
    </card-gray>

    <card-gray title="Security">
      <template #header>
        <div class="col-span-full">
          <VCheckbox v-model="settings.authMode">AUTHENTICATION</VCheckbox>
        </div>
      </template>

      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <v-text-field v-model="authLogin" v-bind="authLoginAttrs" label="Login" :disabled="isAuth" />

        <v-text-field
          v-model="authPass"
          v-bind="authPassAttrs"
          label="Password"
          class="col-end-2"
          :type="showAuthPass ? 'text' : 'password'"
          :disabled="isAuth"
          @on-icon="showAuthPass = !showAuthPass"
        >
          <template #icon>
            <v-icons class="size-5" :name="showAuthPass ? 'EyeOpen' : 'EyeClose'"></v-icons>
          </template>
        </v-text-field>

        <v-text-field v-model="reAuthPassword" v-bind="reAuthPasswordAttrs" label="Password" :type="showAuthPass ? 'text' : 'password'" :disabled="isAuth" @on-icon="showAuthPass = !showAuthPass">
          <template #icon>
            <v-icons class="size-5" :name="showAuthPass ? 'EyeOpen' : 'EyeClose'"></v-icons>
          </template>
        </v-text-field>
      </div>
    </card-gray>

    <card-gray title="Update">
      <BlockUpdate :onDialog="onDialog" />
    </card-gray>

    <card-gray title="System">
      <template #header>
        <button class="text-gray-400 cursor-pointer" @click="dialogInfo = true">
          <v-icons name="Info" class="size-5" />
        </button>
      </template>

      <BlockService @reset="onSureReset" @reboot="onSureReboot" />
    </card-gray>

    <Teleport to="[data-slot='device']">
      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <v-icons name="Dots" class="rotate-90"></v-icons>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenu"></v-list>
      </v-dropdown>
    </Teleport>

    <app-dialog v-if="showDialog" title="SCAN" size="sm" @close="onClose">
      <BlockScan :items="scanList" @select="onSelectSsid" @scan="onScan" />
    </app-dialog>
  </div>
</template>

<script setup lang="ts">
import type { Ref } from 'vue';
import type { TypeMessage, IMessageScan, IListItem, TypeSend } from '@/types';

import { computed, ref, nextTick, watch } from 'vue';
import { required, maxLen, minLen, sameAs, ip } from '@/utils/validate.js';

import { KEYS } from '@/types';

import { useForm } from 'vuesp-components/composables';

import { useConnection } from '@/composables/useConnection.js';
import { arrToStr, strToArr } from 'vuesp-components/helpers';

import { VCheckbox, BlockUpdate } from 'vuesp-components';
import BlockScan from '@/components/block/BlockScan.vue';
import BlockService from '@/components/block/BlockService.vue';

const listMenu: IListItem[] = [{ name: 'Save', value: 1 }];

const showPass = ref(false);
const showAuthPass = ref(false);
const showDialog = ref(false);

const scanList: Ref<Partial<IMessageScan>[]> = ref([]);

const onMessage = ({ key, object }: TypeMessage) => {
  if (key === KEYS.SCAN && object) scanList.value.push(object);
};

const onInit = (send: TypeSend) => {
  send(KEYS.SETTINGS);
};

const { settings, dialogInfo, onSend, onDialog } = useConnection(onInit, onMessage);

const { defineField, handleSubmit } = useForm({
  validationSchema: {
    wifiSsid: [required, maxLen(32)],
    wifiPass: [required, maxLen(32), minLen(8)],
    rePassword: [required, maxLen(32), minLen(8), sameAs('wifiPass')],
    wifiIp: [ip],
    wifiSubnet: [ip],
    wifiGateway: [ip],
    wifiDns: [ip],
    authLogin: [required, maxLen(12)],
    authPass: [required, maxLen(12)],
    reAuthPassword: [required, maxLen(12), sameAs('authPass')],
  },
});

const [wifiSsid, wifiSsidAttrs] = defineField<string>('wifiSsid');
const [wifiPass, wifiPassAttrs] = defineField<string>('wifiPass');
const [rePassword, rePasswordAttrs] = defineField<string>('rePassword');
const [wifiIp, wifiIpAttrs] = defineField<string>('wifiIp');
const [wifiSubnet, wifiSubnetAttrs] = defineField<string>('wifiSubnet');
const [wifiGateway, wifiGatewayAttrs] = defineField<string>('wifiGateway');
const [wifiDns, wifiDnsAttrs] = defineField<string>('wifiDns');
const [authLogin, authLoginAttrs] = defineField<string>('authLogin');
const [authPass, authPassAttrs] = defineField<string>('authPass');
const [reAuthPassword, reAuthPasswordAttrs] = defineField<string>('reAuthPassword');

watch(
  () => settings.value,
  () => {
    wifiSsid.value = settings.value.wifiSsid;
    wifiPass.value = settings.value.wifiPass;
    rePassword.value = settings.value.wifiPass;
    wifiIp.value = arrToStr(settings.value.wifiIp);
    wifiSubnet.value = arrToStr(settings.value.wifiSubnet);
    wifiGateway.value = arrToStr(settings.value.wifiGateway);
    wifiDns.value = arrToStr(settings.value.wifiDns);
    authLogin.value = settings.value.authLogin;
    authPass.value = settings.value.authPass;
    reAuthPassword.value = settings.value.authPass;
  }
);

const onSubmit = handleSubmit(async (values) => {
  settings.value.wifiSsid = values.wifiSsid;
  settings.value.wifiPass = values.wifiPass;
  settings.value.wifiIp = strToArr(values.wifiIp);
  settings.value.wifiSubnet = strToArr(values.wifiSubnet);
  settings.value.wifiGateway = strToArr(values.wifiGateway);
  settings.value.wifiDns = strToArr(values.wifiDns);
  settings.value.authLogin = values.authLogin;
  settings.value.authPass = values.authPass;
  onSave();
});

const listWiFi: IListItem[] = [
  { name: 'OFF', value: 0 },
  { name: 'STA', value: 1 },
  { name: 'AP', value: 2 },
  // { name: 'STA + AP', value: 3 },
];

const isWifiDHCP = computed(() => Boolean(settings.value.wifiDhcp || !settings.value.wifiMode));
const isWifi = computed(() => Boolean(!settings.value.wifiMode));
const isAuth = computed(() => Boolean(!settings.value.authMode));

const onReboot = () => {
  onSend(KEYS.REBOOT);
};

const onSave = () => {
  onSend(KEYS.SETTINGS, settings.value);
  onDialog({ value: true, title: 'Done', message: 'Do you want to restart your device?', callback: onReboot });
};

const onReset = () => {
  settings.value.version = Math.floor(Math.random() * 65000);
  nextTick(() => {
    onSave();
    onReboot();
  });
};

const onSureReboot = () => onDialog({ value: true, message: 'Do you want to restart your device?', callback: onReboot });
const onSureReset = () => onDialog({ value: true, message: 'The configuration will be reset to default. <br/>Are you sure?', callback: onReset });

const onMenu = () => {
  onSubmit();
};

const onSelectSsid = ({ ssid }: IMessageScan) => {
  settings.value.wifiMode = 1;
  settings.value.wifiSsid = ssid;
  const input: HTMLInputElement | null = document.querySelector('#wifiPass input');
  if (input) input.focus();
  onClose();
};

const onClose = () => (showDialog.value = false);

const onScan = () => {
  showDialog.value = true;
  scanList.value = [];
  onSend(KEYS.SCAN);
};

const onChange = (value: number) => (settings.value.wifiMode = value);
const onSureOffWifi = ({ value }: IListItem) => {
  const v = value as number;
  return !v ? onDialog({ value: true, message: 'You are about to disable Wi-Fi. Are you sure?', callback: onChange.bind(this, v) }) : onChange(v);
};
</script>
