<script setup lang="ts">
import { computed } from 'vue';
import { VButton, VIcon, VList, VDropdown } from 'vuesp-components/components';
import type { IListItem } from 'vuesp-components/types';

import { useLocale } from '@/composables/useLocale';
import { getToneDotClass, getToneTagClass } from '@/utils/scriptStudio';
import type { IScriptStudioToolbarEmits, IScriptStudioToolbarProps } from './types';

const { $t } = useLocale();
const { name, status, isBusy, isVmAvailable, isSettingsOpen, isDownloadAvailable, isCurrentSelected } = defineProps<IScriptStudioToolbarProps>();
const emit = defineEmits<IScriptStudioToolbarEmits>();

type TMenuAction = 'reload' | 'download' | 'stop-all' | 'remove';

interface IMenuEntry extends IListItem<TMenuAction> {
  icon: string;
  isDanger?: boolean;
}

const menuItems = computed<IMenuEntry[]>(() => {
  const items: IMenuEntry[] = [{ name: $t('scriptStudio.reloaded'), value: 'reload', icon: 'refresh' }];
  if (isDownloadAvailable) {
    items.push({ name: $t('scriptStudio.download'), value: 'download', icon: 'download' });
  }
  items.push({ name: $t('scriptStudio.stopAll'), value: 'stop-all', icon: 'stop-circle' });
  if (isCurrentSelected) {
    items.push({ name: $t('scriptStudio.delete'), value: 'remove', icon: 'trash', isDanger: true });
  }
  return items;
});

const onMenu = (item: IMenuEntry, hide: (event: Event) => void) => {
  hide(new Event('hide'));
  const { value } = item;
  if (value === 'reload') emit('reload');
  if (value === 'download') emit('download');
  if (value === 'stop-all') emit('stop-all');
  if (value === 'remove') emit('remove');
};
</script>

<template>
  <div class="flex flex-wrap items-center gap-3 border-b border-gray-light-10/70 px-3 py-2.5 dark:border-white/10">
    <span class="size-2.5 rounded-full" :class="getToneDotClass(status.tone)" />
    <span class="max-w-44 truncate font-mono text-sm font-medium">{{ name }}</span>
    <VButton type="icon" size="sm" variant="ghost" color="gray" :title="$t('scriptStudio.editName')" :aria-label="$t('scriptStudio.editName')" @click="emit('rename')">
      <VIcon name="edit" :size="16" />
    </VButton>

    <span class="rounded px-2 py-0.5 text-xs" :class="getToneTagClass(status.tone)">{{ status.label }}</span>
    <span v-if="!isVmAvailable" class="rounded bg-red-main/10 px-2 py-0.5 text-xs text-red-main" :title="$t('scriptStudio.vmUnavailableHint')">
      {{ $t('scriptStudio.vmUnavailable') }}
    </span>

    <div class="ml-auto flex flex-wrap items-center gap-2">
      <VButton size="sm" color="blue" variant="soft" :disabled="isBusy || !isVmAvailable" :title="$t('save')" :aria-label="$t('save')" @click="emit('save')">
        <VIcon name="save" :size="14" />
        {{ $t('save') }}
      </VButton>

      <VButton
        type="icon"
        size="sm"
        :variant="isSettingsOpen ? 'soft' : 'ghost'"
        color="gray"
        :title="$t('scriptStudio.settings')"
        :aria-label="$t('scriptStudio.settings')"
        @click="emit('toggle-settings')"
      >
        <VIcon name="settings" :size="17" />
      </VButton>

      <VDropdown right="0">
        <template #activator="{ on }">
          <VButton type="icon" size="sm" variant="ghost" color="gray" :disabled="isBusy" :title="$t('scriptStudio.more')" :aria-label="$t('scriptStudio.more')" @click="on.click">
            <VIcon name="more" :size="16" />
          </VButton>
        </template>
        <template #default="{ hide }">
          <VList :items="menuItems" @click="(item) => onMenu(item, hide)">
            <template #default="{ item }">
              <VIcon :name="item.icon" :size="14" />
              <span class="ms-2" :class="{ 'text-red-main': item.isDanger }">{{ item.name }}</span>
            </template>
          </VList>
        </template>
      </VDropdown>
    </div>

    <slot></slot>
  </div>
</template>
