<template>
  <div class="page-main">
    <div class="row">
      <div class="col sm1">
        <div class="tools">
          <div v-for="tool of tools" :key="tool.name" class="tools__item" :class="{ 'tools__item--active': tool.active }" :title="tool.name" @click="onTools(tool)">
            <v-icons :icon="tool.name"></v-icons>
          </div>
        </div>
      </div>
      <div class="page-home__menu col sm12 lg8">
        <canvas class="page-main__canvas"></canvas>

        <div class="page-main__palette">
          <div v-for="color of colors" :key="color" class="page-main__color" :style="getColor(color)" @click="onSetColor(color)"></div>
        </div>
      </div>
    </div>
    <AppDialog class="page-main__dialog" title="Select grid" :value="showDialog" @close="onClose">
      <div class="d-flex j-space-between">
        <v-input v-model="height" class="page-main__input" label="height" type="text" hide-message />
        <v-icons class="page-main__close mx-2" icon="close" @click="onClose"></v-icons>
        <v-input v-model="width" class="page-main__input" label="width" type="text" hide-message />
      </div>

      <template #footer>
        <v-button @click="onDone">OK</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup>
import AppDialog from '@/components/app/AppDialog';
import { onMounted, ref, defineProps } from 'vue';
// import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

// import event from '@/assets/js/event';

const props = defineProps({
  changeTheme: { type: Function, default: () => {} },
});

props.changeTheme(true);

const webSocketStore = useWebSocketStore();
// const { device } = storeToRefs(webSocketStore);

// const text = ref('start');
// const logs = ref([]);

// const now = ref(0);

// event.on('init', () => {
//   console.log('init websocket');
// });

const onArray = () => {
  const buffer = canvas.array();
  console.log(buffer);
  webSocketStore.onSend('DEVICE', { buffer, direction: 0, example1: 0 });
};

const showDialog = ref(true);
const height = ref(16);
const width = ref(16);

const onTools = tool => {
  if (tool.event) {
    if (typeof tool.event === 'function') tool.event();
    if (canvas[tool.event]) canvas[tool.event]();
  } else {
    tools.value = tools.value.map(item => ({ ...item, active: Boolean(tool.name === item.name) }));
  }
};
let canvas = {};

const newProject = () => {
  showDialog.value = true;
  localStorage.removeItem('pc-canvas-data');
};

const tools = ref([
  { active: false, name: 'new', event: newProject },
  { active: false, name: 'save', event: 'save' },
  { active: false, name: 'array', event: onArray },
  { active: true, name: 'pen' },
  { active: false, name: 'eraser' },
  { active: false, name: 'addFrame', event: 'addFrame' },
  { active: false, name: 'undo', event: 'undo' },
  { active: false, name: 'redo', event: 'redo' },
  { active: false, name: 'clear', event: 'clear' },
  { active: false, name: 'addImage', event: 'addImage' },
  { active: false, name: 'open', event: 'open' },
]);

const isToolActive = name => {
  const tool = tools.value.find(i => i.name === name);
  return Boolean(tool && tool?.active);
};

const colors = [
  [0, 0, 0, 255],
  [127, 127, 127, 255],
  [255, 0, 0, 255],
  [0, 255, 0, 255],
  [0, 0, 255, 255],
  [255, 242, 0, 255],
  [34, 177, 36, 255],
  [0, 162, 232, 255],
  [63, 72, 204, 255],
  [163, 73, 164, 255],
  [163, 73, 164, 255],
  [255, 255, 255, 255],
];

const getColor = ([red, green, blue]) => {
  return `background-color: rgb(${red},${green},${blue})`;
};

class Canvas {
  constructor(width, height) {
    this.canvas = document.querySelector('canvas');
    this.canvas.width = 10 * width;
    this.canvas.height = 10 * height;
    this.width = width;
    this.height = height;
    this.canvas.style.display = 'block';
    this.canvas.style.height = Math.floor((height / width) * this.canvas.clientWidth) + 'px';
    this.w = +this.canvas.width;
    this.h = +this.canvas.height;
    this.ctx = this.canvas.getContext('2d');
    this.ctx.fillStyle = 'white';
    this.ctx.globalAlpha = 1;
    this.ctx.fillRect(0, 0, this.w, this.h);
    this.data = [...Array(this.width)].map(() => Array(this.height).fill([255, 255, 255, 255]));
    this.steps = [];
    this.redo_arr = [];
    this.frames = [];

    this.canvas.addEventListener('click', e => {
      console.log(e);
      var rect = this.canvas.getBoundingClientRect();
      var x = e.clientX - rect.left;
      var y = e.clientY - rect.top;
      x = Math.floor((this.width * x) / this.canvas.clientWidth);
      y = Math.floor((this.height * y) / this.canvas.clientHeight);
      if (isToolActive('pen')) {
        this.draw(x, y);
      } else if (isToolActive('eraser')) {
        this.erase(x, y);
      }
    });

    this.canvas.addEventListener('mousemove', e => {
      if (this.active) {
        var rect = this.canvas.getBoundingClientRect();
        var x = e.clientX - rect.left;
        var y = e.clientY - rect.top;
        x = Math.floor((this.width * x) / this.canvas.clientWidth);
        y = Math.floor((this.height * y) / this.canvas.clientHeight);
        if (isToolActive('pen')) {
          this.draw(x, y);
        } else if (isToolActive('eraser')) {
          this.erase(x, y);
        }
      }
    });

    this.canvas.addEventListener('touchmove', e => {
      var rect = this.canvas.getBoundingClientRect();
      var x = e.touches[0].clientX - rect.left;
      var y = e.touches[0].clientY - rect.top;
      x = Math.floor((this.width * x) / this.canvas.clientWidth);
      y = Math.floor((this.height * y) / this.canvas.clientHeight);
      if (isToolActive('pen')) {
        this.draw(x, y);
      } else if (isToolActive('eraser')) {
        this.erase(x, y);
      }
    });

    this.canvas.addEventListener('mouseleave', () => {
      this.active = false;
    });

    this.canvas.addEventListener('mousedown', () => {
      this.active = true;
    });

    this.canvas.addEventListener('mouseup', () => {
      this.active = false;
    });
  }

