<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12 lg5">
        <FileManager :files="fileList" :progress="progress" @send="onSend" @clear="onClear" />
      </div>
      <div class="col sm12 offset-lg2 lg5 mb-6">
        <FileInfo v-bind="info" class="page-main__file-info" />
      </div>
    </div>
  </div>
</template>

<script setup>
import { storeToRefs } from 'pinia';
import FileManager from '@/components/pages/main/FileManager';
import FileInfo from '@/components/pages/main/FileInfo';

import { useWebSocketStore } from '@/stores/WebSocketStore';

const webSocketStore = useWebSocketStore();
const { fileList, info, progress } = storeToRefs(webSocketStore);

const onSend = ({ comm, data }) => {
  webSocketStore.onSend(comm, data);
};
const onClear = () => {
  fileList.value = [];
};
</script>

<style lang="scss">
.page-main {
  position: relative;
  &__file-info {
    // max-width: 300px;
    margin: 0 auto;
  }
}
</style>
