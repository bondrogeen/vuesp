<script setup lang="ts">
import { computed } from 'vue';
import { usePreferredReducedMotion } from '@vueuse/core';

import { CardMain, VButton, VisionIcon } from 'vuesp-components/components';

interface IProps {
  title?: string;
  online?: boolean;
  solarPower?: number;
  gridPower?: number;
  batteryPower?: number;
  batterySoc?: number;
  loadPower?: number;
  gridVoltage?: number;
  gridFrequency?: number;
  temperature?: number;
}

const {
  title = 'Сетевой инвертор',
  online = true,
  solarPower = 3.2,
  gridPower = -0.6,
  batteryPower = 0.8,
  batterySoc = 78,
  loadPower = 1.8,
  gridVoltage = 230,
  gridFrequency = 50,
  temperature = 34,
} = defineProps<IProps>();

const emit = defineEmits<{ refresh: [] }>();

const power = (value: number) => `${Math.abs(value).toFixed(1)} кВт`;
const speedFor = (value: number) => Math.min(4.2, Math.max(1.6, 4.2 - Math.abs(value) * 0.55));

const reducedMotion = usePreferredReducedMotion();
const isStatic = computed(() => reducedMotion.value === 'reduce');

const links = computed(() => [
  {
    id: 'solar',
    desktop: 'M24 7 C 36 7 36 43 41 43',
    mobile: 'M24 17 C 24 28 40 28 40 37',
    active: solarPower > 0,
    reverse: false,
    speed: speedFor(solarPower),
  },
  {
    id: 'grid',
    desktop: 'M24 93 C 36 93 36 57 41 57',
    mobile: 'M76 17 C 76 28 60 28 60 37',
    active: gridPower !== 0,
    reverse: gridPower < 0,
    speed: speedFor(gridPower),
  },
  {
    id: 'battery',
    desktop: 'M59 43 C 66 43 72 7 85 7',
    mobile: 'M40 63 C 40 72 24 72 24 83',
    active: batteryPower !== 0,
    reverse: batteryPower > 0,
    speed: speedFor(batteryPower),
  },
  {
    id: 'home',
    desktop: 'M59 57 C 66 57 72 93 85 93',
    mobile: 'M60 63 C 60 72 76 72 76 83',
    active: loadPower > 0,
    reverse: true,
    speed: speedFor(loadPower),
  },
]);

const layouts = [
  { key: 'desktop' as const, cls: 'hidden lg:block' },
  { key: 'mobile' as const, cls: 'lg:hidden' },
];

const activeLinks = computed(() => links.value.filter((link) => link.active));

const nodes = computed(() => [
  {
    key: 'solar',
    wrapper: 'col-start-1 row-start-1 justify-self-start',
    icon: 'sun',
    label: 'Солнечная панель',
    value: power(solarPower),
    tone: 'text-amber-500',
    surface: 'border-amber-500/20 bg-amber-500/10',
    active: solarPower > 0,
  },
  {
    key: 'battery',
    wrapper: 'col-start-1 row-start-3 self-end justify-self-start lg:col-start-3 lg:row-start-1 lg:self-auto lg:justify-self-end',
    icon: 'battery',
    label: 'Батарея',
    value: `${batterySoc} %`,
    tone: 'text-emerald-500',
    surface: 'border-emerald-500/20 bg-emerald-500/10',
    active: batterySoc > 0,
  },
  {
    key: 'grid',
    wrapper: 'col-start-2 row-start-1 justify-self-end lg:col-start-1 lg:row-start-3 lg:self-end lg:justify-self-start',
    icon: 'plug',
    label: 'Сеть',
    value: `${gridVoltage} В`,
    tone: 'text-primary',
    surface: 'border-primary/20 bg-primary/10',
    active: gridVoltage > 0,
  },
  {
    key: 'home',
    wrapper: 'col-start-2 row-start-3 justify-self-end self-end lg:col-start-3 lg:row-start-3',
    icon: 'home',
    label: 'Дом',
    value: power(loadPower),
    tone: 'text-violet-500',
    surface: 'border-violet-500/20 bg-violet-500/10',
    active: loadPower > 0,
  },
]);

