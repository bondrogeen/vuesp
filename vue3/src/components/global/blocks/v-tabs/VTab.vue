<template>
  <div v-show="isActive">
    <slot></slot>
  </div>
</template>
<script setup>
import { ref, defineProps } from 'vue';
// const emit = defineEmits(['close']);
defineProps({
  type: { type: String, default: 'VTab' },
  title: { type: String, default: 'Attention !' },
});

const isActive = ref(false);
</script>
