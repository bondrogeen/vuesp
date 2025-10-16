<template>
  <div class="grid grid-cols-1 gap-4 lg:max-w-[360px] min-w-[250px]">
    <div class="flex flex-col w-full items-center gap-2">
      <div class="relative block h-4 w-full rounded-lg overflow-hidden bg-gray-200 dark:bg-gray-800 select-none" :title="`${percent}%`">
        <div class="absolute left-[110px] top-1/2 -translate-1/2 text-sm font-bold text-black dark:text-white text-center leading-none">{{ percent }}%</div>
        
        <div class="bg-blue-600 rounded-full h-4 overflow-hidden relative transition-all duration-500" :style="`width: ${percent}%`">
          <div class="absolute left-[110px] top-1/2 -translate-1/2 text-sm font-bold text-white text-center leading-none">{{ percent }}%</div>
        </div>
      </div>

      <div class="flex justify-between w-full text-xs">
        <div v-for="item of store" :key="item.name" class="flex-auto">
          <div class="text-gray-500 dark:text-gray-400">{{ item.name }}:</div>

          <div>{{ item.value }}</div>
        </div>
      </div>
    </div>

    <div class="text-sm">
      <div v-for="item of date" :key="item.name" class="flex justify-between items-center">
        <div class="text-gray-500 dark:text-gray-400 text-left">{{ item.name }}:</div>

        <div class="text-right text-xs">{{ item.value }}</div>
      </div>
    </div>

    <div>
      <slot :home="pkg?.homepage" :repo="pkg?.repository?.url"></slot>
    </div>
  </div>
</template>

<script setup lang="ts">
import type { IStateInfo } from '@/types/';

import { ref, computed, defineProps, onMounted, onUnmounted } from 'vue';
import { toByte, secToTime } from 'vuesp-components/helpers';

import { useLocale } from '@/composables/useLocale';

const { $t } = useLocale();

const { id = 0, firmware = [], totalBytes = 0, usedBytes = 0, uptime = 0, pkg } = defineProps<IStateInfo>();

const time = ref(uptime);
let idTime = null as ReturnType<typeof setTimeout> | null;

const getFirmware = computed(() => firmware.join('.'));

const store = computed(() => [
  { name: $t('used'), value: toByte(usedBytes) },
  { name: $t('available'), value: toByte(totalBytes - usedBytes) },
  { name: $t('total'), value: toByte(totalBytes) },
]);

const date = computed(() => [
  { name: $t('uptime'), value: secToTime(time.value) },
  { name: $t('id'), value: id.toString(16) },
  { name: $t('firmware'), value: getFirmware.value },
  { name: $t('version'), value: pkg?.version },
]);

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
