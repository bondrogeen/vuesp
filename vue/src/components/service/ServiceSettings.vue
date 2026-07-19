<template>
  <div class="grid grid-cols-1 xl:grid-cols-2 2xl:grid-cols-3 gap-4">
    <card-main class="xl:col-span-2 2xl:col-span-3">
      <div class="grid grid-cols-1 xl:grid-cols-2 gap-x-4">
        <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
          <div class="flex justify-between mb-6 md:col-span-2">
            <h3 class="text-lg font-semibold text-gray-800 dark:text-white/90">{{ $t('wifi') }}</h3>
          </div>

          <div class="mb-6">
            <v-select :model-value="getValueListWiFi(settings.wifiMode)" :label="$t('mode')" :items="listWiFi" @change="onSureOffWifi" />
          </div>

          <v-text-field v-model="wifiSsid" v-bind="wifiSsidAttrs" :label="$t('ssid')" :disabled="isWifi" :append-button="!isWifi" @on-icon="onScan">
            <template #icon>
              <icon-ri-search-line></icon-ri-search-line>
            </template>
          </v-text-field>

          <v-text-field id="wifiPass" v-model="wifiPass" v-bind="wifiPassAttrs" :label="$t('pass')" :disabled="isWifi" :type="showPass ? 'text' : 'password'" @on-icon="showPass = !showPass">
            <template #icon>
              <icon-ri-eye-line v-if="showPass" class="size-5"></icon-ri-eye-line>
              <icon-ri-eye-off-line v-else class="size-5"></icon-ri-eye-off-line>
            </template>
          </v-text-field>

          <v-text-field v-model="rePassword" v-bind="rePasswordAttrs" :label="$t('passRe')" :disabled="isWifi" :type="showPass ? 'text' : 'password'" @on-icon="showPass = !showPass">
            <template #icon>
              <icon-ri-eye-line v-if="showPass" class="size-5"></icon-ri-eye-line>
              <icon-ri-eye-off-line v-else class="size-5"></icon-ri-eye-off-line>
            </template>
          </v-text-field>
        </div>

        <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
          <div class="flex justify-between mb-6 md:col-span-2">
            <h3 class="text-lg font-semibold text-gray-800 dark:text-white/90">{{ $t('ipSett') }}</h3>

            <div>
              <VCheckbox v-model="settings.wifiDhcp">{{ $t('dhcp') }}</VCheckbox>
            </div>
          </div>

          <v-text-field v-model="wifiIp" v-bind="wifiIpAttrs" :label="$t('ip')" :disabled="isWifiDHCP" />

          <v-text-field v-model="wifiSubnet" v-bind="wifiSubnetAttrs" :label="$t('subnet')" :disabled="isWifiDHCP" />

          <v-text-field v-model="wifiGateway" v-bind="wifiGatewayAttrs" :label="$t('gateway')" :disabled="isWifiDHCP" />

          <v-text-field v-model="wifiDns" v-bind="wifiDnsAttrs" :label="$t('dns')" :disabled="isWifiDHCP" />
        </div>
      </div>
    </card-main>

    <card-main :title="$t('security')">
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
            <icon-ri-eye-line v-if="showAuthPass" class="size-5"></icon-ri-eye-line>
            <icon-ri-eye-off-line v-else class="size-5"></icon-ri-eye-off-line>
          </template>
        </v-text-field>

        <v-text-field v-model="reAuthPassword" v-bind="reAuthPasswordAttrs" :label="$t('pass')" :type="showAuthPass ? 'text' : 'password'" :disabled="isAuth" @on-icon="showAuthPass = !showAuthPass">
          <template #icon>
            <icon-ri-eye-line v-if="showAuthPass" class="size-5"></icon-ri-eye-line>
            <icon-ri-eye-off-line v-else class="size-5"></icon-ri-eye-off-line>
          </template>
        </v-text-field>
      </div>
    </card-main>

    <card-main :title="$t('update')">
      <BlockUpdate :onDialog="onDialog" />
    </card-main>

    <card-main :title="$t('system')">
      <template #header>
        <button class="text-gray-400 cursor-pointer" @click="dialogInfo = true">
          <icon-ri-information-line class="size-5" />
        </button>
      </template>

      <BlockService :locale="getLocale()" :locales="listLocale" @locale="onLocale" @reset="onSureReset" @reboot="onSureReboot" />
    </card-main>

    <teleport to="[data-slot='device']">
      <v-select :items="listMenu" @change="onMenu">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
          </v-button>
        </template>
      </v-select>
    </teleport>

    <v-dialog v-if="showDialog" :title="$t('scan')" @close="onClose">
      <BlockScan :items="scanList" @select="onSelectSsid" @scan="onScan" />
    </v-dialog>
  </div>
