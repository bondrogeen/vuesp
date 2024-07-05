<template>
  <div class="main-paint">
    <div class="col sm12">
      <div class="row">
        <div class="main-paint__menu col sm12">
          <div class="main-paint__wrapper">
            <div class="main-paint__header">
              <div class="tools">
                <div v-for="tool of tools" :key="tool.name" class="tools__item" :class="{ 'tools__item--active': tool.active }" :title="tool.name" @click="onTools(tool)">
                  <v-icons :icon="tool.name"></v-icons>
                </div>
              </div>
              <ColorPicker v-if="colorPicker" class="mr-2" :colors="colors" @color="onSetColor" @fill="onSetFill" @close="onClosePicker" />
            </div>
            <canvas class="main-paint__canvas"></canvas>
          </div>
        </div>
        <AppDialog class="main-paint__dialog" title="Create new" :value="showDialog" @close="onClose">
          <template #footer>
            <v-button @click="onDone">OK</v-button>
          </template>
        </AppDialog>
      </div>
    </div>
  </div>
</template>

<script setup>
import AppDialog from '@/components/app/AppDialog';
import ColorPicker from '@/components/pages/main/ColorPicker';
import { storeToRefs } from 'pinia';

import { defineProps, onMounted, ref } from 'vue';
import Canvas from '@/assets/js/canvas';
import { useWebSocketStore } from '@/stores/WebSocketStore';

defineProps({
  name: { type: String, default: '' },
});

const colorPicker = ref(true);

// const emit = defineEmits(['click']);

const webSocketStore = useWebSocketStore();
const { device } = storeToRefs(webSocketStore);

// const title = computed(() => props.name || '');

const onClick = () => {
  onArray();
};

const onClosePicker = () => (colorPicker.value = false);

let canvas = {};

const onArray = () => {
  const buffer = canvas.getBuffer();
  webSocketStore.onSend('DEVICE', { buffer, effect: 0, command: 3 });
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

const newProject = () => {
  showDialog.value = true;
  localStorage.removeItem('pc-canvas-data');
};

const tools = ref([
  { active: false, name: 'new', event: newProject },
  { active: false, name: 'save', event: 'save' },
  { active: false, name: 'array', event: onArray },
  { active: false, name: 'addFrame', event: 'addFrame' },
  { active: false, name: 'undo', event: 'undo' },
  { active: false, name: 'redo', event: 'redo' },
  { active: false, name: 'clear', event: 'clear' },
  { active: false, name: 'addImage', event: 'addImage' },
  // { active: false, name: 'open', event: 'open' },
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

const onSetColor = color => {
  if (canvas?.setColor) canvas.setColor(color);
};
const onSetFill = color => {
  console.log(color);

  if (canvas?.setFill) canvas.setFill(color);
};

const onClose = () => {
  showDialog.value = false;
};

const onDone = () => {
  canvas = new Canvas({ width: 16, height: 16, event: { click: onClick } });
  onClose();
};

onMounted(() => {
  onDone();
  webSocketStore.onSend('DEVICE', { command: 0 });
  setTimeout(() => {
    canvas.setBuffer(device.value.buffer);
  }, 1000);
});
</script>

<style lang="scss">
.main-paint {
  position: relative;

  &__input {
    max-width: 100px;
  }

  &__close {
    width: 14px;
  }

  &__header {
    display: flex;
    flex-wrap: wrap;
    justify-content: space-between;
    border: 1px solid color('grey', 'darken-1');
    border-bottom: none;
    border-radius: 4px 4px 0 0;
    padding: 4px;
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
  display: grid;
  grid-template-columns: repeat(6, 30px);

  gap: 2px;
  background-color: var(--bg-1);
  &__item {
    height: 30px;
    width: 30px;
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
