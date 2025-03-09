<template>
  <div class="grid gap-4">
    <div class="grid lg:grid-cols-2 gap-4">
      <VCard>
        <!-- <h5 class="mb-6">Setting</h5> -->
        <div class="grid grid-cols-2 gap-4">
          <VSelect :value="modbus.port" label="Port" :list="listBaudRate" @change="onBaudRate"></VSelect>
          <v-button block @click="onConnect">Connect</v-button>
          <VSelect :value="modbus.baudRate" label="Baud rate" :list="listBaudRate" @change="onBaudRate"></VSelect>
          <VSelect :value="modbus.parity" class="col-start-1" label="Parity" :list="listParity" @change="onParity"></VSelect>
          <VSelect :value="modbus.stopBits" label="Stop bits" :list="listStopBits" @change="onStopBits"></VSelect>
        </div>
      </VCard>

      <VCard class="grid gap-4">
        <div class="grid grid-cols-2 gap-4">
          <VTextField v-model="v.slaveID.value" label="Slave ID" :message="getError('slaveID')" @blur="v.slaveID.blur" />
          <VTextField v-model="v.address.value" label="Address" :message="getError('address')" @blur="v.address.blur" />
          <VSelect :value="code.name" class="col-span-2" label="Function Code" :list="functionList" @change="onFunction"></VSelect>
          <VTextField v-model="v.length.value" label="Length" />
        </div>
      </VCard>
    </div>
    {{ packAddress }}
    <VCard class="grid gap-2">
      <div class="grid grid-cols-1">
        <h5 class="mb-2">Request</h5>
        <div class="flex flex-wrap">
          <div v-for="(item, i) of dataPack" :key="i" class="flex-[0_0_30px] flex justify-center items-center border-r border-t border-b h-8 w-full uppercase" :class="{ 'border-l': !i }">
            {{ intToHex(item) }}
          </div>
        </div>
      </div>
      <div class="grid grid-cols-1">
        <h5 class="mb-2">Response</h5>
        <div class="flex flex-wrap">
          <div v-for="(item, i) of getSize" :key="item" class="flex-[0_0_30px] flex justify-center items-center border-r border-t border-b h-8 w-full uppercase" :class="{ 'border-l': !i }">
            {{ intToHex(getDataByte(i)) }}
          </div>
        </div>
      </div>
      <!-- <VTextField class="col-span-3" hideMessage /> -->
      <v-button class="mt-6" block @click="onSend">Send</v-button>
    </VCard>
  </div>
</template>

<script setup>
import { ref, defineProps, defineEmits, computed, watch } from 'vue';
import { rules } from '@/utils/validate/';
import { useForm } from '@/utils/useForm';
import { CRC, getInt16Bytes, functionList, listBaudRate, listParity, listStopBits, intToHex, getBytesInt16 } from './modbus';

import VTextField from '@/components/general/VTextField';
import VSelect from '@/components/general/VSelect';
import VCard from '@/components/general/VCard';

const props = defineProps({
  data: { type: Object, default: () => ({}) },
});
const emit = defineEmits(['send']);

const { required, max, min, sameAs, ip, max12, minValue, maxValue } = rules;

const modbus = ref({
  port: 1,
  baudRate: 9600,
  parity: 'None',
  stopBits: 2,
});

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
  slaveID: { required, min: minValue(0), max: maxValue(65535) },
  address: { required, min: minValue(0), max: maxValue(65535) },
  length: { required, min: minValue(0), max: maxValue(64) },
};

const { v, invalid, getError } = useForm(validators, form);

// const code = ref(0);

watch(
  () => props.data,
  value => {
    data.value = value;
  }
);

const getData = computed(() => props?.data?.data || []);
const getSize = computed(() => props?.data?.size || []);

const onSend = () => emit('send', dataPack.value);
const getDataByte = i => data?.value?.data?.[i] || 0;

const onFunction = item => {
  code.value = item;
};

const onBaudRate = item => (modbus.value.baudRate = item.value);
const onParity = item => (modbus.value.parity = item.value);
const onStopBits = item => (modbus.value.stopBits = item.value);
</script>
