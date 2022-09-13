<template>
  <button class="v-button text-title-1" :class="[`v-button--${color}`, `v-button--${size}`]">
    <slot></slot>
  </button>
</template>

<script setup>
import { defineProps } from 'vue';
defineProps({
  color: { type: String, default: 'primary' },
  size: { type: String, default: 'normal' },
});
</script>

<style lang="scss">
.v-button {
  position: relative;
  height: 40px;
  padding: 0 15px;
  border-radius: 4px;
  border: 1px solid map-get($grey, 'lighten-2');
  color: var(--var-text-1);
  background: var(--var-bg-1);

  &:not(:disabled) {
    cursor: pointer;
  }
  &:disabled {
    background: map-get($grey, 'darken-2');
  }
  &--primary {
    border: 1px solid $primary;
  }
  &--small {
	font-size: 16px;
	height: 30px;
  }
  &--large {
	height: 50px;
  }
}
</style>
