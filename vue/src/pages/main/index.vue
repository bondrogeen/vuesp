<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12 text-h2 mb-6">Main</div>
      <div class="col sm12 text-h2 mb-6">
        <input v-model="datetime" class="page-main__date" type="datetime-local" @change="onDate" />
      </div>

      <div class="col sm12 page-main__list">
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
      </div>
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

const onSend = () => {
  webSocketStore.onSend('DEVICE', { now: now.value, direction: 0, buffer: `${text.value}\n` });
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
}
</style>
