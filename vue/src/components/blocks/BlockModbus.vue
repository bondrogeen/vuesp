<template>
  <div>
    <div class="grid grid-cols-3 gap-4 mb-4">
      <VTextField label="Baud rate" :disabled="isWifiDHCP" />
      <VTextField label="Data bits" :disabled="isWifiDHCP" />
      <VTextField label="Parity" :disabled="isWifiDHCP" />

      <VSelect label="Parity" :list="listParity" @change="onFunction"></VSelect>
      <VSelect label="Stop bits" :list="[1, 2]" @change="onFunction"></VSelect>
    </div>
    <div class="grid grid-cols-3 gap-4 mb-4">
      <VTextField v-model="v.slaveID.value" label="Slave ID" :message="getError('slaveID')" @blur="v.slaveID.blur" />
      <VTextField v-model="v.address.value" label="Address" :message="getError('address')" @blur="v.address.blur" />
      <VSelect :value="code.name" label="Function Code" :list="functionList" @change="onFunction"></VSelect>
      <VTextField v-model="v.length.value" label="Length" />
    </div>
    <div class="flex gap-2">
      <div v-for="(item, i) of arr" :key="item" class="flex-[0_0_40px] flex flex-col">
        <div class="flex justify-center border w-full mb-2 rounded-md">
          {{ i }}
        </div>
        <div class="flex justify-center border w-full uppercase">
          {{ item.toString(16) }}
        </div>
      </div>
    </div>
    {{ dataPack.map(i => i.toString(16)) }}

    <!-- <VTextField class="col-span-3" hideMessage /> -->
    <v-button class="mt-6" block @click="onSend">Send</v-button>
  </div>
</template>

<script setup>
import { ref, defineProps, defineEmits, computed } from 'vue';
import { rules } from '@/utils/validate/';
import { useForm } from '@/utils/useForm';
import { CRC, getInt16Bytes } from '@/utils/modbus';

import VTextField from '@/components/general/VTextField';
import VSelect from '@/components/general/VSelect';

defineProps({
  modelValue: { type: [Boolean, Number], default: false },
  disabled: { type: Boolean, default: false },
});
const emit = defineEmits(['send']);

const functionList = [
  { name: 'Read Coil Status 0x01', type: '', access: 'read', value: 0x01 },
  { name: 'Read Input Status 0x02', type: '', access: 'read', value: 0x02 },
  { name: 'Read Holding Registers 0x03', type: '', access: 'read', value: 0x03 },
  { name: 'Read Input Registers 0x04', type: '', access: 'read', value: 0x04 },
  { name: 'Force Single Coil 0x05', type: '', access: 'write', value: 0x05 },
  { name: 'Preset Single Register 0x06', type: '', access: 'write', value: 0x06 },
  { name: 'Force Multiple Coils 0x0f', type: '', access: 'write', value: 0x0f },
  { name: 'Preset Multiple Registers 0x10', type: '', access: 'write', value: 0x10 },
];

const listParity = [
  { name: 'None', value: 'N' },
  { name: 'Event', value: 'E' },
  { name: 'Odd', value: 'o' },
];

const { required, max, min, sameAs, ip, max12, minValue, maxValue } = rules;

const slaveID = ref(1);
const address = ref(0);
const length = ref(1);
const code = ref({ ...functionList[3] });

// var calculatedCrc = CRC([0x01, 0x04, 0x00, 0x00, 0x00, 0x01]);

const arr = computed(() => [+slaveID.value, code.value.value, 0x00, 0x00, 0x00, 0x01, 0x31, 0xca]);

const pack = computed(() => [+slaveID.value, code.value.value, ...getInt16Bytes(address.value), ...getInt16Bytes(length.value)]);

const dataPack = computed(() => [...pack.value, ...getInt16Bytes(CRC(pack.value), true)]);

console.log(dataPack.value.toString(16));

const form = {
  slaveID,
  address,
  length,
};

const validators = {
  slaveID: { required, min: minValue(0), max: maxValue(255) },
  address: { required, min: minValue(0), max: maxValue(255) },
  length: { required, min: minValue(0), max: maxValue(64) },
};

const { v, invalid, getError } = useForm(validators, form);

// const code = ref(0);

const onSend = () => emit('send', [0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xca]);

const onFunction = item => {
  code.value = item;
};
</script>
