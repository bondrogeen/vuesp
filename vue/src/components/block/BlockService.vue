<template>
  <div class="flex items-center mb-4">
    <v-select :value="locale" :label="$t('selectLang')" :list="locales" @change="onLocale"></v-select>
  </div>

  <div class="flex items-center mb-4">
    <div class="flex-auto text-gray-600 bg:text-gray-400">{{ $t('rebootDev') }}</div>

    <v-button class="min-w-[100px]" color="red" size="small" outline @click="emit('reboot', $event)">{{ $t('reboot') }}</v-button>
  </div>

  <div class="flex items-center mb-4">
    <div class="flex-auto text-gray-600 bg:text-gray-400">{{ $t('resetConf') }}</div>

    <v-button class="min-w-[100px]" color="red" size="small" @click="emit('reset', $event)">{{ $t('reset') }}</v-button>
  </div>
</template>

<script setup lang="ts">
import { defineEmits } from 'vue';
import type { IListItem } from 'vuesp-components/types';

interface Props {
  locale: string;
  locales: IListItem[];
}

const { locales } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'reboot' | 'reset', event: Event): void;
  (e: 'locale', item: IListItem): void;
}>();

const onLocale = (item: IListItem) => emit('locale', item);
</script>
