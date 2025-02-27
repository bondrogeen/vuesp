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
      <VCard class="flex col-span-full gap-4">
        <h5>Temperature:</h5>

        <div class="flex items-center">{{ device.temp }} °C</div>
      </VCard>

      <VCard class="col-span-full">
        <h5 class="mb-4">Brightness</h5>
        <div class="flex flex-col md:flex-row gap-4 md:items-center">
          <VRange class="md:w-1/2" :modelValue="device.light" min="0" max="254" @change="onLight" />
          <v-button @click="onChangeState">{{ device.state ? 'ON' : 'OFF' }}</v-button>
        </div>

        <div class="flex flex-col gap-2">
          <div v-for="pin in ports" :key="pin.gpio">
            <div v-if="pin" class="flex justify-between">
              PIN: {{ pin.gpio }}
              
              <VSelect class="max-w-[250px]" :value="getModeName(pin)" :label="`GPIO: ${pin.gpio}`" :list="listMode" @change="onMode(pin, $event)" />

              <v-button class="min-w-[100px] ml-2" :disabled="isDisabled(pin)" @click="onSetPort(pin, !getStateValue(pin))">{{ getStateValue(pin) ? 'ON' : 'OFF' }}</v-button>
            </div>
          </div>
        </div>
      </VCard>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from 'vue';
import { useRouter } from 'vue-router';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import { getBinary } from '@/utils/fs/';
import { getKey, parseDateGPIO } from '@/utils/gpio/';
import { pathGPIO } from '@/utils/const';

import VSelect from '@/components/general/VSelect';
import VRange from '@/components/general/VRange';

const webSocketStore = useWebSocketStore();
const { device, gpio } = storeToRefs(webSocketStore);

import event from '@/assets/js/event';

import VCard from '@/components/general/VCard';
import VDropdown from '@/components/general/VDropdown';
import VList from '@/components/general/VList';

import IconMenu from '@/components/icons/IconMenu';

const router = useRouter();

const listPage = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Save default' },
];

const datetime = computed(() => new Date((device.value.now || 0) * 1000).toISOString().slice(0, 16));

const now = ref(0);
const ports = ref([]);
const portsDef = ref([]);

const init = async () => {
  webSocketStore.onSend('DEVICE');
  webSocketStore.onSend('PORT');

  ports.value = await onLoadDataGpio();
  portsDef.value = JSON.parse(JSON.stringify(ports.value));
};

event.on('init', init);

const listMode = [
  { name: 'OFF', value: 0 },
  { name: 'INPUT', value: 8 }, // 0x00
  { name: 'OUTPUT', value: 9 }, // 0x01
  { name: 'INPUT_PULLUP', value: 10 }, // 0x02
  { name: 'OUTPUT_OPEN_DRAIN', value: 11 }, // 0x03
  // { name: 'INPUT_PULLDOWN_16', value: 12 }, // 0x04
  // { name: 'WAKEUP_PULLUP', value: 13 }, // 0x05
  // { name: 'WAKEUP_PULLDOWN', value: 15 }, // 0x07
];

const onPage = ({ id }) => {
  if (id === 1) {
    router.push('/config');
  }
  if (id === 2) {
    onSaveDef();
  }
};

const getMode = ({ data }) => {
  const mode = getKey(data, 'mode');
  const init = getKey(data, 'init');
  const value = init * 8 + mode;
  return listMode.find(i => i.value === value) || {};
};

const onSaveDef = () => {
  webSocketStore.onSend('DEVICE', { ...device.value, command: 1 });
};

const onDate = e => {
  const _now = e?.target?.valueAsNumber;
  if (_now) now.value = _now / 1000;
  webSocketStore.onSend('DEVICE', { now: now.value, command: 3 });
};

const onLight = e => {
  const value = e?.target?.valueAsNumber;
  webSocketStore.onSend('DEVICE', { light: value, command: 2 });
};
const onChangeState = e => {
  device.value.state = device.value.state ? 0 : 1;
  webSocketStore.onSend('DEVICE', { state: device.value.state, command: 2 });
};

const onLoadDataGpio = async () => {
  const array = await getBinary(pathGPIO);
  return parseDateGPIO(array);
};

onMounted(async () => {
  await init();
});
</script>
