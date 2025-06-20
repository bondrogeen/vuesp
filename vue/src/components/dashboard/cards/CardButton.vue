<template>
  <CardBase v-bind="props" @click="onClick">
    <template #header>
      <div class="flex justify-end">
        <v-button type="icon" size="sm" @click.stop="setState" :class="props.value ? 'text-blue-600 dark:text-blue-400' : ''">
          <icon-power class="size-10" />
        </v-button>
      </div>
    </template>
    <template #dialog>
      <div class="flex justify-center py-4">
        <v-button type="icon" size="sm" class="h-18 w-18" @click.stop="setState" :class="props.value ? 'text-blue-600 dark:text-blue-400' : ''">
          <icon-power class="size-16" />
        </v-button>
      </div>
    </template>
  </CardBase>
</template>

<script setup lang="ts">
import type { TypeProperty } from '@/vuesp-data/types.ts';

import CardBase from '@/components/dashboard/cards/CardBase.vue';

const emit = defineEmits<{
  (e: 'click', event: Event): void;
  (e: 'setState', item: any): void;
}>();

const props = defineProps<TypeProperty>();

const onClick = (event: Event) => emit('click', event);
const setState = () => emit('setState', props);
</script>
