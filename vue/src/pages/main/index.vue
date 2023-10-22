<template>
  <div class="page-main">
    <div class="row">
      <div class="pagemain__menu col sm12">
        <div class="page-main__wrapper">
          {{ sensors }}
          <div class="tools">
            <div v-for="tool of tools" :key="tool.name" class="tools__item" :class="{ 'tools__item--active': tool.active }" :title="tool.name" @click="onTools(tool)">
              <v-icons :icon="tool.name"></v-icons>
            </div>
          </div>
          <canvas class="page-main__canvas"></canvas>
        </div>

        <div class="page-main__palette">
          <div v-for="color of colors" :key="color" class="page-main__color" :style="getColor(color)" @click="onSetColor(color)"></div>
        </div>
      </div>
    </div>
    <AppDialog class="page-main__dialog" title="Create new" :value="showDialog" @close="onClose">
      <template #footer>
        <v-button @click="onDone">OK</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup>
import AppDialog from '@/components/app/AppDialog';
import { onMounted, ref, defineProps } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';
import Canvas from '@/assets/js/canvas';

// import event from '@/assets/js/event';

const props = defineProps({
  changeTheme: { type: Function, default: () => {} },
});

props.changeTheme(true);

const webSocketStore = useWebSocketStore();
const { sensors } = storeToRefs(webSocketStore);

// const text = ref('start');
// const logs = ref([]);

// const now = ref(0);

// event.on('init', () => {
//   console.log('init websocket');
// });

const onArray = () => {
  const buffer = canvas.array();
  webSocketStore.onSend('DEVICE', { buffer, effect: 0, command: 3 });
};

const onClick = () => {
  onArray();
};

const showDialog = ref(true);

const onTools = tool => {
  if (tool.event) {
    if (typeof tool.event === 'function') tool.event();
    if (canvas[tool.event]) canvas[tool.event]();
    onArray();
  } else {
    tools.value = tools.value.map(item => ({ ...item, active: Boolean(tool.name === item.name) }));
    canvas.setTool(tool.name);
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
  { active: false, name: 'palette' },
]);

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

const onSetColor = color => {
  if (canvas?.setcolor) canvas.setcolor(color);
};

const onClose = () => {
  showDialog.value = false;
};

const onDone = () => {
  canvas = new Canvas({ width: 16, height: 16, event: { click: onClick } });
  canvas.setcolor([0, 0, 0, 255]);
  onClose();
};

onMounted(() => {
  newProject();
  dragElement(document.querySelector('.page-main__palette'));
});

function dragElement(elmnt) {
  var pos1 = 0,
    pos2 = 0,
    pos3 = 0,
    pos4 = 0;
  elmnt.onmousedown = dragMouseDown;

  function dragMouseDown(e) {
    e = e || window.event;
    pos3 = e.clientX;
    pos4 = e.clientY;
    document.onmouseup = closeDragElement;
    document.onmousemove = elementDrag;
  }

  function elementDrag(e) {
    e = e || window.event;
    pos1 = pos3 - e.clientX;
    pos2 = pos4 - e.clientY;
    pos3 = e.clientX;
    pos4 = e.clientY;
    elmnt.style.top = elmnt.offsetTop - pos2 + 'px';
    elmnt.style.left = elmnt.offsetLeft - pos1 + 'px';
  }

  function closeDragElement() {
    document.onmouseup = null;
    document.onmousemove = null;
  }
}
</script>

<style lang="scss">
.page-main {
  position: relative;
  &__palette {
    width: 60px;
    position: absolute;
    top: 0;
    right: 0;
    display: grid;
    grid-template-columns: 1fr 1fr;
  }
  &__color {
    height: 30px;
    width: 30px;
  }

  &__input {
    max-width: 100px;
  }

  &__close {
    width: 14px;
  }

  &__wrapper {
    margin: 0 auto;
    width: 100%;
    max-width: 500px;
  }
  &__canvas {
    width: 100%;
    cursor: crosshair;
    touch-action: none;
    image-rendering: pixelated;
    image-rendering: crisp-edges;
    border: 1px solid color('grey', 'darken-1');
  }
}

.tools {
  display: flex;
  flex-wrap: wrap;
  border: 1px solid color('grey', 'darken-1');
  border-bottom: none;
  border-radius: 4px 4px 0 0;
  padding: 4px;
  gap: 2px;
  background-color: var(--bg-1);
  &__item {
    height: 28px;
    width: 28px;
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
