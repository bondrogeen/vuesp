<template>
  <div class="card-frame">
    <canvas class="card-frame__canvas" :class="`canvas--${id}`"></canvas>
  </div>
</template>

<script setup>
import { defineProps, onMounted } from 'vue';
import Canvas from '@/assets/js/canvas.ts';

const props = defineProps({
  id: { type: Number, default: 0 },
  buffer: { type: Array, default: () => [] },
});

onMounted(() => {
  const canvas = new Canvas({ width: 16, height: 16, selector: `.canvas--${props.id}`, view: true });
  canvas.setBuffer(props.buffer);
});
</script>
<!-- 
<style lang="scss">
.card-frame {
  position: relative;

  &__canvas {
    height: 40px;
    width: 40px;
  }
}
</style> -->
