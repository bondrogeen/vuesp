<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12 text-h2 mb-6">Main</div>

      <div class="col sm12 text-h2 mb-6">
        <input v-model="datetime" class="page-main__date" type="datetime-local" @change="onDate" />
      </div>
      {{ device }}

      <div class="col sm12 md8 mb-6">
        <div class="page-main__card pa-4">
          <div class="d-flex j-between">
            <h2 class="mb-6">GPIO</h2>
            <v-dropdown right="0" left="unset" top="0">
              <template #activator="{ on }">
                <v-icons icon="menu" @click="on.click"></v-icons>
              </template>
              <v-list :list="listMenu" @click="onUploadFile"></v-list>
            </v-dropdown>
          </div>

          <div>
            <h5 class="text-body-1 mb-6">INPUT</h5>
            <div class="d-flex flex-wrap gap-2 mb-8">
              <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="pin">
                <div class="text-small mb-1">{{ `Input ${i + 1}` }}</div>
                <v-button disabled class="">{{ getBit(device.input, pin) ? 'OFF' : 'ON' }}</v-button>
              </div>
            </div>
          </div>

          <div>
            <h5 class="text-body-1 mb-6">OUTPUT</h5>
            <div class="d-flex flex-wrap gap-2">
              <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="pin">
                <div class="text-small mb-1">{{ `Output ${i + 1}` }}</div>
                <v-button class="" @click="onSetOutput(pin, !getBit(device.output, pin))">{{ getBit(device.output, pin) ? 'OFF' : 'ON' }}</v-button>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div class="col sm12 md4 mb-6">
        <div class="page-main__card pa-4">
          <div class="d-flex j-between">
            <h2 class="mb-6">ADC</h2>
            <v-dropdown right="0" left="unset" top="0">
              <template #activator="{ on }">
                <v-icons icon="menu" @click="on.click"></v-icons>
              </template>
              <v-list :list="listMenu" @click="onEventService"></v-list>
            </v-dropdown>
          </div>

          <div v-for="(pin, i) of 4" :key="pin" class="d-flex gap-4">
            <div class="text-small mb-1">{{ `ADC${i + 1}` }}:</div>
            {{ device[`adc${i + 1}`] }}
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { nextTick, onMounted, ref, watch } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';
import { setBit, getBit, clearBit } from '@/utils/gpio/';
import { getFile, onUploadBinary } from '@/utils/fs/';

import event from '@/assets/js/event';

const path = '/data.json';

const webSocketStore = useWebSocketStore();
const { device } = storeToRefs(webSocketStore);

const listMenu = [
  { id: 1, name: 'Download' },
  { id: 2, name: 'Remove' },
];

const text = ref('start');
const logs = ref([]);
const datetime = ref('');

const now = ref(0);

event.on('init', () => {
  webSocketStore.onSend('DEVICE');
});

const onSetOutput = (pin, value) => {
  const byte = device.value.output;
  device.value.output = !value ? clearBit(byte, pin) : setBit(byte, pin);
  webSocketStore.onSend('DEVICE', { ...device.value });
};

const onSend = () => {
  webSocketStore.onSend('DEVICE', { now: now.value, direction: 0 });
  text.value = '';
};
const onDate = e => {
  const _now = e?.target?.valueAsNumber;
  if (_now) now.value = _now / 1000;
  webSocketStore.onSend('DEVICE', { now: now.value, direction: 0 });
};

const onScrollEnd = () => {
  nextTick(() => {
    const el = document.querySelector('.page-main__list');
    if (el) {
      el.scrollTo(0, el.scrollHeight);
    }
  });
};

watch(
  () => device.value,
  value => {
    logs.value.push(value);
    onScrollEnd();
  }
);

const onUploadFile = async () => {
  await onUploadBinary(path, {});
};

onMounted(async () => {
  onSend();

  const res = await getFile(path);
  console.log(res);
});
</script>

<style lang="scss">
.page-main {
  position: relative;
  &__list {
    height: 400px;
    overflow: auto;
  }
  &__item {
    border-bottom: 1px solid var(--border-1);
  }
  &__time {
    min-width: 150px;
  }
  &__date {
    font-size: 16px;
    height: 32px;
  }
  &__card {
    border: 1px solid, var(--primary);
    border-radius: 16px;
  }
}
</style>
