<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12">
        <FileManager :files="fileList" @send="onSend" />
      </div>
    </div>
  </div>
</template>

<script setup>
import FileManager from '@/components/pages/main/FileManager';
import { onMounted } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

const webSocketStore = useWebSocketStore();
const { fileList } = storeToRefs(webSocketStore);

const onSend = ({ comm, data }) => {
  webSocketStore.onSend(comm, data);
};

onMounted(() => {
  console.log('sdsdsdsds');
});
</script>

<style lang="scss">
.page-main {
  position: relative;
}
</style>