  draw(x, y, count) {
    console.log(x, y, count);
    if (x >= 0 && x < this.width && y >= 0 && y < this.height) {
      this.data[x][y] = this.color;
      this.ctx.fillRect(Math.floor(x * (this.w / this.width)), Math.floor(y * (this.h / this.height)), Math.floor(this.w / this.width), Math.floor(this.h / this.height));
      if (!count && JSON.stringify(this.steps[this.steps.length - 1]) !== JSON.stringify([x, y, this.color, this.ctx.globalAlpha])) this.steps.push([x, y, this.color, this.ctx.globalAlpha]);
    }
  }

  array() {
    console.log(this.data);
    let arr = [];
    for (let h = 0; h < this.height; h++) {
      for (let w = 0; w < this.width; w++) {
        arr = h % 2 ? [...arr, ...this.data[15 - w][h]] : [...arr, ...this.data[w][h]];
      }
    }
    let uint8bytes = Uint8Array.from(arr);
    let dataview = new DataView(uint8bytes.buffer);
    const buffer = [];
    for (let i = 0; i < dataview.byteLength; i += 4) {
      buffer.push(dataview.getInt32(i));
    }
    // console.log(buffer);
    return buffer;
  }

  erase(x, y) {
    var temp = this.color;
    var tga = this.ctx.globalAlpha;
    this.setcolor([255, 255, 255, 255]);
    this.draw(x, y);
    this.setcolor(temp);
    this.ctx.globalAlpha = tga;
  }

  setcolor(color) {
    this.ctx.globalAlpha = 1;
    this.color = color;
    this.ctx.fillStyle = 'rgba(' + color[0] + ',' + color[1] + ',' + color[2] + ',' + color[3] + ')';
  }

  save() {
    this.canvas.toBlob(function (blob) {
      var url = URL.createObjectURL(blob);
      var link = document.createElement('a');
      link.download = 'canvas.png';
      link.href = url;
      link.click();
    });
  }

  clear() {
    this.ctx.fillStyle = 'white';
    this.ctx.fillRect(0, 0, this.w, this.h);
    this.data = [...Array(this.width)].map(() => Array(this.height).fill([255, 255, 255, 255]));
    this.setcolor(this.color);
  }

  addFrame(data = null) {
    var img = new Image();
    img.src = data || this.canvas.toDataURL();
    this.frames.push([img, this.data.map(inner => inner.slice())]);
  }

  deleteFrame(f) {
    this.frames.splice(f, 1);
  }

  loadFrame(f) {
    this.clear();
    var img = this.frames[f][1];
    var tmp_color = this.color;
    var tmp_alpha = this.ctx.globalAlpha;
    this.ctx.globalAlpha = 1;
    var i, j;
    for (i = 0; i < this.width; i++) {
      for (j = 0; j < this.height; j++) {
        this.setcolor(img[i][j]);
        this.draw(i, j);
      }
    }
    this.setcolor(tmp_color);
    this.ctx.globalAlpha = tmp_alpha;
  }

  // renderGIF() {
  //   this.frames.forEach(frame => {
  //     gif.addFrame(frame[0], { copy: true, delay: 100 });
  //   });
  //   gif.render();
  // }

  undo() {
    this.clear();
    this.redo_arr.push(this.steps.pop());
    this.steps.forEach(step => {
      this.setcolor(step[2]);
      this.ctx.globalAlpha = step[3];
      this.draw(step[0], step[1], true);
    });
  }

  redo() {
    this.steps.push(this.redo_arr.pop());
    this.steps.forEach(step => {
      this.setcolor(step[2]);
      this.ctx.globalAlpha = step[3];
      this.draw(step[0], step[1], true);
    });
  }

