<template>
  <div class="main-paint">
    <div class="row">
      <div class="main-paint__menu col sm12 lg10 xl8">
        <div class="main-paint__wrapper">
          <div class="main-paint__header">
            <div class="main-paint__tools">
              <div v-for="tool of tools" :key="tool.name" class="main-paint__tool" :title="tool.name" @click="onTools(tool)">
                {{ tool.name }}
              </div>

              <v-dropdown>
                <template #activator="{ on }">
                  <v-icons icon="addFrame" @click="on.click"></v-icons>
                </template>
                <!-- <v-list :list="listFrame" @click="onFrame"></v-list> -->
              </v-dropdown>
            </div>
            <!-- <ColorPicker v-if="colorPicker" class="mr-2" @color="onSetColor" @fill="onSetFill" /> -->
          </div>
          <canvas class="main-paint__canvas"></canvas>
        </div>
      </div>

      <!-- <div class="main-paint__frames col sm12 lg2 xl4">
        <CardFrame v-for="frame of frames" :key="frame.id" v-bind="frame" @click="onCardFrame(frame)" />
      </div> -->

      <AppDialog v-if="showDialog" class="main-paint__dialog" title="Create new" :value="showDialog" @close="onClose">
        <template #footer>
          <v-button @click="onDone">OK</v-button>
        </template>
      </AppDialog>
    </div>
  </div>
</template>

<script setup lang="ts">
// import ColorPicker from '@/components/pages/main/ColorPicker.vue';
// import CardFrame from '@/components/pages/main/CardFrame';

import { defineProps, onMounted, ref } from 'vue';
import Canvas from '@/assets/js/canvas.ts';

interface Props {
  draw?: any;
}

const { draw = {} } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'send', value: any): void;
}>();

// const colorPicker = ref(true);
// const frames = ref([]);

let canvas: any = {};
const showDialog = ref(true);

const onClick = () => {
  const buffer = canvas.getBuffer();
  emit('send', { buffer, effect: 0, command: 3 });
};

const onSave = () => {
  const buffer = canvas.getBuffer();
  emit('send', { buffer, effect: 0, command: 4 });
};

// const onFrame = ({ id }: any) => {
//   if (id === 1) {
//     const buffer = canvas.getBuffer();
//     frames.value.push({ id: frames.value.length + 1, buffer });
//   }
// };

// const onCardFrame = ({ buffer }: any) => {
//   canvas.clear();
//   canvas.setBuffer(buffer);
//   emit('send', { buffer, effect: 0, command: 3 });
// };

// const listFrame = [
//   { id: 1, name: 'New' },
//   { id: 2, name: 'Remove' },
// ];

const onTools = (tool: any) => {
  if (tool.event) {
    if (typeof tool.event === 'function') tool.event();
    if (canvas[tool.event]) canvas[tool.event]();
    onClick();
  } else {
    tools.value = tools.value.map((item) => ({ ...item, active: Boolean(tool.name === item.name) }));
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

// const onSetColor = (color: any) => {
//   if (canvas?.setColor) canvas.setColor(color);
// };
// const onSetFill = (color: any) => {
//   if (canvas?.setFill) canvas.setFill(color);
// };

const onClose = () => (showDialog.value = false);

const onDone = () => {
  canvas = new Canvas({ width: 16, height: 16, event: { click: onClick, save: onSave } });
  onClose();
};

onMounted(() => {
  onDone();
  setTimeout(() => {
    canvas.setBuffer(draw.buffer);
  }, 1000);
});
</script>
