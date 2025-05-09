<template>
  <button class="relative flex justify-center items-center transition not-disabled:cursor-pointer" :class="getClass" @click="onClick">
    <slot></slot>
  </button>
</template>

<script setup lang="ts">
import { defineProps, defineEmits, computed } from 'vue';

interface Props {
  block?: boolean;
  color?: string;
  size?: string;
  type?: string;
}

const { color = '', block, size = 'normal', type = 'button' } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'click', value: Event): void;
}>();

const onClick = (e: Event) => emit('click', e);

const colors: { [index: string]: string } = {
  gray: 'text-gray-500 hover:text-gray-700 bg-white hover:bg-gray-100 dark:border-gray-800 dark:bg-gray-900 dark:text-gray-400 dark:hover:bg-gray-800 dark:hover:text-white',
  blue: 'bg-blue-700 hover:bg-blue-600 text-white border border-blue-700 rounded hover:border-blue-600 disabled:opacity-50 disabled:text-gray-400 disabled:bg-transparent disabled:border-gray-400',
};

const types: { [index: string]: string } = {
  button: 'font-medium px-4',
  icon: 'h-10 w-10 rounded-full border border-gray-200',
};

const sizes: { [index: string]: string } = { normal: 'h-10', small: 'h-8' };

const getSize = (size: string): any => sizes?.[size] || '';

const getType = (type: string): any => types?.[type] || '';

const getColor = (color: string): any => colors?.[color] || '';

const getClass = computed(() => [getColor(color), getType(type), getSize(size), { 'w-full': block }]);
</script>
