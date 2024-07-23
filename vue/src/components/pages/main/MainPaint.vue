<template>
  <div class="main-paint">
    <div class="row">
      <div class="main-paint__menu col sm12 lg10 xl8">
        <div class="main-paint__wrapper">
          <div class="main-paint__header">
            <div class="main-paint__tools">
              <div v-for="tool of tools" :key="tool.name" class="main-paint__tool" :title="tool.name" @click="onTools(tool)">
                <v-icons :icon="tool.name"></v-icons>
              </div>

              <v-dropdown>
                <template #activator="{ on }">
                  <v-icons icon="addFrame" @click="on.click"></v-icons>
                </template>
                <v-list :list="listFrame" @click="onFrame"></v-list>
              </v-dropdown>
            </div>
            <ColorPicker v-if="colorPicker" class="mr-2" @color="onSetColor" @fill="onSetFill" />
          </div>
          <canvas class="main-paint__canvas"></canvas>
        </div>
      </div>
      <div class="main-paint__frames col sm12 lg2 xl4">
        <CardFrame v-for="frame of frames" :key="frame.id" v-bind="frame" @click="onCardFrame(frame)" />
      </div>
      <AppDialog class="main-paint__dialog" title="Create new" :value="showDialog" @close="onClose">
        <template #footer>
          <v-button @click="onDone">OK</v-button>
        </template>
      </AppDialog>
    </div>
  </div>
</template>

<script setup>
import AppDialog from '@/components/app/AppDialog';
import ColorPicker from '@/components/pages/main/ColorPicker';
import CardFrame from '@/components/pages/main/CardFrame';
import { storeToRefs } from 'pinia';

import { defineProps, onMounted, ref } from 'vue';
import Canvas from '@/assets/js/canvas';
import { useWebSocketStore } from '@/stores/WebSocketStore';

defineProps({
  name: { type: String, default: '' },
});

const colorPicker = ref(true);
const frames = ref([]);

const webSocketStore = useWebSocketStore();
const { device } = storeToRefs(webSocketStore);

let canvas = {};
const showDialog = ref(true);

const onClick = () => {
  const buffer = canvas.getBuffer();
  webSocketStore.onSend('DEVICE', { buffer, effect: 0, command: 3 });
};

const onSave = () => {
  const buffer = canvas.getBuffer();
  webSocketStore.onSend('DEVICE', { buffer, effect: 0, command: 4 });
};

const onFrame = ({ id }) => {
  if (id === 1) {
    const buffer = canvas.getBuffer();
    frames.value.push({ id: frames.value.length + 1, buffer });
  }
};

const onCardFrame = ({ buffer }) => {
  canvas.clear();
  canvas.setBuffer(buffer);
  webSocketStore.onSend('DEVICE', { buffer, effect: 0, command: 3 });
};

const listFrame = [
  { id: 1, name: 'New' },
  { id: 2, name: 'Remove' },
];

const onTools = tool => {
  if (tool.event) {
    if (typeof tool.event === 'function') tool.event();
    if (canvas[tool.event]) canvas[tool.event]();
    onClick();
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
  { active: false, name: 'image', event: 'saveImage' },
  { active: false, name: 'save', event: 'save' },
  // { active: false, name: 'palette', event: 'text' },
  { active: false, name: 'send', event: onClick },
  { active: false, name: 'clear', event: 'clear' },
  { active: false, name: 'addImage', event: 'addImage' },
  { active: false, name: 'undo', event: 'undo' },
  { active: false, name: 'redo', event: 'redo' },
  // { active: false, name: 'open', event: 'open' },
]);

const onSetColor = color => {
  if (canvas?.setColor) canvas.setColor(color);
};
const onSetFill = color => {
  if (canvas?.setFill) canvas.setFill(color);
};

const onClose = () => (showDialog.value = false);

const onDone = () => {
  canvas = new Canvas({ width: 16, height: 16, event: { click: onClick, save: onSave } });
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

  &__frames {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 16px 4px;
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

  &__tools {
    display: grid;
    grid-template-columns: repeat(6, 24px);
    gap: 4px;
  }
  &__tool {
    height: 24px;
    width: 24px;
    display: flex;
    justify-content: center;
    align-items: center;
    border-radius: 4px;
    &:hover {
      background-color: var(--bg-2);
    }
  }
}
</style>
