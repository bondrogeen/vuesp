<script setup lang="ts">
import { VButton, VIcon } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import type { IScriptStudioActionBarEmits, IScriptStudioActionBarProps } from './types';

const { $t } = useLocale();
const { lineCount, byteCount, stackMax } = defineProps<IScriptStudioActionBarProps>();
const emit = defineEmits<IScriptStudioActionBarEmits>();
</script>

<template>
  <div class="flex flex-wrap items-center justify-between gap-2">
    <div class="flex flex-wrap items-center gap-2">
      <VButton size="sm" variant="soft" color="gray" @click="emit('insert')">
        <VIcon name="code" :size="14" />
        {{ $t('scriptStudio.insert') }}
      </VButton>
      <VButton size="sm" variant="soft" color="gray" @click="emit('examples')">
        <VIcon name="document" :size="14" />
        {{ $t('examples') }}
      </VButton>
      <span class="hidden h-4 w-px bg-gray-light-10/70 dark:bg-white/10 sm:block" />
      <VButton size="sm" variant="soft" color="gray" @click="emit('details')">
        <VIcon name="list-check" :size="14" />
        {{ $t('scriptStudio.details') }}
      </VButton>
    </div>

    <span class="font-mono text-xs text-gray-dark-25 dark:text-gray-main">
      {{ lineCount }} {{ $t('scriptStudio.lines') }} · {{ byteCount }} {{ $t('scriptStudio.bytes') }} · {{ $t('scriptStudio.stack') }}
      {{ stackMax ?? '—' }}
    </span>
  </div>
</template>
