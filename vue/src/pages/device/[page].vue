<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import { useRoute } from 'vue-router';

import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore';

import { getPageTitle } from 'vuesp-components/helpers';
import { SERVICE_PAGES } from '@/generated/servicePages';

const appStore = useAppStore();
const { menu } = storeToRefs(appStore);

const route = useRoute();

const show = ref(false);

const page = computed(() => String(route.params.page ?? ''));
const servicePage = computed(() => {
  const key = page.value;
  return Object.prototype.hasOwnProperty.call(SERVICE_PAGES, key)
    ? SERVICE_PAGES[key as keyof typeof SERVICE_PAGES]
    : null;
});
const title = computed(() => getPageTitle(menu.value, route.fullPath)?.name ?? 'message.pageNotFound');

onMounted(() => {
  setTimeout(() => {
    show.value = true;
  }, 100);
});
</script>

<template>
  <div>
    <div class="mb-6 flex items-center justify-between">
      <h1>{{ $t(title) }}</h1>

      <div id="device-slot"></div>
    </div>

    <template v-if="show">
      <component :is="servicePage" v-if="servicePage" />

      <p v-else class="text-sm text-gray-dark-25 dark:text-gray-main">{{ $t('message.pageNotFound') }}</p>
    </template>
  </div>
</template>
