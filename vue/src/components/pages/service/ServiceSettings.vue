<template>
  <div class="grid grid-cols-1 xl:grid-cols-2 gap-4">
    <VCardGray title="Wi-Fi">
      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <VSelect :value="getMode" label="Mode" :list="listWiFi" @change="onSureOffWifi"></VSelect>

        <VTextField v-model="v.wifiSsid.value" label="SSID" :disabled="isWifi" :append-button="!isWifi" :message="getError('wifiSsid')" @blur="v.wifiSsid.blur" @on-icon="onScan(false)">
          <template #icon>
            <IconSearch></IconSearch>
          </template>
        </VTextField>

        <VTextField
          id="wifiPass"
          v-model="v.wifiPass.value"
          label="Password"
          :disabled="isWifi"
          :type="showPass ? 'text' : 'password'"
          :message="getError('wifiPass')"
          @blur="v.wifiPass.blur"
          @on-icon="showPass = !showPass"
        >
          <template #icon>
            <IconEyeOpen v-if="showPass"></IconEyeOpen>

            <IconEyeClose v-else></IconEyeClose>
          </template>
        </VTextField>

        <VTextField
          v-model="v.rePassword.value"
          label="Repeat password"
          :disabled="isWifi"
          :message="getError('rePassword')"
          :type="showPass ? 'text' : 'password'"
          @blur="v.rePassword.blur"
          @on-icon="showPass = !showPass"
        >
          <template #icon>
            <IconEyeOpen v-if="showPass"></IconEyeOpen>

            <IconEyeClose v-else></IconEyeClose>
          </template>
        </VTextField>
      </div>
    </VCardGray>

    <VCardGray title="IP Settings">
      <template #header>
        <div class="col-span-full">
          <VCheckbox v-model="settings.wifiDhcp">DHCP</VCheckbox>
        </div>
      </template>

      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <VTextField v-model="v.wifiIp.value" label="IP" :message="getError('wifiIp')" :disabled="isWifiDHCP" @blur="v.wifiIp.blur" />

        <VTextField v-model="v.wifiSubnet.value" label="Subnet" :message="getError('wifiSubnet')" :disabled="isWifiDHCP" @blur="v.wifiSubnet.blur" />

        <VTextField v-model="v.wifiGateway.value" label="Gateway" :message="getError('wifiGateway')" :disabled="isWifiDHCP" @blur="v.wifiGateway.blur" />

        <VTextField v-model="v.wifiDns.value" label="DNS" :message="getError('wifiDns')" :disabled="isWifiDHCP" @blur="v.wifiDns.blur" />
      </div>
    </VCardGray>

    <VCardGray title="Security">
      <template #header>
        <div class="col-span-full">
          <VCheckbox v-model="settings.authMode">AUTHENTICATION</VCheckbox>
        </div>
      </template>

      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <VTextField v-model="v.authLogin.value" label="Login" :disabled="isAuth" />

        <VTextField
          v-model="v.authPass.value"
          class="col-end-2"
          label="Password"
          :type="showAuthPass ? 'text' : 'password'"
          :disabled="isAuth"
          :message="getError('authPass')"
          @blur="v.authPass.blur"
          @on-icon="showAuthPass = !showAuthPass"
        >
          <template #icon>
            <IconEyeOpen v-if="showAuthPass"></IconEyeOpen>

            <IconEyeClose v-else></IconEyeClose>
          </template>
        </VTextField>

        <VTextField
          v-model="v.reAuthPassword.value"
          label="Password"
          :type="showAuthPass ? 'text' : 'password'"
          :disabled="isAuth"
          :message="getError('reAuthPassword')"
          @blur="v.reAuthPassword.blur"
          @on-icon="showAuthPass = !showAuthPass"
        >
          <template #icon>
            <IconEyeOpen v-if="showAuthPass"></IconEyeOpen>

            <IconEyeClose v-else></IconEyeClose>
          </template>
        </VTextField>
      </div>
    </VCardGray>

    <VCardGray title="Update">
      <div class="mb-4 flex items-center">
        <div class="md:flex flex-auto gap-4">
          <h6 class="text-gray-600 bg:text-gray-400">Firmware:</h6>

          <v-text-field-file v-slot="{ files }" accept=".bin" @change="onUpdateFirmware">
            <span>{{ getFileNames(files) }}</span>
          </v-text-field-file>
        </div>
        <v-button class="min-w-[100px]" color="blue" size="small" :disabled="isDisabledFirmware" @click="onSureFlash('firmware')">Update</v-button>
      </div>

      <div class="flex items-center gap-4 mb-4">
        <div class="md:flex flex-auto gap-4">
          <h6 class="text-gray-600 bg:text-gray-400">LittleFS:</h6>

          <v-text-field-file v-slot="{ files }" accept=".bin" @change="onUpdateLittleFS">
            <span>{{ getFileNames(files) }}</span>
          </v-text-field-file>
        </div>

        <v-button class="min-w-[100px]" color="blue" size="small" :disabled="isDisabledLittleFS" @click="onSureFlash('LittleFS')">Update</v-button>
      </div>
    </VCardGray>

    <VCardGray title="System">
      <div class="flex items-center mb-4">
        <div class="flex-auto text-gray-600 bg:text-gray-400">Reboot device</div>

        <v-button class="min-w-[100px]" color="gray" size="small" @click="emit('reboot', $event)">Reboot</v-button>
      </div>

      <div class="flex items-center mb-4">
        <div class="flex-auto text-gray-600 bg:text-gray-400">Reset configuration</div>

        <v-button class="min-w-[100px]" color="red" size="small" @click="emit('reset', $event)">Reset</v-button>
      </div>
    </VCardGray>

    <Teleport to="[data-slot='device']">
      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-dots class="rotate-90"></icon-dots>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenu"></v-list>
      </v-dropdown>
    </Teleport>

    <AppDialog v-if="showDialog" title="SCAN" size="sm" @close="onClose">
      <div>
        <v-list v-slot="{ item }" :list="scanList">
          <div class="flex items-center w-full" @click="onSelectSsid(item)">
            <div class="mr-2">
              <wifi-icon v-bind="item" />
            </div>

            <div>
              <div class="text-title1">{{ item.ssid }}</div>

              <div class="text-gray-400 text-sm">Security: {{ listEncryption[item.encryptionType] || 'unknown' }}</div>
            </div>
          </div>
        </v-list>

        <div v-if="!scanList.length" class="flex justify-center">
          <v-loader class="text-primary"></v-loader>
        </div>
      </div>

      <template #footer>
        <v-button color="blue" @click="onScan(true)">Scan</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, defineProps, defineEmits, inject, onMounted, nextTick } from 'vue';
