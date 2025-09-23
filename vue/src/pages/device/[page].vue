<template>
  <div>
    <div class="mb-4 flex items-center justify-between">
      <h1>{{ title }}</h1>

      <div data-slot="device"></div>
    </div>

    <template v-if="show">
      <ServiceSettings v-if="isPage('settings')" />

      <ServiceStorage v-if="isPage('storage')" />

      <ServiceGPIO v-if="isPage('gpio')" />
    </template>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import { useRoute } from 'vue-router';

import { getPageTitle } from 'vuesp-components/helpers';

import { useConnection } from '@/composables/useConnection.js';

import ServiceGPIO from '@/components/service/ServiceGPIO.vue';
import ServiceStorage from '@/components/service/ServiceStorage.vue';
import ServiceSettings from '@/components/service/ServiceSettings.vue';

const { menu } = useConnection();

const route = useRoute();

const show = ref(false);

const title = computed(() => getPageTitle(menu.value, route.fullPath)?.name);
const page = computed(() => route.params.page);

const isPage = (value: string) => page.value === value;

onMounted(() => {
  setTimeout(() => {
    show.value = true;
  }, 100);
});
</script>
