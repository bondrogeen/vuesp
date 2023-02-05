<template>
  <div class="page-main container">
    <div class="row">
      <div class="col sm12 text-h2 mb-6">Main</div>
      <div class="col sm12" @click="onClick">
        <div class="canvas">
          <canvas width="16" height="16"></canvas>
        </div>
      </div>
      <div class="col sm12">
        <v-input-file @change="onLoad">load</v-input-file>
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
  // const canvas = document.querySelector('canvas');
  // canvas.addEventListener('mousemove', pick);
});

const onLoad = e => {
  console.log(e);
  const img = new Image();
  img.onload = function () {
    console.log(ctx.value);
    ctx.value.drawImage(img, 0, 0);
    const imageData = ctx.value.getImageData(0, 0, 16, 16);
    console.log(imageData);
    const buffer = imageData.data.buffer;
    console.log(buf2hex(buffer));
    // editPixels(imageData.data);

    // drawEditedImage(imageData);
    // const canvas = document.querySelector('canvas');
    // canvas.toBlob(function (blob) {
    //   console.log(blob);
    // });
  };
  img.src = URL.createObjectURL(e.files[0]);
};

const saveByteArray = (data, name) => {
  var a = document.createElement('a');
  document.body.appendChild(a);
  a.style = 'display: none';

  console.log(data, name);
  const blob = new Blob(data, { type: 'octet/stream' });
  console.log(blob);
  const url = window.URL.createObjectURL(blob);
  console.log(url);
  a.href = url;
  a.download = name;
  a.click();
  window.URL.revokeObjectURL(url);
};

const ctx = computed(() => {
  const canvas = document.querySelector('canvas');
  return canvas.getContext('2d');
});

// const editPixels = imgData => {
//   for (var i = 0; i < imgData.length; i += 4) {
//     imgData[i] = imgData[i] - 100;
//     imgData[i + 1] = imgData[i + 1] - 100;
//     imgData[i + 2] = imgData[i + 2] - 100;
//   }
// };

// const drawEditedImage = newData => {
//   ctx.value.putImageData(newData, 0, 0);
// };

const buf2hex = buffer => [...new Uint8Array(buffer)].map(x => x.toString(16).padStart(2, '0')).join(' ');

const onClick = () => {
  const imageData = ctx.value.getImageData(0, 0, 16, 16);
  const buffer = imageData.data.buffer;

  console.log(buffer);
  console.log(buf2hex(buffer));
  // saveByteArray([buffer], 'example.txt');
  onUpload([buffer]);
};

// const pick = event => {
//   var x = event.layerX;
//   var y = event.layerY;
//   console.log(x, y);
//   var pixel = ctx.value.getImageData(x, y, 1, 1);
//   var data = pixel.data;
//   var rgba = 'rgba(' + data[0] + ', ' + data[1] + ', ' + data[2] + ', ' + data[3] / 255 + ')';
//   console.log(rgba);
// };

const onUpload = async buffer => {
  const blob = new Blob(buffer, { type: 'octet/stream' });

  var data = new FormData();
  data.append(`file[0]`, blob, 'test.txt');
  let headers = new Headers();
  headers.set('Authorization', 'Basic ' + btoa('admin:admin'));
console.log(btoa('admin:admin'))
  const res = await (await fetch('/fs', { method: 'POST', headers, body: data })).json();
  console.log(res);
};

const draw = () => {
  // ctx.value.scale(10, 10);
  ctx.value.fillStyle = '#00FF00';
  ctx.value.fillRect(0, 0, 16, 16);
};
</script>

<style lang="scss">
.page-main {
  position: relative;
  .canvas {
    position: relative;
    display: flex;
    align-items: center;
    justify-content: center;
    height: 150px;
    scale: 10;
  }
}
</style>
