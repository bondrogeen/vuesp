<template>
  <CardBase v-bind="props" @click="onClick">
    <template #header="{ value, max }">
      <div class="flex justify-end">
        <div class="" :class="'text-blue-600 dark:text-blue-400'">
          <svg width="32" height="32" viewBox="-7.5 -7.5 75 75" version="1.1" style="transform: rotate(-90deg)">
            <circle class="stroke-gray-300 dark:stroke-gray-700" r="30" cx="30" cy="30" fill="transparent" stroke-width="10"></circle>
            <circle
              r="30"
              cx="30"
              cy="30"
              stroke="currentColor"
              stroke-width="8"
              stroke-linecap="butt"
              :stroke-dashoffset="getRangePercent(value, max)"
              fill="transparent"
              stroke-dasharray="187"
            ></circle>
          </svg>
        </div>
      </div>
    </template>

    <template #dialog="{ min, max, value }">
      <div class="flex justify-center relative mx-2">
        <div class="absolute h-10 w-full top-0 left-0 bg-gray-400 dark:bg-gray-800 z-0 rounded-md"></div>

        <div class="absolute h-10 top-0 left-0 bg-blue-600 dark:bg-blue-400 z-0 rounded-md" :style="`width: ${getPercent(value, max)}%`"></div>

        <div class="absolute top-1/2 left-1/2 -translate-1/2 text-white z-10 font-bold text-xl">{{ getPercent(value, max) }} %</div>

        <input type="range" :min="min" :max="max" :value="value" step="1" class="w-full h-10 z-10 appearance-none cursor-pointer opacity-0" @change="onChange" />
      </div>
    </template>
  </CardBase>
</template>

<script setup lang="ts">
import type { TypeProperty } from '@/vuesp-data/types.ts';

import CardBase from '@/components/dashboard/cards/CardBase.vue';

const emit = defineEmits<{
  (e: 'click', event: Event): void;
  (e: 'setState', item: any): void;
}>();

const props = defineProps<TypeProperty>();

const getPercent = (value: number, max: number) => Math.round((100 * value) / max);
const getRangePercent = (value: number, max: number) => Math.round(187 - (value / max) * 187);

const onClick = (event: Event) => emit('click', event);

const onChange = (e: any) => {
  const value = e?.target?.value || 0;
  emit('setState', { ...props, value });
};
</script>
