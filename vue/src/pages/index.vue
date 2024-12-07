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

    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
      <GeneralCard class="flex justify-between col-span-full">
        <h4>Date</h4>

        <div class="flex items-center">
          <input :value="datetime" type="datetime-local" @change="onDate" />
        </div>
      </GeneralCard>

      <VCard>
        <h4 class="mb-6">INPUT</h4>

        <div class="flex flex-wrap gap-2">
          <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="`input_${pin}`">
            <div class="mb-1">{{ findName('input', `input${i + 1}`) }}</div>

            <v-button disabled>{{ getBit(device.input, pin) ? 'OFF' : 'ON' }}</v-button>
          </div>
        </div>
      </VCard>

      <VCard>
        <h4 class="mb-6">OUTPUT</h4>

        <div class="flex flex-wrap gap-2">
          <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="`output_${pin}`">
            <div class="mb-1">{{ findName('output', `output${i + 1}`) }}</div>

            <v-button @click="onSetOutput(pin, !getBit(device.output, pin))">{{ getBit(device.output, pin) ? 'OFF' : 'ON' }}</v-button>
          </div>
        </div>
      </VCard>

      <VCard>
        <h4 class="mb-6">ADC</h4>

        <div class="grid grid-cols-2">
          <div v-for="(pin, i) of 4" :key="`adc_${pin}`" class="flex gap-4">
            <div class="mb-1">{{ findName('adc', `adc${pin}`) }}:</div>
            {{ device[`adc${i + 1}`] }}
          </div>
        </div>
      </VCard>

      <VCard v-if="isDallas">
        <h4 class="mb-6">DS18B20</h4>

        <div class="grid grid-cols-2">
          <div v-for="(ds, key) in dallas" :key="`adc_${key}`">
            <div class="mb-1" :title="key">{{ findName('ds', key) }}:</div>
            {{ ds.temp }} ℃
          </div>
        </div>
      </VCard>

      <VCard>
        <h4 class="mb-6">DAC</h4>

        <div v-for="(pin, i) of 2" :key="`dac_${pin}`" class="flex gap-4">
          <VTextField v-model.number="dac[`dac${i + 1}`]" :label="findName('dac', `dac${pin}`)" />
          <v-button :disabled="isDac(dac[`dac${i + 1}`])" @click="onDac(i + 1, dac[`dac${i + 1}`])">Send</v-button>
        </div>
      </VCard>
    </div>
    <AppDialog size="lg" title="Config" :value="showDialog" @close="onClose">
      <template #footer>
        <v-button @click="onSubmit(true)">Scan</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, inject } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';
import { setBit, getBit, clearBit } from '@/utils/gpio/';
import { getConfig } from '@/utils/fs/';

import event from '@/assets/js/event';

import AppDialog from '@/components/app/AppDialog';
import VCard from '@/components/general/VCard';
import VTextField from '@/components/general/VTextField';
import VDropdown from '@/components/general/VDropdown';
import VList from '@/components/general/VList';

import IconMenu from '@/components/icons/IconMenu';

const config = ref();

const showDialog = ref(false);

const webSocketStore = useWebSocketStore();
const { device, dallas } = storeToRefs(webSocketStore);

const listPage = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Save default' },
];
const listMenu = [{ id: 1, name: 'Save' }];

const overlay = inject('overlay');

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

const onPage = ({ id }) => {
  // if (id === 1) {
  // }
  if (id === 2) {
    onSaveDef();
  }
};
const isDac = value => !(value >= 0 && value <= 255);

const onClose = () => {
  showDialog.value = false;
  overlay.value = false;
};

const onSetOutput = (pin, value) => {
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

const onDate = e => {
  const _now = e?.target?.valueAsNumber;
  if (_now) now.value = _now / 1000;
  webSocketStore.onSend('DEVICE', { now: now.value, command: 1 });
};

const onUploadFile = async () => {
  showDialog.value = true;
  // overlay.value = true;
};

const onSubmit = async e => {
  console.log(e);
};

onMounted(async () => {
  onSend();
  config.value = await getConfig();

  dac.value.dac1 = device.value?.dac1 || 0;
  dac.value.dac2 = device.value?.dac2 || 0;
});
</script>
