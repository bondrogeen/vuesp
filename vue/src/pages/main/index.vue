<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12 text-h2 mb-6">Main</div>
      <div>{{ relay }}</div>
    </div>
  </div>
</template>

<script setup>
import { useWebSocketStore } from '@/stores/WebSocketStore';
import { storeToRefs } from 'pinia';
import { computed } from 'vue';

const webSocketStore = useWebSocketStore();
const { data } = storeToRefs(webSocketStore);

const relay = computed(() => data.value?.RELAY);

// const onSend = ({ comm, data }) => {
//   webSocketStore.onSend(comm, data);
// };
// const onClear = () => {
//   fileList.value = [];
// };
</script>

<style lang="scss">
.page-main {
  position: relative;
}
</style>
