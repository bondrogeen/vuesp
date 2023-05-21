<template>
  <div class="service-gpio">
    {{ title }}
    <div v-for="pin in ports" :key="pin.gpio">
      <div v-if="pin" class="d-flex">
        <v-select class="service-gpio__select" :value="getMode(pin)" :label="`GPIO: ${pin.gpio}`" :list="listMode" @change="onMode(pin, $event)"></v-select>
        <v-checkbox :model-value="getValue(pin)" :disabled="true" @update:model-value="onSetPort(pin, $event)"></v-checkbox>

        <!-- <div class="mr-2">{{ pin }}</div> -->
        <div>{{ getData(pin.data) }}</div>
      </div>
    </div>
    <div class="row mt-6">
      <div class="col sm12 d-flex j-end">
        <v-button class="mr-4" @click="onGetPort">Update</v-button>
        <v-button @click="onSave">Save</v-button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { defineProps, defineEmits, computed, onMounted, ref } from 'vue';
import { getBinary, onUploadBinary } from '@/utils/fs/';
import { command, getKey, getData, setData, parseDateGPIO, stringifyDateGPIO } from '@/utils/gpio/';

const props = defineProps({
  pins: { type: Object, default: () => ({}) },
});

const emit = defineEmits(['click', 'send']);

const ports = ref([]);

const listMode = [
  { name: 'Input', value: 0 },
  { name: 'Output', value: 1 },
];

const title = computed(() => props.name || '');

const onMode = (port, item) => {
  const obj = getData(port.data);
  obj.mode = item.value;
  port.data = setData(obj);
};

const getMode = ({ data }) => {
  const mode = getKey(data, 'mode');
  return listMode.find(i => i.value === mode)?.name || '';
};

const getValue = ({ data }) => {
  const mode = getKey(data, 'value');
  return Boolean(mode);
};

const onSetPort = (port, value) => {
  const obj = getData(port.data);
  obj.value = value;
  port.data = setData(obj);

  emit('send', { comm: 'PORT', data: { gpio: port.gpio, command: command.GPIO_COMMAND_SET, data: port.data } });
};

const onGetPort = port => {
  console.log(port);
  emit('send', { comm: 'PORT', data: { command: command.GPIO_COMMAND_GET_ALL } });
};

const onLoadDataGpio = async () => {
  const array = await getBinary('/gpio.io');
  return parseDateGPIO(array);
};

const onSave = async () => {
  const data = stringifyDateGPIO(ports.value);
  const buffer = new Uint8Array(data).buffer;
  const res = await onUploadBinary('/gpio.io', buffer);
  console.log(res);
};

onMounted(async () => {
  ports.value = await onLoadDataGpio();
  console.log(ports.value);
});
</script>

<style lang="scss">
.service-gpio {
  position: relative;
  &__select {
    max-width: 200px;
  }
}
</style>
