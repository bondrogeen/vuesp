<template>
  <div class="flex items-center mb-4">
    <div class="flex-auto text-gray-600 bg:text-gray-400">{{ $i18n.t('rebootDevice') }}</div>

    <v-button class="min-w-[100px]" color="red" size="small" outline @click="emit('reboot', $event)">{{ $i18n.t('reboot') }}</v-button>
  </div>

  <div class="flex items-center mb-4">
    <div class="flex-auto text-gray-600 bg:text-gray-400">{{ $i18n.t('ResetConfig') }}</div>

    <v-button class="min-w-[100px]" color="red" size="small" @click="emit('reset', $event)">{{ $i18n.t('reset') }}</v-button>
  </div>
</template>

<script setup lang="ts">
import { defineEmits } from 'vue';

const emit = defineEmits<{
  (e: 'reboot' | 'reset', event: Event): void;
}>();
</script>