const metrics = computed(() => [
  { label: 'Напряжение сети', value: `${gridVoltage} В`, icon: 'bolt' },
  { label: 'Частота', value: `${gridFrequency} Гц`, icon: 'chart' },
  { label: 'Генерация', value: power(solarPower), icon: 'sun' },
  { label: 'Нагрузка', value: power(loadPower), icon: 'home' },
  { label: 'Температура', value: `${temperature} °C`, icon: 'temperature' },
]);

const gridState = computed(() => (gridPower < 0 ? 'Отдаём в сеть' : 'Берём из сети'));
</script>

<template>
  <CardMain :title="title">
    <template #header>
      <div class="flex items-center gap-2">
        <span
          class="inline-flex items-center gap-2 rounded-full border px-2.5 py-1 text-xs font-medium"
          :class="online ? 'border-emerald-500/25 bg-emerald-500/10 text-emerald-600 dark:text-emerald-400' : 'border-gray-300/60 bg-gray-500/10 text-gray-500 dark:border-white/10 dark:text-gray-400'"
        >
          <span class="size-1.5 rounded-full" :class="online ? 'animate-pulse bg-emerald-500' : 'bg-gray-400'" />
          {{ online ? 'Онлайн' : 'Не в сети' }}
        </span>

        <VButton type="icon" size="sm" aria-label="Обновить данные" title="Обновить данные" @click="emit('refresh')">
          <VisionIcon name="refresh" :size="16" />
        </VButton>
      </div>
    </template>

    <div class="grid gap-4 lg:grid-cols-[minmax(0,1fr)_260px]">
      <div class="relative min-h-80 sm:min-h-96">
        <svg class="pointer-events-none absolute inset-0 size-full" viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true">
          <g v-for="layout of layouts" :key="layout.key" :class="layout.cls">
            <path
              v-for="link of links"
              :key="`rail-${layout.key}-${link.id}`"
              class="energy-rail text-gray-400 dark:text-white/20"
              :d="link[layout.key]"
              fill="none"
              vector-effect="non-scaling-stroke"
            />

            <g v-for="link of activeLinks" :key="`flow-${layout.key}-${link.id}`">
              <path class="energy-track" :d="link[layout.key]" fill="none" vector-effect="non-scaling-stroke" />
              <path class="energy-glow" :d="link[layout.key]" fill="none" vector-effect="non-scaling-stroke" />

              <path
                class="energy-line"
                :class="{ 'energy-line--reverse': link.reverse }"
                :d="link[layout.key]"
                :style="{ animationDuration: `${link.speed}s` }"
                fill="none"
                vector-effect="non-scaling-stroke"
              />

              <path
                v-if="!isStatic"
                class="energy-comet"
                :class="{ 'energy-comet--reverse': link.reverse }"
                :d="link[layout.key]"
                :style="{ animationDuration: `${link.speed}s` }"
                fill="none"
                vector-effect="non-scaling-stroke"
              />
            </g>
          </g>
        </svg>

        <div class="relative grid h-full grid-cols-2 grid-rows-3 gap-3 lg:grid-cols-3">
          <div v-for="node of nodes" :key="node.key" :class="node.wrapper">
            <div class="glass-inset flex min-w-0 items-center gap-2 rounded-md px-2.5 py-2">
              <span class="grid size-9 shrink-0 place-items-center rounded-md border" :class="[node.tone, node.surface, { 'opacity-40': !node.active }]">
                <VisionIcon :name="node.icon" :size="18" />
              </span>

              <span class="min-w-0">
                <span class="block truncate text-xs text-gray-500 dark:text-gray-400">{{ node.label }}</span>
                <span class="block text-sm font-semibold">{{ node.value }}</span>
              </span>
            </div>
          </div>

          <div class="col-span-2 row-start-2 grid place-items-center lg:col-span-1 lg:col-start-2">
            <div class="relative flex w-32 flex-col items-center gap-1 rounded-xl border border-primary/30 bg-primary/10 px-4 py-3 text-primary shadow-[0_0_60px_-20px_rgba(40,183,240,0.9)] lg:w-36">
              <span class="absolute -inset-2 rounded-2xl border border-primary/15" />

              <VisionIcon name="inverter" :size="26" />
              <span class="text-xs text-gray-500 dark:text-gray-400">Инвертор</span>
              <span class="text-sm font-semibold text-gray-800 dark:text-white">{{ power(loadPower) }}</span>
            </div>
          </div>
        </div>
      </div>

      <div class="flex flex-col gap-2">
        <dl class="grid grid-cols-2 gap-2">
          <div
            v-for="metric of metrics"
            :key="metric.label"
            class="rounded-md bg-white/5 p-3"
            :class="{ 'col-span-2': metric.label === 'Температура' }"
          >
            <dt class="flex items-center gap-1.5 text-xs text-gray-500 dark:text-gray-400">
              <VisionIcon :name="metric.icon" :size="14" />
              {{ metric.label }}
            </dt>
            <dd class="mt-1 text-sm font-semibold">{{ metric.value }}</dd>
          </div>
        </dl>

        <div class="rounded-md bg-white/5 p-3">
          <div class="flex items-center justify-between gap-2 text-xs">
            <span class="text-gray-500 dark:text-gray-400">Батарея</span>
            <span class="font-semibold">{{ batterySoc }} %</span>
          </div>

          <div class="mt-2 h-1.5 overflow-hidden rounded-full bg-gray-200/70 dark:bg-white/10">
            <div class="h-full rounded-full bg-emerald-500 transition-all duration-500" :style="{ width: `${batterySoc}%` }" />
          </div>

          <p class="mt-2 text-xs text-gray-500 dark:text-gray-400">
            {{ batteryPower >= 0 ? 'Зарядка' : 'Разрядка' }} · {{ power(batteryPower) }}
          </p>
        </div>

        <div class="rounded-md bg-white/5 p-3 text-xs text-gray-500 dark:text-gray-400">
          <p class="font-medium text-gray-600 dark:text-gray-300">{{ gridState }}</p>
          <p class="mt-1">{{ power(gridPower) }} · {{ gridFrequency }} Гц</p>
        </div>
      </div>
    </div>
  </CardMain>
