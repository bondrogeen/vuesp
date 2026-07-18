<template>
  <div class="md:flex items-center mb-4">
    <div class="flex-auto text-gray-600 bg:text-gray-400 mb-2">{{ $t('selectLang') }}</div>

    <div class="md:max-w-50">
      <v-select :model-value="getValueLocale(locale)" :label="$t('lang')" :items="locales" @change="onLocale" />
    </div>
  </div>

  <div class="flex items-center mb-4">
    <div class="flex-auto text-gray-600 bg:text-gray-400">{{ $t('rebootDev') }}</div>

    <v-button class="min-w-[100px]" color="red" outline @click="emit('reboot', $event)">{{ $t('reboot') }}</v-button>
  </div>

  <div class="flex items-center mb-4">
    <div class="flex-auto text-gray-600 bg:text-gray-400">{{ $t('resetConf') }}</div>

    <v-button class="min-w-[100px]" color="red" @click="emit('reset', $event)">{{ $t('reset') }}</v-button>
  </div>
</template>

<script setup lang="ts">
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

const getValueLocale = (local: string) => locales.find((i) => i.value === local)?.name;
</script>
