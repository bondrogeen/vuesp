<template>
  <div>
    <v-list v-slot="{ item }" :list="items" className="">
      <div class="flex items-center w-full" @click="onSelect(item)">
        <div class="mr-2">
          <v-wifi-icon v-bind="item" />
        </div>

        <div>
          <div class="text-sm">{{ item.ssid }}</div>

          <div class="text-gray-400 text-xs">{{ $i18n.t('security') }} : {{ listEncryption[item.encryptionType] || 'unknown' }}</div>
        </div>
      </div>
    </v-list>

    <div v-if="!items.length" class="flex justify-center">
      <v-loader class="text-primary"></v-loader>
    </div>

    <div class="flex justify-end mt-4">
      <v-button color="blue" @click="onScan">{{ $i18n.t('scan') }}</v-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import type { IMessageScan } from '@/types';
import { defineEmits, defineProps } from 'vue';

interface Props {
  items: Partial<IMessageScan>[];
}

const { items = [] } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'scan', event: Event): void;
  (e: 'select', event: IMessageScan): void;
}>();

const listEncryption: string[] = ['OPEN', 'WEP', 'WPA_PSK', 'WPA2_PSK', 'WPA_WPA2_PSK', 'MAX', '', 'NO', 'AUTO'];

const onSelect = (item: IMessageScan) => emit('select', item);
const onScan = (e: Event) => emit('scan', e);
</script>
