<script setup lang="ts">
import { onMounted, ref } from 'vue';

import { VButton } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';

import Canvas from '@/assets/js/canvas.ts';

interface Props {
  draw?: { buffer?: number[] };
}

const { draw = {} } = defineProps<Props>();

const { $t } = useLocale();

const emit = defineEmits<{
  (e: 'send', value: Record<string, unknown>): void;
}>();

let canvas: Canvas;

const tools = ref([
  { name: 'pen', active: true },
  { name: 'eraser', active: false },
  { name: 'clear', active: false },
  { name: 'undo', active: false },
  { name: 'redo', active: false },
  { name: 'addImage', active: false },
  { name: 'save', active: false },
]);

const onClick = () => {
  emit('send', { buffer: canvas.getBuffer(), effect: 0, command: 3 });
};

const onTools = (tool: { name: string; active: boolean }) => {
  if (tool.name === 'clear' || tool.name === 'undo' || tool.name === 'redo' || tool.name === 'addImage') {
    canvas[tool.name as 'clear' | 'undo' | 'redo' | 'addImage']();
    canvasesFill(tool.name);
    return;
  }
  if (tool.name === 'save') {
    canvas.saveImage();
    return;
  }
  canvas.setTool(tool.name);
  tools.value = tools.value.map((item) => ({ ...item, active: item.name === tool.name }));
};

// clear/undo/redo/addImage change the buffer, so the device should receive it.
const canvasesFill = (name: string) => {
  if (name === 'addImage') {
    canvas.update();
    return;
  }
  onClick();
};

onMounted(() => {
  canvas = new Canvas({ width: 16, height: 16, selector: '.main-paint__canvas', event: { click: onClick, save: () => canvas.saveImage() } });
  canvas.setBuffer(draw.buffer);
});
</script>

<template>
  <card-main :title="$t('menu.draw')">
    <div class="mb-4 flex flex-wrap gap-2">
      <v-button v-for="tool in tools" :key="tool.name" size="sm" :color="tool.active ? 'blue' : 'gray'" :outline="!tool.active" @click="onTools(tool)">
        {{ tool.name }}
      </v-button>

      <v-button color="blue" size="sm" @click="onClick">send</v-button>
    </div>

    <div class="flex justify-center overflow-auto rounded-lg border border-gray-200/70 bg-black/80 p-2 dark:border-white/10">
      <canvas class="main-paint__canvas"></canvas>
    </div>
  </card-main>
</template>
