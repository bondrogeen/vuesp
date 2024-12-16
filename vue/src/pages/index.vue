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

      <VCard class="col-span-full">
        <h5 class="mb-6">GPIO</h5>

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
import { computed, onMounted, ref, inject } from 'vue';
import { useRouter } from 'vue-router';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';
import { setBit, getBit, clearBit } from '@/utils/gpio/';
import { getConfig } from '@/utils/fs/';

import { getBinary, onUploadBinary } from '@/utils/fs/';
import { command, getKey, getData, setData, parseDateGPIO, stringifyDateGPIO } from '@/utils/gpio/';
import { pathGPIO } from '@/utils/const';

const webSocketStore = useWebSocketStore();
const { device, gpio } = storeToRefs(webSocketStore);

const notification = inject('notification');

import event from '@/assets/js/event';

import VCard from '@/components/general/VCard';
import VTextField from '@/components/general/VTextField';
import VDropdown from '@/components/general/VDropdown';
import VList from '@/components/general/VList';

import IconMenu from '@/components/icons/IconMenu';

const router = useRouter();

const config = ref();

const listPage = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Save default' },
];

const datetime = computed(() => new Date((device.value.now || 0) * 1000).toISOString().slice(0, 16));

const now = ref(0);
const ports = ref([]);
const portsDef = ref([]);

event.on('init', () => {
  webSocketStore.onSend('DEVICE');
});

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

const onMode = (port, item) => {
  const obj = getData(port.data);
  const value = item.value;
  obj.init = value & 0b00001111 ? 1 : 0;
  obj.mode = value & 0b00000111;
  port.data = setData(obj);
};

const getMode = ({ data }) => {
  const mode = getKey(data, 'mode');
  const init = getKey(data, 'init');
  const value = init * 8 + mode;
  return listMode.find(i => i.value === value) || {};
};

const getModeName = pin => getMode(pin).name;

const getValue = ({ data }) => Boolean(getKey(data, 'value'));

const getStateValue = pin => {
  return getValue(gpio?.value?.[pin.gpio] || {});
};

const isDisabled = pin => Boolean(![9, 11].includes(getMode(pin).value));

const onSetPort = (port, value) => {
  console.log(port, value);

  const obj = getData(port.data);
  obj.value = value;
  port.data = setData(obj);

  webSocketStore.onSend('PORT', { gpio: port.gpio, command: command.GPIO_COMMAND_SET, data: port.data });
};

const onSetOutput = (pin, value) => {
  notification({ text: 'add' });
  const byte = device.value.output;
  device.value.output = !value ? clearBit(byte, pin) : setBit(byte, pin);
  webSocketStore.onSend('DEVICE', { ...device.value, command: 2 });
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

const onLoadDataGpio = async () => {
  const array = await getBinary(pathGPIO);
  return parseDateGPIO(array);
};

const onGetPort = () => {
  webSocketStore.onSend('PORT', { command: command.GPIO_COMMAND_GET_ALL });
};

onMounted(async () => {
  onGetPort();
  ports.value = await onLoadDataGpio();
  portsDef.value = JSON.parse(JSON.stringify(ports.value));

  onSend();
});
</script>
