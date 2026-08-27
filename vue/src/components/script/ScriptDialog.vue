<script setup lang="ts">
import type { ValidationSchema } from 'vuesp-components/types';
import { required, maxLen } from '@/utils/validate';

import { useForm } from 'vuesp-components/composables';

const emit = defineEmits<{
  (e: 'add', v: string): void;
  (e: 'close', v: Event): void;
}>();

const { defineField, handleSubmit, isValid } = useForm({
  validationSchema: () =>
    ({
      name: [required, maxLen(8)],
    }) as ValidationSchema,
});
const [name, nameProps] = defineField<string>('name');

const addScript = () => {
  emit('add', name.value);
};

const onSubmit = handleSubmit(() => {
  addScript();
  name.value = ``;
  emit('close', new Event('close'));
});
</script>

<template>
  <div class="my-4">
    <v-text-field v-model="name" v-bind="nameProps" :label="$t('name')" />

    <v-button class="w-full" color="blue" :disabled="!isValid" @click="onSubmit">
      {{ $t('add') }}
    </v-button>
  </div>
</template>
