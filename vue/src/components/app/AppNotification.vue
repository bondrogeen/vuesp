<template>
  <TransitionGroup name="list" tag="ul" class="flex flex-col gap-2">
    <div v-for="item of notifications" :key="item.id" class="w-full min-w-[260px] max-w-xs p-4 bg-white rounded-lg shadow-lg dark:bg-gray-800" role="alert">
      <div class="flex items-center gap-4 text-body">
        <div class="flex-auto">
          {{ item }}
        </div>
        <span v-if="isProgressDone(item)" class="text-green-600">Done</span>
        <button class="transition text-gray-400 self-start" @click="emit('close', item)">
          <IconClose class="h-4 w-4"></IconClose>
        </button>
      </div>
      <div v-if="isProgress(item)" class="w-full bg-gray-200 rounded-full dark:bg-gray-700 mt-2">
        <div class="bg-blue-600 text-small font-medium text-blue-100 text-center leading-none rounded-full" :style="`width: ${getPercent(item)}%`">{{ getPercent(item) }}%</div>
      </div>
    </div>
  </TransitionGroup>
</template>

<script setup>
import { defineProps, defineEmits, watch } from 'vue';

import IconClose from '@/components/icons/IconClose';

const props = defineProps({
  notifications: { type: Array, default: () => [] },
});

const emit = defineEmits(['close']);

const getPercent = progress => (progress.status ? Math.ceil((progress.size * 100) / progress.length) : 100);
const isProgress = progress => Boolean(progress.length);
const isProgressDone = progress => Boolean(isProgress(progress) && getPercent(progress) === 100 && !progress.status);

watch(
  () => props.notifications,
  (value, valueOld) => {
    console.log(value);
    console.log(valueOld);
  }
);
</script>
