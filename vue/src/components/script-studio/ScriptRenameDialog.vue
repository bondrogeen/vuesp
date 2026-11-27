<script setup lang="ts">
import { computed, ref } from 'vue';
import { VButton, VDialog, VTextField } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import { isValidScriptName } from '@/utils/scriptStudio';
import type { IScriptRenameDialogEmits, IScriptRenameDialogProps } from './types';

const { $t } = useLocale();
const { name, isBusy } = defineProps<IScriptRenameDialogProps>();
const emit = defineEmits<IScriptRenameDialogEmits>();

const value = ref(name);
const isNameValid = computed(() => isValidScriptName(value.value));

const onSave = () => {
  if (!isNameValid.value || isBusy) return;
  emit('save', value.value.toLowerCase());
};
</script>

<template>
  <VDialog size="sm" :title="$t('scriptStudio.editName')" @close="emit('close')">
    <VTextField
      v-model="value"
      size="sm"
      :label="$t('scriptStudio.id')"
      :message="$t('scriptStudio.nameHint')"
      @enter="onSave"
    />
    <div class="mt-5 flex justify-end gap-2">
      <VButton size="sm" variant="outline" color="gray" @click="emit('close')">
        {{ $t('scriptStudio.cancel') }}
      </VButton>
      <VButton size="sm" color="blue" variant="solid" :disabled="isBusy || !isNameValid" @click="onSave">
        {{ $t('save') }}
      </VButton>
    </div>
  </VDialog>
</template>
