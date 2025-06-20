<template>
  <div class="grid gap-4">
    <div class="grid md:grid-cols-2 lg:grid-cols-[100px_300px_100px_100px] gap-4">
      <VTextField v-model="v.slaveID.value" label="Slave ID" :message="getError('slaveID')" @blur="v.slaveID.blur" />
      <VSelect :value="code.name" class="" label="Function Code" :list="functionList" @change="onFunction"></VSelect>
      <VTextField v-model="v.address.value" label="Address" :message="getError('address')" @blur="v.address.blur" />
      <VTextField v-model="v.length.value" label="Length" :message="getError('length')" />
    </div>

    <div>
      <h5 class="mb-2">Request</h5>

      <div class="flex gap-4 items-center">
        <div class="flex">
          <div
            v-for="(item, i) of dataPack"
            :key="i"
            class="flex-[0_0_30px] flex justify-center items-center border-r border-t border-b border-gray-600 h-8 w-full uppercase"
            :class="{ 'border-l': !i }"
          >
            {{ intToHex(item) }}
          </div>
        </div>

        <div>
          <v-button class="w-[100px]" block :disabled="invalid" @click="onSend">Send</v-button>
        </div>
      </div>
    </div>
    {{ v }}
    <div class="grid grid-cols-1">
      <h5 class="mb-2">Response</h5>

      <div class="flex flex-wrap">
        <div
          v-for="(item, i) of getSize"
          :key="item"
          class="flex-[0_0_30px] flex justify-center items-center border-r border-t border-b border-gray-600 h-8 w-full uppercase"
          :class="{ 'border-l': !i }"
        >
          {{ intToHex(getDataByte(i)) }}
        </div>
      </div>
    </div>

    <!-- <VTextField class="col-span-3" hideMessage /> -->
  </div>
</template>

<script setup>
import { ref, defineProps, defineEmits, computed, watch, onMounted } from 'vue';
import { required, maxNumber, minNumber } from '@/utils/validate.js';
import { useForm } from '@/composables/useForm.ts';
import { CRC, getInt16Bytes, functionList, intToHex, getBytesInt16 } from './modbus.js';

import { debounce } from '@/utils/helpers.ts';

const props = defineProps({
  data: { type: Object, default: () => ({}) },
});
const emit = defineEmits(['send']);

const slaveID = ref(1);
const address = ref(0);
const length = ref(1);
const code = ref({ ...functionList[3] });

const data = ref(1);

const packAddress = computed(() => getInt16Bytes(address.value));
const packLength = computed(() => getInt16Bytes(length.value));

const pack = computed(() => [+slaveID.value, code.value.value, ...packAddress.value, ...packLength.value]);
const dataPack = computed(() => [...pack.value, ...getInt16Bytes(CRC(pack.value), true)]);

const form = { slaveID, address, length };

const validators = {
  slaveID: { required, min: minNumber(0), max: maxNumber(255) },
  address: { required, min: minNumber(0), max: maxNumber(65535) },
  length: { required, min: minNumber(1), max: maxNumber(255) },
};

const { v, invalid, getError } = useForm(validators, form);

watch(
  () => props.data,
  (value) => {
    data.value = value;
  }
);

const getData = computed(() => props?.data?.data || []);
const getSize = computed(() => props?.data?.size || []);

const onSend = () => emit('send', dataPack.value);
const getDataByte = (i) => data?.value?.data?.[i] || 0;

const onFunction = (item) => {
  code.value = item;
};

const onBaudRate = (item) => (modbus.value.baudRate = item.value);
const onParity = (item) => (modbus.value.parity = item.value);
const onStopBits = (item) => (modbus.value.stopBits = item.value);

let port;
let reader;
let writer;

const connect = async () => {
  try {
    port = await navigator.serial.requestPort();

    const { usbProductId, usbVendorId } = port.getInfo();
    console.log(usbProductId, usbVendorId);

    await port.open({
      baudRate: 9600,
      bufferSize: 255,
      dataBits: 8,
      flowControl: 'none', // 'hardware'
      parity: 'none', // 'even', 'odd'
      stopBits: 1, // 2,
    });

    setInterval(() => {
      send();
    }, 5000);

    readData();

    log('Успешно подключено!');
  } catch (error) {
    log(`Ошибка подключения: ${error}`);
  }
};

let buffer = [];
const onLoad = debounce(() => {
  console.log(buffer);
}, 300);

const addBuffer = (value) => {
  const regularArray = Array.from(value);
  buffer.push(...regularArray);
  onLoad();
};

const readData = async () => {
  try {
    const decoder = new TextDecoder();
    reader = port.readable.getReader();

    while (true) {
      const { value, done } = await reader.read();
      if (done) break;
      addBuffer(value);
    }
  } catch (error) {
    log(`Ошибка чтения: ${error}`);
  } finally {
    reader?.releaseLock();
  }
};

const send = async () => {
  try {
    if (!writer) writer = port.writable.getWriter();

    const data = new Uint8Array([1, 4, 0, 0, 0, 1, 49, 202]);
    await writer.write(data);

    buffer = [];
  } catch (error) {
    log(`Ошибка отправки: ${error}`);
  }
};

// 3. Чтение данных

const disconnect = async () => {
  try {
    await reader?.cancel();
    await writer?.close();
    await port?.close();
    log('Отключено');
  } catch (error) {
    log(`Ошибка отключения: ${error}`);
  }
};

const log = (message) => {
  console.log(message);
};
const onConnect = async (message) => {
  await connect();
};
</script>
