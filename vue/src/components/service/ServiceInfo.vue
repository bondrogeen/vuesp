<template>
  <div class="grid grid-cols-1 gap-4 lg:max-w-[360px] min-w-[250px]">
    <div class="flex flex-col w-full items-center gap-2">
      <div class="relative block h-4 w-full rounded-lg overflow-hidden bg-gray-200 dark:bg-gray-800">
        <div class="absolute left-0 top-0 flex h-full items-center justify-center bg-blue-600 hover:bg-blue-500" :style="`width: ${percent}%;`" :title="`${percent}%`"></div>

        <span class="absolute left-1/2 -translate-x-1/2 top-0 text-xs font-medium text-gray-700 dark:text-gray-400">{{ percent }}%</span>
      </div>

      <div class="flex justify-between w-full text-xs">
        <div v-for="(item, key) in store" :key="key" class="flex-auto">
          <div class="text-gray-500 dark:text-gray-400">{{ key }}:</div>

          <div>{{ item }}</div>
        </div>
      </div>
    </div>

    <div class="text-sm">
      <div v-for="(item, key) in date" :key="key" class="flex justify-between items-center">
        <div class="text-gray-500 dark:text-gray-400 text-left">{{ key }}:</div>

        <div class="text-right text-xs">{{ item }}</div>
      </div>
    </div>

    <div>
      <slot></slot>

      <div class="flex gap-2 mt-4">
        <v-button :href="pkg?.repository.url" target="_blank" class="w-full text-white" color="blue">Github</v-button>

        <v-button v-if="pkg?.homepage" :href="pkg?.homepage" target="_blank" class="w-full" color="blue" outline>Homepage</v-button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, defineProps, onMounted, onUnmounted } from 'vue';
import { toByte, secToTime } from '@/utils/helpers.ts';

import type { IStoreInfo } from 'vuesp-components/types';

const { id = 0, firmware = [], totalBytes = 0, usedBytes = 0, uptime = 0, pkg } = defineProps<IStoreInfo>();

const time = ref(uptime);
let idTime = null as ReturnType<typeof setTimeout> | null;

const getFirmware = computed(() => firmware.join('.'));

const store = computed(() => ({
  Used: toByte(usedBytes),
  Available: toByte(totalBytes - usedBytes),
  Total: toByte(totalBytes),
}));

const date = computed(() => ({
  Uptime: secToTime(time.value),
  ID: id.toString(16),
  'Firmware (hw)': getFirmware.value,
  'Version (web)': pkg?.version,
}));

const percent = computed(() => (usedBytes ? Math.round((usedBytes * 100) / totalBytes) : 0));

const timeUp = () => {
  time.value++;
  idTime = setTimeout(timeUp, 1000);
};

onMounted(() => {
  timeUp();
});

onUnmounted(() => {
  time.value = 0;
  if (idTime) clearTimeout(idTime);
});
</script>