  saveInLocal() {
    let d = {
      colors: colors,
      currColor: this.color,
      width: this.width,
      height: this.height,
      url: this.canvas.toDataURL(),
      steps: this.steps,
      redo_arr: this.redo_arr,
    };
    localStorage.setItem('pc-canvas-data', JSON.stringify(d));
  }

  addImage() {
    var _this = this;
    var fp = document.createElement('input');
    fp.type = 'file';
    fp.click();
    fp.onchange = function (e) {
      var reader = new FileReader();
      reader.readAsDataURL(e.target.files[0]);
      reader.onload = function () {
        var uimg = new Image();
        uimg.src = reader.result;
        uimg.width = _this.w;
        uimg.height = _this.h;
        uimg.onload = function () {
          var pxc = document.createElement('canvas');
          pxc.width = _this.w;
          pxc.height = _this.h;
          var pxctx = pxc.getContext('2d');
          pxctx.drawImage(uimg, 0, 0, _this.w, _this.h);
          var i, j;
          for (i = 0; i < _this.width; i++) {
            for (j = 0; j < _this.height; j++) {
              var ctr = 0;
              var avg = [0, 0, 0, 0];
              var pix = pxctx.getImageData(10 * i, 10 * j, 10, 10).data;
              pix.forEach((x, k) => {
                avg[k % 4] += x;
                if (k % 4 == 0) ctr++;
              });
              avg = avg.map(x => ~~(x / ctr));
              _this.setcolor(avg);
              _this.draw(i, j);
            }
          }
        };
      };
    };
  }
}

const onSetColor = color => {
  canvas.setcolor(color);
};

// class Frames {
//   static open() {
//     document.querySelector('#frames').style.display = 'block';
//     document.querySelector('#frames').style.transform = 'translate(-50%,-50%) scale(1,1)';
//     document.querySelector('#frames').focus();
//     document.querySelector('#frames #gallery').innerHTML = '';
//     for (var frame of board.frames) document.querySelector('#frames #gallery').appendChild(frame[0]);
//     document.querySelectorAll('#frames #gallery img').forEach((x, i) => {
//       x.onclick = () => {
//         board.loadFrame(i);
//         Frames.close();
//       };
//       x.oncontextmenu = e => {
//         e.preventDefault();
//         var del_confirmation = confirm('Delete?');
//         if (del_confirmation) {
//           board.deleteFrame(i);
//           Frames.open();
//         }
//       };
//     });
//   }
//   static close() {
//     document.querySelector('#frames').style.transform = 'translate(-50%,-50%) scale(0,0)';
//   }
// }

// function filler(x, y, cc) {
//   if (x >= 0 && x < window.board.width && y >= 0 && y < window.board.height) {
//     if (JSON.stringify(board.data[x][y]) == JSON.stringify(cc) && JSON.stringify(window.board.data[x][y]) != JSON.stringify(window.board.color)) {
//       window.board.draw(x, y);
//       filler(x + 1, y, cc);
//       filler(x, y + 1, cc);
//       filler(x - 1, y, cc);
//       filler(x, y - 1, cc);
//     }
//   }
// }

const onClose = () => {
  showDialog.value = false;
};

const onDone = () => {
  canvas = new Canvas(width.value, height.value);
  canvas.setcolor([0, 0, 0, 255]);
  onClose();
  // eslint-disable-next-line no-undef
  // gif = new GIF({
  //   workers: 2,
  //   quality: 10,
  //   width: 10 * canvas.width,
  //   height: 10 * canvas.height,
  // });
  // gif.on('finished', function (blob) {
  //   var url = URL.createObjectURL(blob);
  //   var link = document.createElement('a');
  //   link.download = 'canvas.gif';
  //   link.href = url;
  //   link.click();
  // });
};

onMounted(() => {
  newProject();
});
</script>

<style lang="scss">
.page-main {
  position: relative;
  &__palette {
    position: absolute;
    top: 0;
    right: 0;
    display: grid;
    grid-template-columns: 1fr 1fr;
  }
  &__color {
    height: 10px;
    width: 10px;
  }

  &__input {
    max-width: 100px;
  }

  &__close {
    width: 14px;
  }
  &__canvas {
    // box-shadow: 0px 0px 10px 0px rgba(0, 0, 0, 0.5);
    height: 100%;
    width: 100%;
    // max-width: 500px;
    cursor: crosshair;
    touch-action: none;
    image-rendering: pixelated;
    image-rendering: crisp-edges;
  }
  &__tool {
    height: 50px;
    width: 50px;
  }
}

.tools {
  display: grid;
  grid-template-columns: 1fr 1fr;
  &__item {
    height: 40px;
    width: 40px;
    display: flex;
    justify-content: center;
    align-items: center;
    border-radius: 4px;
    &:hover {
      background-color: var(--bg-2);
    }
  }

  &__item--active {
    background-color: var(--bg-2);
  }
}
</style>
