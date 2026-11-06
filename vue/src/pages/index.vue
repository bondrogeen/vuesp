<script setup lang="ts">
import type { IListItem } from '@/types';

import { KEYS } from '@/utils/const';

import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

import { VButton, VSelect } from 'vuesp-components/components';
import { computed } from 'vue';

const { $t } = useLocale();

const { main, onSend } = useConnection((send) => {
  send(KEYS.DEVICE);
});

const listMenu: IListItem<number>[] = [{ name: $t('btnUpdate'), value: 2 }];

const onUpdate = () => {
  onSend('INFO');
};

const onMenu = ({ value }: IListItem<number>) => {
  if (value === 2) onUpdate();
};

const voltage = computed(() => main.value.device.voltage || 0);
const current = computed(() => main.value.device.current || 0);
const power = computed(() => main.value.device.power || 0);
const frequency = computed(() => main.value.device.frequency || 0);
const activeEnergy = computed(() => Math.floor((main.value.device?.activeEnergy || 0) / 100));
const activeEnergyDrob = computed(() => String((main.value.device?.activeEnergy || 0) % 100).padStart(2, '0'));

const metrics = computed(() => [
  { label: 'Напряжение', value: (voltage.value / 10).toFixed(1), unit: 'В', color: 'text-blue-500 dark:text-blue-400' },
  { label: 'Ток', value: (current.value / 100).toFixed(2), unit: 'А', color: 'text-violet-500 dark:text-violet-400' },
  { label: 'Мощность', value: power.value, unit: 'Вт', color: 'text-amber-500 dark:text-amber-400' },
  { label: 'Частота', value: (frequency.value / 100).toFixed(2), unit: 'Гц', color: 'text-teal-500 dark:text-teal-400' },
]);
</script>

<template>
  <div class="container mx-auto">
    <section class="mb-6 flex items-center justify-between">
      <h1>{{ $t('home') }}</h1>

      <v-select model-value="2" :items="listMenu" @change="onMenu">
        <template #activator="{ on }">
          <v-button type="icon" @click="on.click">
            <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
          </v-button>
        </template>
      </v-select>
    </section>

    <card-main title="DDS6619" class="p-6 sm:p-7">
      <div class="glass-panel relative overflow-hidden rounded-xl px-5 py-8 text-center">
        <div class="mb-3 text-[11px] uppercase tracking-[0.3em] text-gray-500">Показания</div>

        <div class="font-mono text-3xl font-bold leading-none tracking-[0.2em] sm:text-5xl">
          <span class="text-emerald-500 dark:text-emerald-400">{{ activeEnergy }}</span>
          <span class="text-red-500 dark:text-red-400">.{{ activeEnergyDrob }}</span>
        </div>

        <div class="mt-3 text-sm tracking-[0.2em] text-gray-500">кВт·ч</div>
      </div>

      <div class="mt-4 grid grid-cols-1 gap-3 sm:grid-cols-2 xl:grid-cols-4">
        <div v-for="metric in metrics" :key="metric.label" class="glass-panel rounded-xl px-4 py-4 text-center">
          <div class="mb-2 text-[10px] uppercase tracking-[0.2em] text-gray-500">{{ metric.label }}</div>

          <div class="font-mono text-2xl font-bold" :class="metric.color">
            {{ metric.value }}
            <small class="ml-1 text-xs font-normal text-gray-500">{{ metric.unit }}</small>
          </div>
        </div>
      </div>
    </card-main>
  </div>
</template>
