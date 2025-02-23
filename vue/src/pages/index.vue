<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Main</h1>

      <div class="v-spacer"></div>

      <VDropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <IconMenu @click="on.click"></IconMenu>
        </template>
        <VList :list="listPage" @click="onPage"></VList>
      </VDropdown>
    </div>

    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-5 gap-4">
      <VCard class="flex justify-between col-span-full">
        <h5>Date</h5>

        <div class="flex items-center">
          <input :value="datetime" type="datetime-local" @change="onDate" />
        </div>
      </VCard>

      <VCard>
        <h5 class="mb-6">INPUT</h5>

        <div class="grid grid-cols-2 lg:grid-cols-3 gap-2">
          <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="`input_${pin}`">
            <div class="text-body mb-1 text-gray-600">{{ findName('input', `input${i + 1}`) }}</div>

            <v-button block disabled>{{ getBit(device.input, pin) ? 'OFF' : 'ON' }}</v-button>
          </div>
        </div>
      </VCard>

      <VCard>
        <h5 class="mb-6">OUTPUT</h5>

        <div class="grid grid-cols-2 lg:grid-cols-3 gap-2">
          <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="`output_${pin}`">
            <div class="text-body mb-1 text-gray-600">{{ findName('output', `output${i + 1}`) }}</div>

            <v-button block @click="onSetOutput(pin, !getBit(device.output, pin))">{{ getBit(device.output, pin) ? 'OFF' : 'ON' }}</v-button>
          </div>
        </div>
      </VCard>

      <VCard>
        <h5 class="mb-6">ADC</h5>

        <div class="grid gap-2 grid-cols-2">
          <div v-for="pin of 4" :key="`adc_${pin}`">
            <span class="text-body text-gray-600 mr-2">{{ findName('adc', `adc${pin}`) }}:</span>
            <span class="font-bold">{{ findValue('adc', `adc${pin}`) }}</span>
          </div>
        </div>
      </VCard>

      <VCard v-if="isDallas">
        <h5 class="mb-6">DS18B20</h5>

        <div class="grid gap-2 grid-cols-1">
          <div v-for="(ds, key) in dallas" :key="`adc_${key}`">
            <span class="text-body text-gray-600 mr-2" :title="key">{{ findName('ds', key) }}:</span>
            <span class="font-bold">{{ ds.temp.toFixed(2) }} ℃</span>
          </div>
        </div>
      </VCard>

      <VCard>
        <h5 class="mb-6">DAC</h5>
        <div class="grid grid-cols-1 gap-4">
          <div v-for="(pin, i) of 2" :key="`dac_${pin}`" class="grid gap-2 grid-cols-4">
            <VTextField v-model="dac[`dac${i + 1}`]" class="col-span-3" :label="findName('dac', `dac${pin}`)" hideMessage />

            <v-button block :disabled="isDac(dac[`dac${i + 1}`])" @click="onDac(i + 1, dac[`dac${i + 1}`])">Send</v-button>
          </div>
        </div>
      </VCard>

      <VCard>
        <h5 class="mb-6">Modbus</h5>
        <div class="grid grid-cols-1 gap-4">
          <VTextField class="col-span-3" hideMessage />
          <v-button block @click="onSendModbus">Send</v-button>
        </div>
      </VCard>

      <VCard>
        <h5 class="mb-6">Electric counter</h5>
        <div class="grid gap-2 grid-cols-1">
          <div v-for="(value, key) in getModbus(modbus)" :key="`modbus_${key}`">
            <span class="text-body text-gray-600 mr-2">{{ key }}:</span>

            <span class="font-bold">{{ value }}</span>
          </div>
        </div>
      </VCard>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, inject } from 'vue';
import { useRouter } from 'vue-router';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';
import { setBit, getBit, clearBit } from '@/utils/gpio/';
import { getConfig } from '@/utils/fs/';

const notification = inject('notification');

import event from '@/assets/js/event';

import VCard from '@/components/general/VCard';
import VTextField from '@/components/general/VTextField';
import VDropdown from '@/components/general/VDropdown';
import VList from '@/components/general/VList';

import IconMenu from '@/components/icons/IconMenu';

const router = useRouter();

const config = ref();

const webSocketStore = useWebSocketStore();
const { device, dallas, modbus } = storeToRefs(webSocketStore);

const listPage = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Save default' },
];

const datetime = computed(() => new Date((device.value.now || 0) * 1000).toISOString().slice(0, 16));

const isDallas = computed(() => Boolean(Object.values(dallas?.value)?.length));

const now = ref(0);

const dac = ref({ dac1: 0, dac2: 0 });

event.on('init', () => {
  webSocketStore.onSend('DEVICE');
});

const findName = (name, key) => {
  const value = config?.value?.[name]?.[key]?.name;
  return typeof value === 'undefined' ? `${key}` : value;
};
const findValue = (name, key) => {
  const f = config?.value?.[name]?.[key]?.fun;
  const value = device?.value?.[key];
  return f ? eval(f)(value) : value;
};

const onPage = ({ id }) => {
  if (id === 1) {
    router.push('/config');
  }
  if (id === 2) {
    onSaveDef();
  }
};

const isDac = value => !(value >= 0 && value <= 255);

const getModbus = ({ voltage, power, frequency, current, cos }) => ({ voltage, power, frequency, current, cos });

const onSetOutput = (pin, value) => {
  notification({ text: 'add' });
  const byte = device.value.output;
  device.value.output = !value ? clearBit(byte, pin) : setBit(byte, pin);
  webSocketStore.onSend('DEVICE', { ...device.value, command: 2 });
};

const onDac = (i, value) => {
  console.log(i, value);
  device.value[`dac${i}`] = value;
  webSocketStore.onSend('DEVICE', { ...device.value, command: 3 });
};
const onSaveDef = () => {
  webSocketStore.onSend('DEVICE', { ...device.value, command: 4 });
};

const onSend = () => {
  webSocketStore.onSend('DEVICE', { now: now.value, command: 0 });
};

const onSendModbus = () => {
  const data = [0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xca];
  webSocketStore.onSend('MODBUS', { command: 1, data, size: data.length });
};

const onDate = e => {
  const _now = e?.target?.valueAsNumber;
  if (_now) now.value = _now / 1000;
  webSocketStore.onSend('DEVICE', { now: now.value, command: 1 });
};

onMounted(async () => {
  onSend();
  config.value = await getConfig();

  dac.value.dac1 = device.value?.dac1 || 0;
  dac.value.dac2 = device.value?.dac2 || 0;

  // const res = await fetch(`/get`, { method: 'GET' });
  // const content = await res.blob();
});
</script>
