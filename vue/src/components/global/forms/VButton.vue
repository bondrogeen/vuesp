<template>
  <button class="v-button text-title-1" :class="[`v-button--${color}`, `v-button--${size}`]" @click="onClick">
    <slot></slot>
  </button>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue';
defineProps({
  color: { type: String, default: 'primary' },
  size: { type: String, default: 'normal' },
});

const emit = defineEmits(['click']);
const onClick = e => emit('click', e);
</script>

<style lang="scss">
.v-button {
  position: relative;
  height: 40px;
  padding: 0 15px;
  border-radius: 4px;
  border: 1px solid color('grey', 'lighten-2');
  color: var(--text-1);
  background: var(--bg-1);
  min-width: 100px;
  &:not(:disabled) {
    cursor: pointer;
  }
  &--primary {
    color: var(--text-1);
    background: var(--bg-1);
    border: 1px solid color('app', 'primary');
  }
  &--small {
    font-size: 16px;
    height: 30px;
  }
  &:disabled {
    opacity: 0.5;
    border: 1px solid var(--border-1);
  }
}
</style>
