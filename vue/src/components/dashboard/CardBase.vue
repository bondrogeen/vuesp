<template>
  <div
    class="flex flex-col border aspect-square rounded-sm bg-white dark:bg-gray-900 border-b border-gray-300 dark:border-gray-700 p-2 hover:border-blue-400 dark:hover:border-blue-700 transition-all"
    @click="onClick"
  >
    <div class="flex-auto flex justify-between">
      <div>
        <component :is="getIcon" class="size-8" />
      </div>
      <slot name="header"></slot>
    </div>

    <div>
      <h6 class="text-sm text-gray-500 font-medium text-ellipsis overflow-hidden text-nowrap">{{ name }}</h6>

      <div class="text-sm text-ellipsis overflow-hidden text-nowrap">{{ getModifyValue(value) }}</div>
    </div>

    <AppDialog v-if="dialog" size="sm" :title="name" @close="dialog = false">
      <template #header>
        <div class="flex-auto flex items-center justify-between gap-6 me-2">
          <div class="flex justify-center">
            <component :is="getIcon" class="size-11" />
          </div>

          <v-button type="icon" size="" @click="onEdit">
            <icon-dots />
          </v-button>
        </div>
      </template>

      <div class="min-h-60 relative">
        <div class="pt-10">
          <h3 class="text-center text-3xl text-ellipsis overflow-hidden">{{ name }}</h3>

          <slot name="dialog"></slot>

          <p class="text-center text-xl">{{ getModifyValue(value) }}</p>
        </div>
      </div>
    </AppDialog>
  </div>
</template>

<script setup lang="ts">
import type { TypeProperty } from '@/utils/VuespData.ts';
import { ref, computed } from 'vue';

import AppDialog from '@/components/app/AppDialog.vue';

const dialog = ref(false);

const emit = defineEmits<{
  (e: 'edit', event: Event): void;
}>();

const { name, value, icon, modifyValue } = defineProps<TypeProperty>();

const getIcon = computed(() => (icon ? `icon-${icon}` : 'icon-therm'));

const onEdit = (e: Event) => emit('edit', e);

const onClick = () => (dialog.value = true);

const getModifyValue = (value: any) => modifyValue?.(value) || value;
</script>
