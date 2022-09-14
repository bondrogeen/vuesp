<template>
  <div class="page-settings container">
    <div class="row">
      {{ settings }}
    </div>
  </div>
</template>

<script setup>
import { onMounted } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

const webSocketStore = useWebSocketStore();
const { settings } = storeToRefs(webSocketStore);

onMounted(() => {
  webSocketStore.onSend('SETTINGS');
});
</script>

<style lang="scss">
.page-settings {
  position: relative;
}
</style>
