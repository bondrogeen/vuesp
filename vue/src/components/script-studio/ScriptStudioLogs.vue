<script setup lang="ts">
import { watch } from 'vue';
import { VButton, VIcon, VTextField } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import { useScriptStudioLogs } from '@/composables/script/useScriptStudioLogs';
import type { IScriptStudioLogsEmits } from './types';

const { name } = defineProps<{ name: string }>();
const emit = defineEmits<IScriptStudioLogsEmits>();
const { $t } = useLocale();

const {
  liveLogs,
  visibleLogs,
  count,
  isOpen,
  isPaused,
  isSearchOpen,
  isAutoScroll,
  isCopied,
  mode,
  search,
  onOpen,
  onToggleOpen,
  onTogglePause,
  onToggleSearch,
  onClear,
  onFollow,
  onScroll,
  onCopy,
  onDownload,
  getLogText,
} = useScriptStudioLogs(() => name);

watch(count, (value) => emit('update:count', value), { immediate: true });

defineExpose({ open: onOpen });
</script>

<template>
  <div ref="logsPanel">
    <div class="flex flex-wrap items-center justify-between gap-2 bg-gray-dark-40/[0.03] px-3 py-2 dark:bg-black/20">
      <div class="flex flex-wrap items-center gap-2">
        <VButton size="sm" variant="ghost" color="gray" @click="onToggleOpen">
          <VIcon name="code" :size="14" />
          <span>{{ $t('logs') }}</span>
          <VIcon name="chevron-down" :size="14" class="transition" :class="{ 'rotate-180': isOpen }" />
        </VButton>

        <span class="hidden h-4 w-px bg-gray-light-10/70 dark:bg-white/10 sm:block" />

        <div class="flex h-8 items-center gap-1 rounded-md ring-1 ring-inset ring-gray-light-10/70 dark:ring-white/10">
          <VButton size="sm" :variant="mode === 'text' ? 'soft' : 'ghost'" :color="mode === 'text' ? 'blue' : 'gray'" @click="mode = 'text'">
            {{ $t('scriptStudio.textMode') }}
          </VButton>
          <VButton size="sm" :variant="mode === 'hex' ? 'soft' : 'ghost'" :color="mode === 'hex' ? 'blue' : 'gray'" @click="mode = 'hex'">
            {{ $t('scriptStudio.hexMode') }}
          </VButton>
        </div>

        <span class="text-xs text-gray-dark-25 dark:text-gray-main">{{ visibleLogs.length }}/{{ liveLogs.length }} {{ $t('scriptStudio.lines') }}</span>
        <span v-if="isPaused" class="text-xs font-medium text-amber-main">
          {{ $t('scriptStudio.paused') }}
        </span>
      </div>

      <div class="flex flex-wrap items-center gap-1">
        <VButton
          type="icon"
          size="sm"
          :variant="isPaused ? 'soft' : 'ghost'"
          :color="isPaused ? 'amber' : 'gray'"
          :title="isPaused ? $t('scriptStudio.resume') : $t('scriptStudio.pause')"
          :aria-label="isPaused ? $t('scriptStudio.resume') : $t('scriptStudio.pause')"
          @click="onTogglePause"
        >
          <VIcon :name="isPaused ? 'play-circle' : 'stop-circle'" :size="16" />
        </VButton>
        <VButton type="icon" size="sm" :variant="isSearchOpen ? 'soft' : 'ghost'" color="gray" :title="$t('scriptStudio.search')" :aria-label="$t('scriptStudio.search')" @click="onToggleSearch">
          <VIcon name="search" :size="16" />
        </VButton>
        <VButton type="icon" size="sm" variant="ghost" color="gray" :disabled="liveLogs.length === 0" :title="$t('scriptStudio.clear')" :aria-label="$t('scriptStudio.clear')" @click="onClear">
          <VIcon name="eraser" :size="16" />
        </VButton>
        <VButton
          type="icon"
          size="sm"
          variant="ghost"
          color="gray"
          :disabled="visibleLogs.length === 0"
          :title="isCopied ? $t('scriptStudio.copied') : $t('scriptStudio.copy')"
          :aria-label="$t('scriptStudio.copy')"
          @click="onCopy"
        >
          <VIcon :name="isCopied ? 'check' : 'copy'" :size="16" :class="{ 'text-green-main': isCopied }" />
        </VButton>
        <VButton
          type="icon"
          size="sm"
          variant="ghost"
          color="gray"
          :disabled="visibleLogs.length === 0"
          :title="$t('scriptStudio.downloadLog')"
          :aria-label="$t('scriptStudio.downloadLog')"
          @click="onDownload"
        >
          <VIcon name="download" :size="16" />
        </VButton>
      </div>
    </div>

    <div v-if="isSearchOpen" class="border-b border-gray-light-10/70 px-3 py-2 dark:border-white/10">
      <VTextField v-model="search" size="sm" hide-message :label="$t('scriptStudio.search')" :placeholder="$t('scriptStudio.searchPlaceholder')" />
    </div>

    <div v-if="isOpen" class="relative">
      <div ref="logEl" class="h-[min(22vh,30rem)] min-h-42 overflow-auto bg-gray-dark-40/[0.05] p-4 font-mono text-xs leading-6 scrollbar dark:bg-black/35" @scroll="onScroll">
        <div v-if="visibleLogs.length === 0" class="flex h-full flex-col items-center justify-center gap-3 text-center text-gray-dark-25 dark:text-gray-light-10">
          <span class="grid size-12 place-items-center rounded-full bg-gray-dark-40/5 text-gray-main dark:bg-white/5">
            <VIcon name="code" :size="24" />
          </span>
          <p>{{ search ? $t('scriptStudio.noMatches') : $t('scriptStudio.logEmpty') }}</p>
        </div>
        <div v-for="(log, index) in visibleLogs" :key="index" class="break-all">
          <span class="mr-2 text-gray-dark-25 dark:text-gray-main">{{ log.time }}</span>
          <span class="whitespace-pre-wrap text-gray-dark-40 dark:text-gray-light-10">
            {{ getLogText(log.text, mode) }}
          </span>
        </div>
      </div>
      <div v-if="!isAutoScroll && visibleLogs.length > 0" class="absolute bottom-3 right-3 z-10 flex">
        <VButton size="sm" color="blue" variant="soft" class="flex" @click="onFollow">
          <VIcon name="chevron-down" :size="14" />
          {{ $t('scriptStudio.follow') }}
        </VButton>
      </div>
    </div>
  </div>
</template>
