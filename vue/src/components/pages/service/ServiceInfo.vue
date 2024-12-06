<template>
  <div class="p-4 border border-primary rounded-lg max-w-[320px]">
    <div class="my-6 mb-10">
      <VProgressbar :value="percent"></VProgressbar>
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

import VProgressbar from '@/components/general/VProgressbar';

const props = defineProps({
  totalBytes: { type: Number, default: 0 },
  usedBytes: { type: Number, default: 0 },
});

const date = computed(() => ({ Used: props.usedBytes, Available: props.totalBytes - props.usedBytes, Total: props.totalBytes }));
const percent = computed(() => (props.usedBytes * 100) / props.totalBytes);
</script>
