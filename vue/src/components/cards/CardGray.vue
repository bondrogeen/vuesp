<template>
  <div class="rounded-2xl border border-gray-200 bg-white p-5 dark:border-gray-800 dark:bg-white/[0.03] sm:p-6">
    <div class="mb-6 flex items-center justify-between">
      <h3 class="text-lg font-semibold text-gray-800 dark:text-white/90">{{ title }}</h3>

      <div>
        <slot name="header"></slot>
      </div>
    </div>

    <div class="max-w-full">
      <slot></slot>
    </div>
  </div>
</template>

<script setup lang="ts">
interface Props {
  title: string;
}

const { title = 'Title' } = defineProps<Props>();
</script>
