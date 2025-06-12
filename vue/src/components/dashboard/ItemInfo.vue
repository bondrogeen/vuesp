<template>
  <div class="border aspect-square rounded-sm border-gray-700 p-2" @click="onClick">
    <h6 class="text-sm text-gray-500">{{ name }}</h6>
    {{ value }}
  </div>
</template>

<script setup lang="ts">
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
