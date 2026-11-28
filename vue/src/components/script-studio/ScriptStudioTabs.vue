<script setup lang="ts">
import { computed, nextTick, useTemplateRef, watch } from 'vue';
import { VButton, VIcon } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import { getScriptStatusMeta, getToneDotClass, isScriptActive } from '@/utils/scriptStudio';
import type { IScriptListItem } from '@/types';
import type { IScriptStudioTabsEmits, IScriptStudioTabsProps } from './types';

const ACTIVE_TAB_CLASS = 'cursor-default border-gray-light-10/70 bg-gray-dark-40/[0.05] text-gray-dark-40 dark:border-white/10 dark:bg-black/35 dark:text-white';
const IDLE_TAB_CLASS = 'cursor-pointer border-transparent bg-transparent text-gray-dark-30 hover:bg-black/5 dark:border-transparent dark:bg-gray-dark-40 dark:text-white/60 dark:hover:bg-white/5';

const { $t } = useLocale();
const { scripts, activeName, draftName, isBusy, isDirty, maxScripts, ranName } = defineProps<IScriptStudioTabsProps>();
const emit = defineEmits<IScriptStudioTabsEmits>();

const container = useTemplateRef<HTMLDivElement>('container');
const canCreate = computed(() => scripts.length < maxScripts);

const statusByScript = computed(() => new Map(scripts.map((script) => [script.name, getScriptStatusMeta(script, $t)])));
const isUnsaved = (scriptName: string): boolean => isDirty && scriptName === activeName;
const getTabTone = (script: IScriptListItem) =>
  isUnsaved(script.name) ? 'amber' : statusByScript.value.get(script.name)?.tone ?? 'gray';
const getTabTitle = (script: IScriptListItem) =>
  isUnsaved(script.name)
    ? $t('scriptStudio.unsaved')
    : statusByScript.value.get(script.name)?.label ?? '';
const isCompletedFlash = (script: IScriptListItem): boolean =>
  ranName === script.name && !isScriptActive(script.state);

const getTabClass = (scriptName: string): string => (scriptName === activeName ? ACTIVE_TAB_CLASS : IDLE_TAB_CLASS);

const scrollToActive = () => {
  void nextTick(() => {
    const element = container.value;
    if (!element) return;
    const active = element.querySelector<HTMLElement>(`[id="tab-${activeName}"]`);
    if (!active) return;
    const left = active.offsetLeft;
    const right = left + active.offsetWidth;
    if (left < element.scrollLeft) {
      element.scrollLeft = Math.max(0, left - 8);
    } else if (right > element.scrollLeft + element.clientWidth) {
      element.scrollLeft = right - element.clientWidth + 8;
    }
  });
};

const onWheel = (event: WheelEvent) => {
  const element = container.value;
  if (!element || event.ctrlKey || event.deltaX !== 0 || event.deltaY === 0) return;
  const maxScroll = element.scrollWidth - element.clientWidth;
  if (maxScroll <= 0) return;
  if (event.deltaY < 0 && element.scrollLeft <= 0) return;
  if (event.deltaY > 0 && element.scrollLeft >= maxScroll) return;
  event.preventDefault();
  element.scrollLeft = Math.max(0, Math.min(maxScroll, element.scrollLeft + event.deltaY));
};

watch([() => activeName, () => scripts.length], scrollToActive, { immediate: true });

defineExpose({ scrollToActive });
</script>

<template>
  <div class="flex items-end bg-black/[0.02] dark:bg-white/[0.02]">
    <VButton
      size="xs"
      variant="soft"
      color="gray"
      :disabled="!canCreate"
      :title="canCreate ? $t('scriptStudio.newScript') : $t('scriptStudio.scriptsLimit')"
      :aria-label="canCreate ? $t('scriptStudio.newScript') : $t('scriptStudio.scriptsLimit')"
      class="mx-1 mb-1"
      @click="emit('create')"
    >
      <VIcon name="add" :size="12" />
    </VButton>
    <div
      ref="container"
      class="no-scrollbar flex flex-1 gap-1 overflow-x-auto overflow-y-hidden pt-1.5"
      @wheel="onWheel"
    >
      <div class="flex min-w-max items-end">
        <div
          v-for="script in scripts"
          :key="script.name"
          :id="`tab-${script.name}`"
          class="flex items-center gap-1 rounded-t-lg border border-b-0 px-2 py-1"
          :class="getTabClass(script.name)"
          @click="emit('select', script)"
        >
          <div class="flex min-w-0 items-center gap-1 rounded-none p-0 text-left me-1">
            <span class="size-1.5 shrink-0 rounded-full" :class="getToneDotClass(getTabTone(script))" :title="getTabTitle(script)" />
            <span class="max-w-40 truncate font-mono text-xs" :title="script.name">{{ script.name }}</span>
          </div>

          <VButton
            size="none"
            variant="ghost"
            color="gray"
            class="rounded p-0.5 opacity-70 transition hover:opacity-100 disabled:opacity-40"
            :disabled="isBusy"
            :title="isCompletedFlash(script) ? $t('scriptStudio.stateCompleted') : isScriptActive(script.state) ? $t('scriptStudio.stop') : $t('scriptStudio.run')"
            :aria-label="isCompletedFlash(script) ? $t('scriptStudio.stateCompleted') : isScriptActive(script.state) ? $t('scriptStudio.stop') : $t('scriptStudio.run')"
            @click.stop="emit('toggle-run', script)"
          >
            <VIcon
              :name="isCompletedFlash(script) ? 'check' : isScriptActive(script.state) ? 'stop-circle' : 'play-circle'"
              :size="14"
              :class="{ 'text-green-main': isCompletedFlash(script) }"
            />
          </VButton>
          <VButton
            size="none"
            variant="ghost"
            color="red"
            class="rounded p-0.5 opacity-70 transition hover:bg-red-main/10 hover:opacity-100"
            :title="$t('scriptStudio.delete')"
            :aria-label="$t('scriptStudio.delete')"
            @click.stop="emit('remove', script.name)"
          >
            <VIcon name="trash" :size="14" />
          </VButton>
        </div>

        <div
          v-if="draftName"
          :id="`tab-${draftName}`"
          class="flex items-center gap-1 rounded-t-lg border border-b-0 border-amber-main/40 bg-white/70 px-2 py-1 text-gray-dark-40 dark:border-amber-main/40 dark:bg-white/10 dark:text-white"
        >
          <span class="size-1.5 shrink-0 rounded-full bg-amber-main" />
          <span class="max-w-40 truncate font-mono text-xs" :title="draftName">{{ draftName }}</span>
        </div>

        <span v-if="scripts.length === 0 && !draftName" class="px-3 py-1 text-xs text-gray-dark-30 dark:text-white/60">
          {{ $t('noScripts') }}
        </span>
      </div>
    </div>
  </div>
</template>
