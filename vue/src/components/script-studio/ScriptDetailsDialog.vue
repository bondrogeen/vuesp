<script setup lang="ts">
import { ref } from 'vue';
import { VButton, VDialog, VIcon } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import { copyText } from '@/utils/scriptStudio';
import type { IScriptDetailsDialogEmits, IScriptDetailsDialogProps } from './types';

type TDetailsTab = 'disasm' | 'hex';

const { $t } = useLocale();
const { result, hex } = defineProps<IScriptDetailsDialogProps>();
const emit = defineEmits<IScriptDetailsDialogEmits>();

const tab = ref<TDetailsTab>('disasm');

const onCopy = async () => {
  if (hex) await copyText(hex);
};
</script>

<template>
  <VDialog size="lg" :title="$t('scriptStudio.details')" @close="emit('close')">
    <div class="flex flex-wrap items-center gap-2 pb-2">
      <div class="flex h-8 items-center gap-1 rounded-md ring-1 ring-inset ring-gray-light-10/70 dark:ring-white/10">
        <VButton
          size="sm"
          :variant="tab === 'disasm' ? 'soft' : 'ghost'"
          :color="tab === 'disasm' ? 'blue' : 'gray'"
          @click="tab = 'disasm'"
        >
          {{ $t('scriptStudio.disassembly') }}
        </VButton>
        <VButton
          size="sm"
          :variant="tab === 'hex' ? 'soft' : 'ghost'"
          :color="tab === 'hex' ? 'blue' : 'gray'"
          @click="tab = 'hex'"
        >
          {{ $t('scriptStudio.hex') }}
        </VButton>
      </div>
      <VButton
        v-if="tab === 'hex'"
        size="sm"
        variant="ghost"
        color="gray"
        class="ml-auto"
        :disabled="!hex"
        @click="onCopy"
      >
        <VIcon name="copy" :size="14" />
        {{ $t('scriptStudio.copy') }}
      </VButton>
    </div>

    <pre
      v-if="tab === 'disasm'"
      class="m-0 max-h-96 overflow-auto whitespace-pre rounded-md bg-gray-dark-40/[0.05] p-4 font-mono text-xs leading-6 scrollbar dark:bg-black/35"
    >{{ result?.disassembly || '—' }}</pre>
    <pre
      v-else
      class="m-0 max-h-96 overflow-auto whitespace-pre rounded-md bg-gray-dark-40/[0.05] p-4 font-mono text-xs leading-6 scrollbar dark:bg-black/35"
    >{{ hex || '—' }}</pre>
  </VDialog>
</template>
