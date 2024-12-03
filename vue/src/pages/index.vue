<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12 mb-6 d-flex a-center">
        <h1 class="text-h2">Main</h1>

        <div class="v-spacer"></div>

        <v-dropdown right="0" left="unset" top="0">
          <template #activator="{ on }">
            <v-icons icon="menu" @click="on.click"></v-icons>
          </template>
          <v-list :list="listPage" @click="onPage"></v-list>
        </v-dropdown>
      </div>
      {{ device }}

      <div class="col sm12 mb-4">
        <div class="page-main__card pa-4 d-flex j-between">
          <h2 class="text-h4">Date</h2>

          <div class="d-flex a-center">
            <input :value="datetime" class="page-main__date mr-4" type="datetime-local" @change="onDate" />
          </div>
        </div>
      </div>

      <div class="col sm12 lg8 mb-4">
        <div class="page-main__card pa-4">
          <h2 class="mb-6">GPIO</h2>

          <div>
            <h3 class="text-body-1 mb-6">INPUT</h3>

            <div class="d-flex flex-wrap gap-2 mb-8">
              <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="`input_${pin}`">
                <div class="text-small mb-1">{{ getName('input', i) }}</div>

                <v-button disabled class="">{{ getBit(device.input, pin) ? 'OFF' : 'ON' }}</v-button>
              </div>
            </div>
          </div>

          <div>
            <h3 class="text-body-1 mb-6">OUTPUT</h3>

            <div class="d-flex flex-wrap gap-2">
              <div v-for="(pin, i) of [1, 2, 4, 8, 16, 32]" :key="`output_${pin}`">
                <div class="text-small mb-1">{{ getName('output', i) }}</div>

                <v-button class="" @click="onSetOutput(pin, !getBit(device.output, pin))">{{ getBit(device.output, pin) ? 'OFF' : 'ON' }}</v-button>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div class="col sm12 lg4">
        <div class="page-main__card pa-4">
          <h2 class="mb-6">ADC</h2>

          <div class="row">
            <div v-for="(pin, i) of 4" :key="`adc_${pin}`" class="d-flex gap-4 col sm12 md6 lg6">
              <div class="text-small mb-1">{{ getName('adc', i) }}:</div>
              {{ device[`adc${i + 1}`] }}
            </div>
          </div>
        </div>

        <div class="page-main__card pa-4 mt-4">
          <h2 class="mb-6">DAC</h2>

          <div v-for="(pin, i) of 2" :key="`dac_${pin}`" class="d-flex gap-4">
            <v-input v-model.number="dac[`dac${i + 1}`]" :label="getName('dac', i)" />
            <v-button class="" :disabled="isDac(dac[`dac${i + 1}`])" @click="onDac(i + 1, dac[`dac${i + 1}`])">Send</v-button>
          </div>
        </div>
      </div>
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

const config = ref();

const showDialog = ref(false);

const webSocketStore = useWebSocketStore();
const { device } = storeToRefs(webSocketStore);

const listPage = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Save default' },
];
const listMenu = [{ id: 1, name: 'Save' }];

const overlay = inject('overlay');

const datetime = computed(() => new Date((device.value.now || 0) * 1000).toISOString().slice(0, 16));

const now = ref(0);

const dac = ref({ dac1: 0, dac2: 0 });

event.on('init', () => {
  webSocketStore.onSend('DEVICE');
});

const getName = (key, i) => config?.value?.[key]?.[i]?.name || `${key} ${i}`;

const onPage = item => {
  console.log(item);
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

<style lang="scss">
.page-main {
  position: relative;

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
