<template>
  <button
    class="flex hover:text-dark-900 relative h-11 w-11 items-center justify-center rounded-full border border-gray-200 bg-white text-gray-500 transition-colors hover:bg-gray-100 hover:text-gray-700 dark:border-gray-800 dark:bg-gray-900 dark:text-gray-400 dark:hover:bg-gray-800 dark:hover:text-white"
    @click.prevent="emit('click', $event)"
  >
    <slot></slot>
  </button>
</template>

<script setup lang="ts">
// import type { MenuType, MenuItemType, MenuChildType, TypeStateInfo } from '@/types/types.ts';

// interface Props {
//   sidebarToggle?: boolean;
//   menu?: MenuType[];
//   info?: TypeStateInfo;
// }

// const { info = {}, sidebarToggle = false, menu = [] } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'click', value: Event): void;
}>();
</script>
