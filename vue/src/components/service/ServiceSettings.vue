<template>
  <div class="grid grid-cols-1 xl:grid-cols-2 2xl:grid-cols-3 gap-4">
    <card-gray :title="$t('wifi')">
      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <v-select class="mb-6" :value="settings.wifiMode" :label="$t('mode')" :list="listWiFi" @change="onSureOffWifi"></v-select>

        <v-text-field v-model="wifiSsid" v-bind="wifiSsidAttrs" :label="$t('ssid')" :disabled="isWifi" :append-button="!isWifi" @on-icon="onScan">
          <template #icon>
            <v-icons name="Search"></v-icons>
          </template>
        </v-text-field>

        <v-text-field id="wifiPass" v-model="wifiPass" v-bind="wifiPassAttrs" :label="$t('pass')" :disabled="isWifi" :type="showPass ? 'text' : 'password'" @on-icon="showPass = !showPass">
          <template #icon>
            <v-icons class="size-5" :name="showPass ? 'EyeOpen' : 'EyeClose'"></v-icons>
          </template>
        </v-text-field>

        <v-text-field v-model="rePassword" v-bind="rePasswordAttrs" :label="$t('passRe')" :disabled="isWifi" :type="showPass ? 'text' : 'password'" @on-icon="showPass = !showPass">
          <template #icon>
            <v-icons class="size-5" :name="showPass ? 'EyeOpen' : 'EyeClose'"></v-icons>
          </template>
        </v-text-field>
      </div>
    </card-gray>

    <card-gray :title="$t('ipSett')">
      <template #header>
        <div class="col-span-full">
          <VCheckbox v-model="settings.wifiDhcp">{{ $t('dhcp') }}</VCheckbox>
        </div>
      </template>

      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <v-text-field v-model="wifiIp" v-bind="wifiIpAttrs" :label="$t('ip')" :disabled="isWifiDHCP" />

        <v-text-field v-model="wifiSubnet" v-bind="wifiSubnetAttrs" :label="$t('subnet')" :disabled="isWifiDHCP" />

        <v-text-field v-model="wifiGateway" v-bind="wifiGatewayAttrs" :label="$t('gateway')" :disabled="isWifiDHCP" />

        <v-text-field v-model="wifiDns" v-bind="wifiDnsAttrs" :label="$t('dns')" :disabled="isWifiDHCP" />
      </div>
    </card-gray>

    <card-gray :title="$t('security')">
      <template #header>
        <div class="col-span-full">
          <VCheckbox v-model="settings.authMode">{{ $t('auth') }}</VCheckbox>
        </div>
      </template>

      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <v-text-field v-model="authLogin" v-bind="authLoginAttrs" :label="$t('login')" :disabled="isAuth" />

        <v-text-field
          v-model="authPass"
          v-bind="authPassAttrs"
          :label="$t('pass')"
          class="col-end-2"
          :type="showAuthPass ? 'text' : 'password'"
          :disabled="isAuth"
          @on-icon="showAuthPass = !showAuthPass"
        >
          <template #icon>
            <v-icons class="size-5" :name="showAuthPass ? 'EyeOpen' : 'EyeClose'"></v-icons>
          </template>
        </v-text-field>

        <v-text-field v-model="reAuthPassword" v-bind="reAuthPasswordAttrs" :label="$t('pass')" :type="showAuthPass ? 'text' : 'password'" :disabled="isAuth" @on-icon="showAuthPass = !showAuthPass">
          <template #icon>
            <v-icons class="size-5" :name="showAuthPass ? 'EyeOpen' : 'EyeClose'"></v-icons>
          </template>
        </v-text-field>
      </div>
    </card-gray>

    <card-gray :title="$t('update')">
      <BlockUpdate :onDialog="onDialog" />
    </card-gray>

    <card-gray :title="$t('system')">
      <template #header>
        <button class="text-gray-400 cursor-pointer" @click="dialogInfo = true">
          <v-icons name="Info" class="size-5" />
        </button>
      </template>

      <BlockService :locale="getLocale()" :locales="listLocale" @locale="onLocale" @reset="onSureReset" @reboot="onSureReboot" />
    </card-gray>

    <teleport to="[data-slot='device']">
      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <v-icons name="Dots" class="rotate-90"></v-icons>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenu"></v-list>
      </v-dropdown>
    </teleport>

    <app-dialog v-if="showDialog" :title="$t('scan')" size="sm" @close="onClose">
      <BlockScan :items="scanList" @select="onSelectSsid" @scan="onScan" />
    </app-dialog>
  </div>
</template>

<script setup lang="ts">
import type { Ref } from 'vue';
import type { TypeMessage, IMessageScan, IListItem, TypeSend } from '@/types';

import { computed, ref, nextTick, watch } from 'vue';
import { required, maxLen, minLen, sameAs, ip } from '@/utils/validate';

import { KEYS } from '@/types';

import { useForm } from 'vuesp-components/composables';

import { useConnection } from '@/composables/useConnection';
import { arrToStr, strToArr } from 'vuesp-components/helpers';

import { VCheckbox } from 'vuesp-components';
import { useLocale } from '@/composables/useLocale';

import BlockScan from '@/components/block/BlockScan.vue';
import BlockService from '@/components/block/BlockService.vue';
import BlockUpdate from '@/components/block/BlockUpdate.vue';

const { $t, getLocale, setLocale, list } = useLocale();

const listLocale = computed(() => list.map((i: string) => ({ name: $t(`locale.${i}`), value: i })));
const onLocale = ({ value }: IListItem) => setLocale(value as string);

const listMenu: IListItem[] = [{ name: $t('save'), value: 1 }];

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
    wifiPass: [required, minLen(8), maxLen(32)],
    rePassword: [required, minLen(8), maxLen(32), sameAs('wifiPass')],
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
  { name: $t('off'), value: 0 },
  { name: $t('sta'), value: 1 },
  { name: $t('ap'), value: 2 },
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
  onSureReboot();
};

const onReset = () => {
  settings.value.version = Math.floor(Math.random() * 65000);
  nextTick(() => {
    onSave();
    onReboot();
  });
};

const onSureReboot = () => onDialog({ value: true, title: $t('done'), message: $t('dialog.doRestart'), callback: onReboot });
const onSureReset = () => onDialog({ value: true, message: $t('dialog.doReset'), callback: onReset });

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
  return !v ? onDialog({ value: true, message: $t('dialog.doReset'), callback: onChange.bind(this, v) }) : onChange(v);
};
</script>
