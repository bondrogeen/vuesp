<template>
  <div>
    <div class="mb-6 flex items-center justify-between">
      <h1>Main</h1>

      <div class="v-spacer"></div>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-dots class="rotate-90"></icon-dots>
          </v-button>
        </template>
        <VList :list="listPage" @click="onPage"></VList>
      </v-dropdown>
    </div>

    {{ device }}
    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-5 gap-4">
      <VCardGray class="flex justify-between col-span-full" title="Date">
        <div class="flex items-center">
          <input :value="datetime" type="datetime-local" @change="onDate" />
        </div>
      </VCardGray>

      <VCardGray title="INPUT">
        <div class="grid grid-cols-2 lg:grid-cols-3 gap-2">
          <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="`input_${pin}`">
            <div class="text-body mb-1 text-gray-600">{{ findName('input', `input${i + 1}`) }}</div>

            <v-button block disabled>{{ getBit(device.input, pin) ? 'OFF' : 'ON' }}</v-button>
          </div>
        </div>
      </VCardGray>

      <VCardGray title="OUTPUT">
        <div class="grid grid-cols-2 lg:grid-cols-3 gap-2">
          <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="`output_${pin}`">
            <div class="text-body mb-1 text-gray-600">{{ findName('output', `output${i + 1}`) }}</div>

            <v-button block @click="onSetOutput(pin, !getBit(device.output, pin))">{{ getBit(device.output, pin) ? 'OFF' : 'ON' }}</v-button>
          </div>
        </div>
      </VCardGray>

      <VCardGray title="ADC">
        <div class="grid gap-2 grid-cols-2">
          <div v-for="pin of 4" :key="`adc_${pin}`">
            <span class="text-body text-gray-600 mr-2">{{ findName('adc', `adc${pin}`) }}:</span>
            <span class="font-bold">{{ findValue('adc', `adc${pin}`) }}</span>
          </div>
        </div>
      </VCardGray>

      <VCardGray v-if="isDallas" title="DS18B20">
        <div class="grid gap-2 grid-cols-1">
          <div v-for="(ds, key) in dallas" :key="`adc_${key}`">
            <span class="text-body text-gray-600 mr-2" :title="key">{{ findName('ds', key) }}:</span>
            <span class="font-bold">{{ ds.temp.toFixed(2) }} ℃</span>
          </div>
        </div>
      </VCardGray>

      <VCardGray title="DAC">
        <div class="grid grid-cols-1 gap-4">
          <div v-for="(pin, i) of 2" :key="`dac_${pin}`" class="grid gap-2 grid-cols-4">
            <VTextField v-model="dac[`dac${i + 1}`]" class="col-span-3" :label="findName('dac', `dac${pin}`)" hideMessage />

            <v-button block :disabled="isDac(dac[`dac${i + 1}`])" @click="onDac(i + 1, dac[`dac${i + 1}`])">Send</v-button>
          </div>
        </div>
      </VCardGray>

      <!-- <VCardGray>
        <h5 class="mb-6">Electric counter</h5>
        <div class="grid gap-2 grid-cols-1">
          <div v-for="(value, key) in getModbus(modbus)" :key="`modbus_${key}`">
            <span class="text-body text-gray-600 mr-2">{{ key }}:</span>

            <span class="font-bold">{{ value }}</span>
          </div>
        </div>
      </VCardGray> -->
    </div>

    <!-- <div class="mt-4">
      <VCardGray>
        <h5 class="mb-6">Modbus (master)</h5>
        <div class="grid grid-cols-1 gap-4">
          <BlockModbus :data="modbus" @send="onSendModBus" />
        </div>
      </VCardGray>
    </div> -->
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

import BlockModbus from '@/components/blocks/modbus/BlockModbus.vue';

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

const isDac = (value) => !(value >= 0 && value <= 255);

const getModbus = ({ voltage, power, frequency, current, cos }) => ({ voltage, power, frequency, current, cos });

const onSetOutput = (pin, value) => {
  // notification({ text: 'add' });
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
  webSocketStore.onSend('DEVICE');
};

const onSendModBus = (data) => {
  webSocketStore.onSend('MODBUS', { command: 1, data, size: data.length });
};

const onDate = (e) => {
  const _now = e?.target?.valueAsNumber;
  if (_now) now.value = _now / 1000;
  webSocketStore.onSend('DEVICE', { now: now.value, command: 1 });
};

onMounted(async () => {
  setTimeout(() => {
    onSend();
  }, 2000);
  // config.value = await getConfig();

  // dac.value.dac1 = device.value?.dac1 || 0;
  // dac.value.dac2 = device.value?.dac2 || 0;

  // const res = await fetch(`/get`, { method: 'GET' });
  // const content = await res.blob();
});
</script>
