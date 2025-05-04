<template>
  <div class="grid grid-cols-2 lg:grid-cols-1 gap-8 lg:max-w-[360px]">
    <div class="flex flex-col w-full items-center gap-3">
      <div class="relative block h-2 w-full rounded-sm bg-gray-200 dark:bg-gray-800">
        <div class="absolute left-0 top-0 flex h-full items-center justify-center rounded-sm bg-blue-600 text-xs font-medium text-white" :style="`width: ${percent}%;`"></div>
      </div>
      <p class="text-theme-sm font-medium text-gray-700 dark:text-gray-400">{{ percent }}%</p>
    </div>

    <div class="text-sm flex flex-col justify-center gap-2">
      <div v-for="(item, key) in date" :key="key" class="grid grid-cols-2">
        <div class="text-gray-400">{{ key }}:</div>
        <div class="font-medium">{{ item }}</div>
      </div>
    </div>

    <div>
      <slot></slot>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, defineProps, onMounted } from 'vue';
import { toByte, secToTime } from '@/utils/func/index.ts';

import type { TypeStateInfo } from '@/types/types.ts';

const { id = 0, firmware = [], totalBytes = 0, usedBytes = 0, uptime = 0 } = defineProps<TypeStateInfo>();

const time = ref(uptime);

const getFirmware = computed(() => firmware.join('.'));

const date = computed(() => ({
  Used: toByte(usedBytes),
  Available: toByte(totalBytes - usedBytes),
  Total: toByte(totalBytes),
  Uptime: secToTime(time.value),
  ID: id.toString(16),
  Firmware: getFirmware.value,
}));

const percent = computed(() => Math.round((usedBytes * 100) / totalBytes));

const timeUp = () => {
  time.value++;
  setTimeout(timeUp, 1000);
};

onMounted(() => {
  timeUp();
});
</script>
