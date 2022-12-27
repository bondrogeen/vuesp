<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12 text-h2 mb-6">Main</div>
      <div class="col sm12" @click="onClick">
        <canvas width="150" height="150"></canvas>
      </div>
    </div>
  </div>
</template>

<script setup>
// import { useWebSocketStore } from '@/stores/WebSocketStore';

import { computed, onMounted } from 'vue';

// const webSocketStore = useWebSocketStore();
// const { fileList, info, progress } = storeToRefs(webSocketStore);

// const onSend = ({ comm, data }) => {
//   webSocketStore.onSend(comm, data);
// };
// const onClear = () => {
//   fileList.value = [];
// };

onMounted(() => {
  draw();
  const canvas = document.querySelector('canvas');
  canvas.addEventListener('mousemove', pick);
});

const ctx = computed(() => {
  const canvas = document.querySelector('canvas');
  return canvas.getContext('2d');
});

const onClick = () => {
  const imageData = ctx.value.getImageData(0, 0, 16, 16);
  const buffer = imageData.data.buffer;

  console.log(buffer);
};

const pick = event => {
  var x = event.layerX;
  var y = event.layerY;
  var pixel = ctx.value.getImageData(x, y, 1, 1);
  var data = pixel.data;
  var rgba = 'rgba(' + data[0] + ', ' + data[1] + ', ' + data[2] + ', ' + data[3] / 255 + ')';
  console.log(rgba);
};

const draw = () => {
  ctx.value.scale(10, 10);
  ctx.value.fillStyle = 'rgb(200,0,0)';
  ctx.value.fillRect(0, 0, 16, 16);
};
</script>

<style lang="scss">
.page-main {
  position: relative;
}
</style>
