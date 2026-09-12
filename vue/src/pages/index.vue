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
const activeEnergyDrob = computed(() => (main.value.device?.activeEnergy || 0) % 100);
// const positiveActiveEnergy = computed(() => Math.floor(main.value.device.positiveActiveEnergy / 100));
// const reverseActiveEnergy = computed(() => Math.floor(main.value.device.reverseActiveEnergy / 100));
// const reactiveEnergy = computed(() => Math.floor(main.value.device.reactiveEnergy / 100));
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

    <section>
      <card-main title="DDS6619" class="p-7">
        <div class="relative overflow-hidden rounded-lg px-5 py-6 text-center bg-gray-100 dark:bg-black/20">
          <div class="text-[11px] tracking-[4px] uppercase text-slate-500 mb-3">Показания</div>

          <div id="reading" class="font-mono text-2xl xl:text-5xl font-bold tracking-[4px] leading-none">
            <span class="text-emerald-400 [text-shadow:0_0_20px_rgba(46,204,113,0.7),0_0_40px_rgba(46,204,113,0.3)]">{{ activeEnergy }}</span>
            <span class="text-red-500 [text-shadow:0_0_20px_rgba(231,76,60,0.7)]">.{{ activeEnergyDrob }}</span>
          </div>

          <div class="text-sm tracking-[2px] text-slate-500 mt-3">кВт·ч</div>
        </div>

        <div class="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-4 gap-3.5 mt-5">
          <div class="rounded-lg p-3.5 text-center bg-gray-100 dark:bg-black/20 border border-white/[0.06]">
            <div class="text-[10px] tracking-[2px] uppercase text-slate-400 mb-2">Напряжение</div>

            <div id="voltage" class="font-mono text-2xl font-bold text-blue-400">
              {{ voltage / 10 }}
              <small class="text-xs text-slate-400 ml-0.5">В</small>
            </div>
          </div>

          <div class="rounded-lg p-3.5 text-center bg-gray-100 dark:bg-black/20 border border-white/[0.06]">
            <div class="text-[10px] tracking-[2px] uppercase text-slate-400 mb-2">Ток</div>

            <div id="current" class="font-mono text-2xl font-bold text-purple-400">
              {{ current / 100 }}
              <small class="text-xs text-slate-400 ml-0.5">А</small>
            </div>
          </div>

          <div class="rounded-lg p-3.5 text-center bg-gray-100 dark:bg-black/20 border border-white/[0.06]">
            <div class="text-[10px] tracking-[2px] uppercase text-slate-400 mb-2">Мощность</div>

            <div id="power" class="font-mono text-2xl font-bold text-amber-400">
              {{ power }}
              <small class="text-xs text-slate-400 ml-0.5">Вт</small>
            </div>
          </div>

          <div class="rounded-lg p-3.5 text-center bg-gray-100 dark:bg-black/20 border border-white/[0.06]">
            <div class="text-[10px] tracking-[2px] uppercase text-slate-400 mb-2">Частота</div>

            <div class="font-mono text-2xl font-bold text-teal-400">
              {{ frequency / 100 }}
              <small class="text-xs text-slate-400 ml-0.5">Hz</small>
            </div>
          </div>
        </div>
      </card-main>
    </section>
  </div>
</template>