import { required, max, min, sameAs, ip } from '@/utils/validate.js';

import { useForm } from '@/composables/useForm.js';

import { DialogKey } from '@/simbol/index.ts';

import type { TypeStateScan, TypeStateSettings, TypelistWiFi, TypeTextFieldFile, TypeTextFieldEvent } from '@/types/types.ts';

interface Props {
  modelValue?: TypeStateSettings;
  scanList: TypeStateScan[];
}

const { modelValue = {}, scanList = [] } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'update:modelValue', value: TypeStateSettings): void;
  (e: 'scan', value: boolean): void;
  (e: 'save', value: TypeStateSettings): void;
  (e: 'reset', value: Event): void;
  (e: 'reboot', value: Event): void;
}>();

const dialog = inject(DialogKey, ({}) => {});

const listMenu = [{ name: 'Save', icon: 'IconSave' }];

const showPass = ref(false);
const showAuthPass = ref(false);
const showDialog = ref(false);

const settings = computed({
  set: (value) => emit('update:modelValue', value),
  get: () => modelValue,
});

const wifiIp = computed({
  set: (value) => value.split('.'),
  get: () => (settings?.value?.wifiIp || []).join('.'),
});
const wifiSubnet = computed({
  set: (value) => value.split('.'),
  get: () => (settings?.value?.wifiSubnet || []).join('.'),
});
const wifiGateway = computed({
  set: (value) => value.split('.'),
  get: () => (settings?.value?.wifiGateway || []).join('.'),
});
const wifiDns = computed({
  set: (value) => value.split('.'),
  get: () => (settings?.value?.wifiDns || []).join('.'),
});
const wifiSsid = computed({
  set: (value) => (settings.value.wifiSsid = value),
  get: () => settings?.value?.wifiSsid,
});
const wifiPass = computed({
  set: (value) => (settings.value.wifiPass = value),
  get: () => settings?.value?.wifiPass,
});
const authLogin = computed({
  set: (value) => (settings.value.authLogin = value),
  get: () => settings?.value?.authLogin,
});
const authPass = computed({
  set: (value) => (settings.value.authPass = value),
  get: () => settings?.value?.authPass,
});

const rePassword = ref(settings.value.wifiPass);
const reAuthPassword = ref(settings.value.authPass);

