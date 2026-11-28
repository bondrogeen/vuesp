<script setup lang="ts">
import { VButton, VDialog } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import type { IScriptDiscardDialogEmits } from './types';

const { $t } = useLocale();
const emit = defineEmits<IScriptDiscardDialogEmits>();
</script>

<template>
  <VDialog size="sm" :title="$t('scriptStudio.unsavedTitle')" @close="emit('cancel')">
    <p class="text-sm text-gray-dark-30 dark:text-white/70">{{ $t('scriptStudio.unsavedHint') }}</p>
    <div class="mt-5 flex justify-end gap-2">
      <VButton size="sm" variant="outline" color="gray" @click="emit('discard')">
        {{ $t('scriptStudio.dontSave') }}
      </VButton>
      <VButton size="sm" color="blue" variant="solid" @click="emit('save')">
        {{ $t('save') }}
      </VButton>
    </div>
  </VDialog>
</template>
