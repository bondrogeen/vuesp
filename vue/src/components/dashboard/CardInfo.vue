<template>
  <CardBase :name="name" :value="value" :id="id" @click="onClick">
    <slot name="dialog"></slot>
  </CardBase>
</template>

<script setup lang="ts">
import CardBase from '@/components/dashboard/CardBase.vue';

interface Props {
  id: string;
  name?: string;
  key?: string;
  value?: string | number | Date;
}

const emit = defineEmits<{
  (e: 'click', { id, value }: Props): void;
}>();

const { id, name, key, value } = defineProps<Props>();

const onClick = () => emit('click', { id, value });
</script>