</template>

<script setup lang="ts">
import type { Ref } from 'vue';
import type { TypeMessage, IMessageScan, IListItem, TypeSend } from '@/types';

import { computed, ref, nextTick, watch } from 'vue';
import { required, maxLen, minLen, sameAs, ip } from '@/utils/validate';

import { KEYS } from '@/types';

import { useForm } from 'vuesp-components/composables';
import type { ValidationSchema } from 'vuesp-components/types';

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
  validationSchema: (values) =>
    ({
      wifiSsid: [required, maxLen(32)],
      wifiPass: values.wifiPass !== '' ? [required, minLen(8), maxLen(32)] : [],
      rePassword: values.wifiPass !== '' ? [required, minLen(8), maxLen(32), sameAs('wifiPass')] : [],
      wifiIp: [ip],
      wifiSubnet: [ip],
      wifiGateway: [ip],
      wifiDns: [ip],
      authLogin: settings.value.authMode ? [required, maxLen(12)] : [],
      authPass: settings.value.authMode ? [required, maxLen(12)] : [],
      reAuthPassword: settings.value.authMode ? [required, maxLen(12), sameAs('authPass')] : [],
    }) as ValidationSchema,
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
    if (settings.value.wifiPass) wifiPass.value = settings.value.wifiPass;
    if (settings.value.wifiPass) rePassword.value = settings.value.wifiPass;
    wifiIp.value = arrToStr(settings.value.wifiIp);
    wifiSubnet.value = arrToStr(settings.value.wifiSubnet);
    wifiGateway.value = arrToStr(settings.value.wifiGateway);
    wifiDns.value = arrToStr(settings.value.wifiDns);
    authLogin.value = settings.value.authLogin;
    authPass.value = settings.value.authPass;
    reAuthPassword.value = settings.value.authPass;
  }
);

const onSubmit = handleSubmit((values) => {
  settings.value.wifiSsid = values.wifiSsid as string;
  settings.value.wifiPass = values.wifiPass as string;
  settings.value.wifiIp = strToArr(values.wifiIp as string);
  settings.value.wifiSubnet = strToArr(values.wifiSubnet as string);
  settings.value.wifiGateway = strToArr(values.wifiGateway as string);
  settings.value.wifiDns = strToArr(values.wifiDns as string);
  settings.value.authLogin = values.authLogin as string;
  settings.value.authPass = values.authPass as string;
  onSave();
});

const listWiFi: IListItem[] = [
  { name: $t('off'), value: 0 },
  { name: $t('sta'), value: 1 },
  { name: $t('ap'), value: 2 },
  // { name: 'STA + AP', value: 3 },
];

const getValueListWiFi = (mode: number) => listWiFi.find((i) => i.value === mode)?.name;

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

const onSureReboot = () => onDialog({ value: true, message: $t('dialog.doRestart'), callback: onReboot });
const onSureReset = () => onDialog({ value: true, message: $t('dialog.doReset'), callback: onReset });

const onMenu = () => {
  onSubmit();
};

const onSelectSsid = ({ ssid }: IMessageScan) => {
  settings.value.wifiMode = 1;
  settings.value.wifiSsid = ssid;
  wifiSsid.value = ssid;
  wifiPass.value = '';
  rePassword.value = '';
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
