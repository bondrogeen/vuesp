<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12 text-h2 mb-6">Main</div>

      <div class="col sm12 text-h2 mb-6">
        <input v-model="datetime" class="page-main__date" type="datetime-local" @change="onDate" />
      </div>

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
                <div class="text-small mb-1">{{ getName('input', i) }}</div>
                <v-button disabled class="">{{ getBit(device.input, pin) ? 'OFF' : 'ON' }}</v-button>
              </div>
            </div>
          </div>

          <div>
            <h5 class="text-body-1 mb-6">OUTPUT</h5>
            <div class="d-flex flex-wrap gap-2">
              <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="pin">
                <div class="text-small mb-1">{{ getName('output', i) }}</div>
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
    {{ config }}
    <AppDialog size="lg" title="Config" :value="showDialog" @close="onClose">
      <AppConfig :config="config" @submit="onSubmit"></AppConfig>
      <template #footer>
        <v-button @click="onSubmit(true)">Scan</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup>
import { nextTick, onMounted, ref, watch, inject } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';
import { setBit, getBit, clearBit } from '@/utils/gpio/';
import { getFileJSON, onUploadBinary } from '@/utils/fs/';

import event from '@/assets/js/event';

import AppDialog from '@/components/app/AppDialog';
import AppConfig from '@/components/app/AppConfig';

const config = ref({
  input: [{ name: 'input 1' }, { name: 'input 2' }, { name: 'input 3' }, { name: 'input 4' }, { name: 'input 5' }, { name: 'input 6' }],
  output: [{ name: 'output 1' }, { name: 'output 2' }, { name: 'output 3' }, { name: 'output 4' }, { name: 'output 5' }, { name: 'output 6' }],
});

const showDialog = ref(false);
const path = '/device/data.json';

const webSocketStore = useWebSocketStore();
const { device } = storeToRefs(webSocketStore);

const listMenu = [
  { id: 1, name: 'Download' },
  { id: 2, name: 'Remove' },
];

const overlay = inject('overlay');

const text = ref('start');
const logs = ref([]);
const datetime = ref('');

const now = ref(0);

event.on('init', () => {
  webSocketStore.onSend('DEVICE');
});

const getName = (key, i) => config?.value?.[key]?.[i]?.name || `${key} ${i}`;

const onClose = () => {
  showDialog.value = false;
  overlay.value = false;
};

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
  showDialog.value = true;
  // overlay.value = true;
};

const onSubmit = async e => {
  console.log(e);
};

onMounted(async () => {
  onSend();
  const res = await getFileJSON(path);
  if (res.ok) {
    config.value = res;
  }

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
