<template>
  <div class="container">
    <div class="row">
      <h1 class="col sm12 text-h2 mb-6">Main</h1>
      <div class="col sm12 lg8 xl7">
        <MainPaint :draw="main.draw" class="col sm12" @send="onSend" />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import MainPaint from '@/components/pages/main/MainPaint.vue';

// import event from '@/assets/js/event';

const webSocketStore = useWebSocketStore();
const { main } = storeToRefs(webSocketStore);

onMounted(() => {
  webSocketStore.onSend('DRAW');
});
const onSend = (data: any) => {
  webSocketStore.onSend('DRAW', data);
};
</script>