const form = {
  wifiSsid,
  wifiPass,
  rePassword,

  wifiIp,
  wifiSubnet,
  wifiGateway,
  wifiDns,

  authLogin,
  authPass,
  reAuthPassword,
};

const validators = {
  wifiSsid: { required, max: max(32) },
  wifiPass: {
    required,
    max,
    min,
    sameAs: (value: string) => sameAs(value, rePassword.value || ''),
  },
  rePassword: {
    required,
    max,
    min,
    sameAs: (value: string) => sameAs(value, wifiPass.value || ''),
  },

  wifiIp: { ip },
  wifiSubnet: { ip },
  wifiGateway: { ip },
  wifiDns: { ip },

  authLogin: { required, max: max(12) },
  authPass: { required, max: max(12), sameAs: (value: string) => sameAs(value, reAuthPassword.value || '') },
  reAuthPassword: { required, max: max(12), sameAs: (value: string) => sameAs(value, authPass.value || '') },
};

const { v, invalid, getError } = useForm(validators, form);

const listWiFi: TypelistWiFi[] = [
  { name: 'OFF', value: 0 },
  { name: 'STA', value: 1 },
  { name: 'AP', value: 2 },
  // { name: 'STA + AP', value: 3 },
];

const getMode = computed(() => listWiFi.find((i) => i.value === settings.value.wifiMode)?.name || '');
const isWifiDHCP = computed(() => Boolean(settings.value.wifiDhcp || !settings.value.wifiMode));
const isWifi = computed(() => Boolean(!settings.value.wifiMode));
const isAuth = computed(() => Boolean(!settings.value.authMode));

const onSave = () => emit('save', settings.value);

const onMenu = () => {
  if (invalid.value) return;
  onSave();
};

const listEncryption: string[] = ['OPEN', 'WEP', 'WPA_PSK', 'WPA2_PSK', 'WPA_WPA2_PSK', 'MAX', '', 'NO', 'AUTO'];

const onSelectSsid = ({ ssid }: TypeStateScan) => {
  settings.value.wifiMode = 1;
  settings.value.wifiSsid = ssid;
  const input: HTMLInputElement | null = document.querySelector('#wifiPass input');
  if (input) {
    input.select();
    input.focus();
  }
  onClose();
};

const onClose = () => {
  showDialog.value = false;
};

const onScan = (value: boolean) => {
  showDialog.value = true;
  emit('scan', value);
};

const onChange = (value: number) => (settings.value.wifiMode = value);
const onSureOffWifi = ({ value }: TypelistWiFi) => (!value ? dialog({ value: true, message: 'You are about to disable Wi-Fi. Are you sure?', callback: onChange.bind(this, value) }) : onChange(value));

onMounted(() => {
  rePassword.value = settings.value.wifiPass;
});

const data = { LittleFS: null, firmware: null };

const selectFile = ref<any>(data);

const onUpdateFirmware = (e: TypeTextFieldEvent) => (selectFile.value.firmware = e);
const onUpdateLittleFS = (e: TypeTextFieldEvent) => (selectFile.value.LittleFS = e);
const isDisabledFirmware = computed(() => Boolean(!selectFile.value?.firmware));
const isDisabledLittleFS = computed(() => Boolean(!selectFile.value?.LittleFS));

const getFileNames = (files: TypeTextFieldFile[]) => (files.length ? files.map((i) => `${i.name} (${i.size}) Byte`).join('') : 'Select a file...');
const getName = (name: string) => (selectFile.value?.[name]?.info?.files || []).map((i: TypeTextFieldFile) => `File: ${i.name} <br/> Size: ${i.size} B`).join('');

const onFlash = async (name: string) => {
  if (!selectFile.value?.[name]) return;
  const { files } = selectFile.value[name];
  const date = new FormData();
  for (let i = 0; i < files.length; i++) {
    const file = files.item(i);
    date.append(`file[${i}]`, file, `${name}.bin`);
  }
  if (!files.length) return;
  const res = await (await fetch('/update', { method: 'POST', body: date })).json();
  if (res?.state) dialog({ value: true, title: 'Done', message: 'Reboot...' });
};

const updateFirmware = () => nextTick(() => onFlash('firmware'));
const updateLittleFS = () => nextTick(() => onFlash('LittleFS'));

const onSureFlash = (name: string) =>
  dialog({
    value: true,
    message: `Are you sure you want to update the ${name}? <br/> <p class="mt-2" >${getName(name)}</p>`,
    callback: name === 'firmware' ? updateFirmware : updateLittleFS,
  });
</script>
