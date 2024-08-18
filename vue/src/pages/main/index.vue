<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12 text-h2 mb-6">Main</div>
      <div class="col sm12 text-h2 mb-6">
        <input v-model="datetime" class="page-main__date" type="datetime-local" @change="onDate" />
      </div>
      <div class="col sm12 mb-6">
        <h3 class="mb-4">Inputs</h3>

        <ul class="page-main__ports">
          <li v-for="gpio of 16" :key="gpio">
            <div :class="['page-main__port', { 'page-main__port--active': getGPIOIn(gpio - 1) }]"></div>
            <span class="text-caption fw-700 d-flex j-center">IN {{ gpio }}</span>
          </li>
        </ul>
      </div>
      <div class="col sm12">
        <h3 class="mb-4">Outputs</h3>

        <ul class="page-main__ports">
          <li v-for="gpio of 16" :key="gpio">
            <button @click="onOutput(gpio)">
              <div :class="['page-main__port', { 'page-main__port--active': getGPIOOut(gpio - 1) }]"></div>
              <span class="text-caption fw-700 d-flex j-center">OUT {{ gpio }}</span>
            </button>
          </li>
        </ul>
      </div>
      {{ device }}
      <!-- <div class="col sm12 page-main__list">
        <div class="d-flex gap-4 mb-2">
          <div class="page-main__time grey-base">Time</div>
          <div class="grey-base">Message</div>
        </div>
        <div v-for="item of logs" :key="item.now" class="page-main__item d-flex gap-4 mb-2">
          <div class="page-main__time">
            {{ new Date(item.now * 1000).toLocaleString() }}
          </div>
          <div class="page-main__log grey">
            {{ item.buffer }}
          </div>
        </div>
      </div> -->
      <div class="col sm12 page-main__send d-flex">
        <v-input v-model="text" placeholder="Message" @enter="onSend" />
        <v-button class="ml-4" :disabled="!text" @click="onSend">Send</v-button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { nextTick, onMounted, ref, watch } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import event from '@/assets/js/event';

const webSocketStore = useWebSocketStore();
const { device } = storeToRefs(webSocketStore);

const text = ref('start');
const logs = ref([]);
const datetime = ref('');

const now = ref(0);

event.on('init', () => {
  console.log('init websocket');
});

const getGPIOIn = i => {
  const value = (device.value.portIn1 << 8) + device.value.portIn2;
  return value & (1 << i); /// Math.pow(i, 2);
};

const getGPIOOut = i => {
  const value = (device.value.portOut2 << 8) + device.value.portOut1;
  return value & (1 << i); /// Math.pow(i, 2);
};

const getBit = (n, position) => (n >> position) & 1;
const setBit = (n, position) => n | (1 << position);
function clearBit(n, position) {
  const mask = ~(1 << position);
  return n & mask;
}

const onOutput = gpio => {
  device.value.portOut1 = setBit(device.value.portOut1, gpio - 1);
  console.log(device.value.portOut1);
  onSend();
};

const onSend = () => {
  webSocketStore.onSend('DEVICE', { now: now.value, direction: 0, ...device.value });
  text.value = '';
};
const onDate = e => {
  const _now = e?.target?.valueAsNumber;
  if (_now) now.value = _now / 1000;
  webSocketStore.onSend('DEVICE', { now: now.value, direction: 0, buffer: `Change date` });
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

onMounted(() => {
  onSend();
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

  &__ports {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }
  &__port {
    position: relative;
    width: 40px;
    height: 40px;

    border: 1px solid grey;
    border-radius: 4px;
    &::before {
      content: '';
      position: absolute;
      left: 50%;
      top: 50%;
      transform: translate(-50%, -50%);
      width: 10px;
      height: 10px;
      border: 1px solid grey;
      border-radius: 50%;
    }
    &--active::before {
      border: 1px solid green;
      background-color: green;
    }
  }
}
</style>
