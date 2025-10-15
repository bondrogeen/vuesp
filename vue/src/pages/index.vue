<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>{{ $t('home') }}</h1>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <v-icons name="Dots" class="rotate-90"></v-icons>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenu"></v-list>
      </v-dropdown>
    </div>

    <p class="text-lg font-bold mb-4">
      {{ pkg.description }}
    </p>

    <div>
      {{ main.info }}
    </div>
  </div>
</template>

<script setup lang="ts">
import type { IListItem } from '@/types';

import { KEYS } from '@/types';

import { useConnection } from '@/composables/useConnection';

import { useLocale } from '@/composables/useLocale';

const { $t } = useLocale();

const { pkg, main, onSend } = useConnection((send) => {
  send(KEYS.DEVICE);
});

const listMenu: IListItem[] = [{ name: $t('btnUpdate'), value: 2 }];

const onUpdate = () => {
  onSend('INFO');
};

const onMenu = ({ value }: IListItem) => {
  if (value === 2) onUpdate();
};
</script>
