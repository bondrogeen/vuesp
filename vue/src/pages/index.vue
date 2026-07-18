<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>{{ $t('home') }}</h1>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
          </v-button>
        </template>

        <v-list :items="listMenu" @click="onMenu" />
      </v-dropdown>
    </div>

    <div class="flex flex-col gap-6">


    <p class="text-lg font-bold mb-4 text-gray-400 max-w-200">
      {{ pkg.description }}
    </p>

     <card-main :title="$t('icons')" >
      <div class="flex gap-3 mb-6">

        <icon-ri-home-line  />
        <icon-ri-mail-line class="hover:scale-130 transition-all" />
        <icon-ri-printer-line class="hover:text-blue-500 transition-all" />
        <icon-ri-chat-1-line class="text-blue-500"  />
        <icon-ri-settings-2-line class="size-4"/>
      </div>

      <v-button color="blue" class="" href="https://remixicon.com/" target="_blank">https://remixicon.com/</v-button>
      </card-main>

    <div>
      {{ main.info }}
    </div>

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
