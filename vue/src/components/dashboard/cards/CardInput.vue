<template>
  <CardBase v-bind="props" @click="onClick">
    <template #dialog="{ value, name }">
      <div class="flex justify-center relative mx-2 my-6">
        <v-text-field :modelValue="value" :label="name" @change="onChange"></v-text-field>
      </div>
    </template>
  </CardBase>
</template>

<script setup lang="ts">
import type { TypeProperty } from '@/vuesp-data/types.ts';

import CardBase from '@/components/dashboard/cards/CardBase.vue';

const emit = defineEmits<{
  (e: 'click', event: Event): void;
  (e: 'setState', item: any): void;
}>();

const props = defineProps<TypeProperty>();

const onChange = (e: any) => {
  const value: number = e?.target?.value;
  emit('setState', { ...props, value });
};

const onClick = (event: Event) => emit('click', event);
</script>