</template>

<style>
@keyframes energy-flow {
  to {
    stroke-dashoffset: -36;
  }
}

@keyframes energy-comet {
  to {
    stroke-dashoffset: -40;
  }
}

.energy-rail {
  stroke: currentColor;
  stroke-width: 1.5;
  stroke-linecap: round;
  stroke-dasharray: 1 6;
  opacity: 0.45;
}

.energy-track {
  stroke: var(--color-primary);
  stroke-width: 2.5;
  stroke-linecap: round;
  opacity: 0.55;
}

.energy-glow {
  stroke: var(--color-primary);
  stroke-width: 10;
  stroke-linecap: round;
  opacity: 0.18;
  filter: blur(6px);
}

.energy-line {
  stroke: #d6f6ff;
  stroke-width: 2;
  stroke-linecap: round;
  stroke-dasharray: 16 22;
  opacity: 0.75;
  filter: drop-shadow(0 0 4px rgba(40, 183, 240, 0.55));
  animation: energy-flow 1.1s linear infinite;
}

.energy-line--reverse {
  animation-direction: reverse;
}

.energy-comet {
  stroke: #d6f6ff;
  stroke-width: 6;
  stroke-linecap: round;
  stroke-dasharray: 0.1 20;
  filter: drop-shadow(0 0 6px rgba(40, 183, 240, 0.85));
  animation: energy-comet 2s linear infinite;
}

.energy-comet--reverse {
  animation-direction: reverse;
}

@media (prefers-reduced-motion: reduce) {
  .energy-line {
    animation: none;
    stroke-dasharray: none;
  }

  .energy-comet {
    animation: none;
  }
}
</style>
