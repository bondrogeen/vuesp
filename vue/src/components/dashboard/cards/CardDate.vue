<template>
  <CardBase v-bind="props" @click="onClick">
    <template #dialog="{ value }">
      <div class="flex justify-center relative mx-2 my-6">
        <v-text-field :modelValue="datetime(value)" type="datetime-local" label="Date" @change="onChangeDate"></v-text-field>
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

const datetime = (now: number) => new Date(now * 1000).toISOString().slice(0, 16);

const onChangeDate = (e: any) => {
  const now: number = e?.target?.valueAsNumber;
  console.log(now);

  if (now) {
    emit('setState', { ...props, value: now / 1000 });
  }
};

const onClick = (event: Event) => emit('click', event);
</script>
