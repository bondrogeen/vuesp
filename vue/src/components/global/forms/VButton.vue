<template>
  <button class="flex justify-center items-center px-4 transition text-title-1 disabled:opacity-50 disabled:text-gray-400 disabled:border-gray-400" :class="getClass" @click="onClick">
    <slot></slot>
  </button>
</template>

<script setup>
import { defineProps, defineEmits, computed } from 'vue';

const props = defineProps({
  color: { type: String, default: 'primary' },
  size: { type: String, default: 'normal' },
  block: { type: Boolean, default: false },
});

const emit = defineEmits(['click']);
const onClick = e => emit('click', e);

const colors = {
  primary: 'bg-white text-black border border-primary rounded hover:border-secondary',
};
const sizes = { normal: 'h-[40px]', small: 'h-[32px]' };
const getClass = computed(() => [colors[props.color], sizes[props.size], { 'w-full': props.block }]);
</script>
