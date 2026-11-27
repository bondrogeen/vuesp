<script setup lang="ts">
import { VButton, VCard, VDialog } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import type { IScriptExamplesDialogEmits, IScriptExamplesDialogProps } from './types';

const { $t } = useLocale();
const { examples } = defineProps<IScriptExamplesDialogProps>();
const emit = defineEmits<IScriptExamplesDialogEmits>();
</script>

<template>
  <VDialog size="lg" :title="$t('examples')" @close="emit('close')">
    <div class="grid gap-2 sm:grid-cols-2">
      <VCard
        v-for="example in examples"
        :key="`example-${example.name}`"
        variant="outline"
        padding="none"
        hover
      >
        <VButton
          size="none"
          variant="ghost"
          color="gray"
          block
          class="rounded-xl p-4"
          @click="emit('load', example)"
        >
          <span class="block w-full text-left">
            <span class="block font-mono text-sm">{{ example.name }}</span>
            <span class="mt-1 block max-h-12 overflow-hidden whitespace-pre-wrap text-xs text-gray-dark-25 dark:text-gray-main">
              {{ example.source }}
            </span>
          </span>
        </VButton>
      </VCard>
    </div>
  </VDialog>
</template>
