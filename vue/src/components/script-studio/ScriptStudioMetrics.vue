<script setup lang="ts">
import { VButton, VIcon } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import type { IScriptStudioMetricsEmits, IScriptStudioMetricsProps } from './types';

const { $t } = useLocale();
const { metrics, isBusy, isDownloadAvailable, isCurrentSelected, instrPerTick, usPerTick } = defineProps<IScriptStudioMetricsProps>();
const emit = defineEmits<IScriptStudioMetricsEmits>();
</script>

<template>
  <div class="border-b border-gray-light-10/70 px-3 py-3 dark:border-white/10">
    <div class="flex flex-wrap gap-3">
      <div v-for="metric in metrics" :key="metric.key" class="flex items-center text-xs">
        <div class="text-gray-dark-25 dark:text-gray-main">{{ metric.label }}</div>
        <div class="truncate font-mono font-semibold" :class="{ 'text-red-main': metric.isError }" :title="metric.title">:{{ metric.value }}</div>
      </div>
    </div>

    <div class="mt-3 flex flex-wrap items-center gap-2 text-xs mb-2">
      <VButton size="sm" variant="soft" color="gray" :disabled="isBusy || !isCurrentSelected" @click="emit('reload')">
        <VIcon name="refresh" :size="14" />
        {{ $t('scriptStudio.reloaded') }}
      </VButton>
      <VButton size="sm" variant="soft" color="gray" :disabled="!isDownloadAvailable" @click="emit('download')">
        <VIcon name="download" :size="14" />
        {{ $t('scriptStudio.download') }}
      </VButton>
      <VButton size="sm" variant="soft" color="gray" :disabled="isBusy" @click="emit('stop-all')">
        <VIcon name="stop-circle" :size="14" />
        {{ $t('scriptStudio.stopAll') }}
      </VButton>
      <span class="ml-auto font-mono text-xs text-gray-dark-25 dark:text-gray-main">{{ $t('scriptStudio.limits') }}: {{ instrPerTick }} / {{ usPerTick }} µs</span>
    </div>

    <slot></slot>
  </div>
</template>
