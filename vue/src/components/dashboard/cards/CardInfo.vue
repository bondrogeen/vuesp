<template>
  <CardBase v-bind="props" @click="onClick">
    <slot name="dialog"></slot>
  </CardBase>
</template>

<script setup lang="ts">
import type { TypeProperty } from '@/vuesp-data/types.ts';

import CardBase from '@/components/dashboard/cards/CardBase.vue';

const emit = defineEmits<{
  (e: 'click', event: Event): void;
}>();

const props = defineProps<TypeProperty>();

const onClick = (e: Event) => emit('click', e);
</script>
