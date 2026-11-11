<script setup lang="ts">
import { computed, ref } from 'vue';

import { CardMain, VButton } from 'vuesp-components/components';

import EnergyFlowBlock from '@/components/energy/EnergyFlowBlock.vue';
import { useLocale } from '@/composables/useLocale';

const { $t } = useLocale();

type ScenarioKey = 'day' | 'night' | 'export';

const scenario = ref<ScenarioKey>('day');

const scenarioItems: { key: ScenarioKey; label: string }[] = [
  { key: 'day', label: 'День' },
  { key: 'night', label: 'Ночь' },
  { key: 'export', label: 'Экспорт' },
];

const scenarios = {
  day: { solarPower: 2.6, gridPower: 0, batteryPower: 0.8, batterySoc: 78, loadPower: 1.8 },
  night: { solarPower: 0, gridPower: 1.4, batteryPower: -0.6, batterySoc: 62, loadPower: 1.8 },
  export: { solarPower: 4.8, gridPower: -1.6, batteryPower: 1.2, batterySoc: 86, loadPower: 2.0 },
} satisfies Record<ScenarioKey, Record<string, number>>;

const scenarioValues = computed(() => scenarios[scenario.value]);

const resources = computed(() => [
  { href: 'https://docs.vuesp.ru/', icon: 'book', title: $t('homePage.docs'), text: $t('homePage.docsText') },
  { href: 'https://docs.vuesp.ru/vuesp-components-examples', icon: 'grid', title: $t('homePage.components'), text: $t('homePage.componentsText') },
  { href: 'https://docs.vuesp.ru/web-ui', icon: 'code', title: $t('homePage.webUi'), text: $t('homePage.webUiText') },
  { href: 'https://github.com/bondrogeen/vuesp-components', icon: 'github', title: 'vuesp-components', text: $t('homePage.componentsSource') },
]);
</script>

<template>
  <div class="container mx-auto space-y-6 pb-10">
    <div class="mb-6 flex items-center justify-between">
      <h1>{{ $t('home') }}</h1>
    </div>

    <div class="flex flex-wrap items-center gap-2">
      <span class="text-xs text-gray-500 dark:text-gray-400">Сценарий:</span>

      <VButton
        v-for="item of scenarioItems"
        :key="item.key"
        size="sm"
        :variant="scenario === item.key ? 'primary' : 'ghost'"
        @click="scenario = item.key"
      >
        {{ item.label }}
      </VButton>
    </div>

    <EnergyFlowBlock v-bind="scenarioValues" />

    <CardMain :title="$t('homePage.docs')" :loading="false">
      <p class="text-sm leading-relaxed text-gray-500 dark:text-gray-400">{{ $t('homePage.docsIntro') }}</p>
      <div class="mt-4 grid gap-2 sm:grid-cols-2">
        <a
          v-for="resource of resources"
          :key="resource.href"
          :href="resource.href"
          target="_blank"
          rel="noreferrer"
          class="flex items-center gap-3 rounded-md bg-white/5 p-3 transition-colors hover:bg-white/10"
        >
          <icon-ri-book-open-line v-if="resource.icon === 'book'" class="size-4 shrink-0 text-primary" />
          <icon-ri-layout-grid-line v-else-if="resource.icon === 'grid'" class="size-4 shrink-0 text-primary" />
          <icon-ri-code-s-slash-line v-else-if="resource.icon === 'code'" class="size-4 shrink-0 text-primary" />
          <icon-ri-github-fill v-else-if="resource.icon === 'github'" class="size-4 shrink-0 text-primary" />
          <span class="min-w-0 flex-1">
            <span class="block text-xs font-medium">{{ resource.title }}</span>
            <span class="mt-0.5 block text-[10px] leading-snug text-gray-500 dark:text-gray-400">{{ resource.text }}</span>
          </span>
          <icon-ri-external-link-line class="size-3.5 shrink-0 text-gray-400" />
        </a>
      </div>
    </CardMain>
  </div>
</template>
