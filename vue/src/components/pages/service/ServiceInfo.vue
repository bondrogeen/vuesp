<template>
  <div class="p-4 border border-primary rounded-lg max-w-[320px]">
    <div class="flex justify-center mb-4">
      <div class="relative size-40">
        <svg class="size-full -rotate-90" viewBox="0 0 36 36">
          <circle cx="18" cy="18" r="16" fill="none" class="stroke-current text-gray-200 dark:text-neutral-700" stroke-width="4"></circle>
          <circle
            cx="18"
            cy="18"
            r="16"
            fill="none"
            class="stroke-current text-primary dark:text-blue-500 hover:text-secondary transition"
            stroke-width="4"
            stroke-dasharray="100"
            :stroke-dashoffset="100 - percent"
            stroke-linecap="round"
          ></circle>
        </svg>

        <div class="absolute top-1/2 start-1/2 transform -translate-y-1/2 -translate-x-1/2">
          <span class="text-center text-2xl font-bold text-blue-600 dark:text-blue-500">{{ percent }}%</span>
        </div>
      </div>
    </div>

    <div class="flex justify-between">
      <div v-for="(item, key) in date" :key="key" class="text-center">
        <div class="text-gray-400">{{ key }}</div>
        <div class="font-bold">{{ toByte(item) }}</div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, defineProps } from 'vue';
import { toByte } from '@/utils/func/';

const props = defineProps({
  totalBytes: { type: Number, default: 0 },
  usedBytes: { type: Number, default: 0 },
});

const date = computed(() => ({ Used: props.usedBytes, Available: props.totalBytes - props.usedBytes, Total: props.totalBytes }));
const percent = computed(() => Math.round((props.usedBytes * 100) / props.totalBytes));
</script>
