<script setup lang="ts">
import type { IListItem } from '@/types';

import { KEYS } from '@/utils/const';

import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

import { VButton, VSelect } from 'vuesp-components/components';
import { useTemplateRef } from 'vue';

const { $t } = useLocale();

const { onSend } = useConnection((send) => {
  send(KEYS.DEVICE);
});

const listMenu: IListItem<number>[] = [{ name: $t('btnUpdate'), value: 2 }];

const onUpdate = () => {
  onSend('INFO');
};

const onMenu = ({ value }: IListItem<number>) => {
  if (value === 2) onUpdate();
};

const inputElement = useTemplateRef('img');

function captureFrame() {
  const img = inputElement.value;
  if (!img || !img.naturalWidth) return;

  const canvas = document.createElement('canvas');
  canvas.width = img.naturalWidth;
  canvas.height = img.naturalHeight;

  const ctx = canvas.getContext('2d');
  if (!ctx) return;
  ctx.drawImage(img, 0, 0);

  const a = document.createElement('a');
  a.download = `esp32cam-${Date.now()}.jpg`;
  a.href = canvas.toDataURL('image/jpeg', 0.92);
  a.click();
}
</script>

<template>
  <div class="container mx-auto pb-10">
    <section class="mb-6 flex items-center justify-between">
      <h1>{{ $t('home') }}</h1>

      <v-select modelValue="2" :items="listMenu" @change="onMenu">
        <template #activator="{ on }">
          <v-button type="icon" @click="on.click">
            <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
          </v-button>
        </template>
      </v-select>
    </section>

    <card-main title="Cam esp32">
      <template #header>
        <div class="col-span-full">
          <v-button color="blue" size="sm" @click="captureFrame">📸</v-button>
        </div>
      </template>

      <div class="flex justify-center">
        <img ref="img" :src="'/stream'" alt="ESP32-CAM Stream" />
      </div>
    </card-main>
  </div>
</template>
