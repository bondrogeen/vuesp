<script setup lang="ts">
import type { IScriptStudioMetricsProps } from './types';

const { metrics } = defineProps<IScriptStudioMetricsProps>();
</script>

<template>
  <div class="border-b border-gray-light-10/70 px-3 py-3 dark:border-white/10">
    <div class="flex flex-wrap gap-3">
      <div v-for="metric in metrics" :key="metric.key" class="flex items-center text-xs">
        <div class="text-gray-dark-25 dark:text-gray-main">{{ metric.label }}</div>
        <div class="truncate font-mono font-semibold" :class="{ 'text-red-main': metric.isError }" :title="metric.title">:{{ metric.value }}</div>
      </div>
    </div>

    <div class="mt-3">
      <slot></slot>
    </div>
  </div>
</template>
