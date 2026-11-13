<script setup lang="ts">
import { computed } from 'vue';

import { CardMain, EnergyFlowBlock, deviceBlockStorage, energyFlowDefinition, useBlock } from 'vuesp-components/components';
import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';
import { KEYS } from '@/utils/const';
import { COMMAND } from '@/utils/gpio';

const { $t } = useLocale();

const { main } = useConnection((send) => {
  send(KEYS.INFO);
  send(KEYS.DEVICE);
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
});

const block = useBlock(energyFlowDefinition, main, { storage: deviceBlockStorage });

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

    <EnergyFlowBlock
      v-bind="block.data"
      :config="block.config"
      :context="block.context"
      @update:config="block.update"
    />

    <CardMain :title="$t('homePage.docs')" :loading="false">
      <p class="text-sm leading-relaxed text-gray-dark-25 dark:text-gray-main">{{ $t('homePage.docsIntro') }}</p>
      <div class="mt-4 grid gap-2 sm:grid-cols-2">
        <a
          v-for="resource of resources"
          :key="resource.href"
          :href="resource.href"
          target="_blank"
          rel="noreferrer"
          class="flex items-center gap-3 rounded-md bg-white/5 p-3 transition-colors hover:bg-white/10"
        >
          <icon-ri-book-open-line v-if="resource.icon === 'book'" class="size-4 shrink-0 text-cyan-main" />
          <icon-ri-layout-grid-line v-else-if="resource.icon === 'grid'" class="size-4 shrink-0 text-cyan-main" />
          <icon-ri-code-s-slash-line v-else-if="resource.icon === 'code'" class="size-4 shrink-0 text-cyan-main" />
          <icon-ri-github-fill v-else-if="resource.icon === 'github'" class="size-4 shrink-0 text-cyan-main" />
          <span class="min-w-0 flex-1">
            <span class="block text-xs font-medium">{{ resource.title }}</span>
            <span class="mt-0.5 block text-[10px] leading-snug text-gray-dark-25 dark:text-gray-main">{{ resource.text }}</span>
          </span>
          <icon-ri-external-link-line class="size-3.5 shrink-0 text-gray-main" />
        </a>
      </div>
    </CardMain>
  </div>
</template>
