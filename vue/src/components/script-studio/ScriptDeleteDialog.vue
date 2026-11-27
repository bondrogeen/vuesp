<script setup lang="ts">
import { VButton, VDialog } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import type { IScriptDeleteDialogEmits, IScriptDeleteDialogProps } from './types';

const { $t } = useLocale();
const { name, isBusy } = defineProps<IScriptDeleteDialogProps>();
const emit = defineEmits<IScriptDeleteDialogEmits>();
</script>

<template>
  <VDialog size="sm" :title="$t('scriptStudio.deleteConfirm')" @close="emit('close')">
    <p class="text-sm text-gray-dark-30 dark:text-white/70">
      <span class="font-mono">{{ name }}</span> — {{ $t('scriptStudio.deleteHint') }}
    </p>
    <div class="mt-5 flex justify-end gap-2">
      <VButton size="sm" variant="outline" color="gray" @click="emit('close')">
        {{ $t('scriptStudio.cancel') }}
      </VButton>
      <VButton size="sm" color="red" variant="solid" :disabled="isBusy" @click="emit('confirm')">
        {{ $t('scriptStudio.delete') }}
      </VButton>
    </div>
  </VDialog>
</template>
