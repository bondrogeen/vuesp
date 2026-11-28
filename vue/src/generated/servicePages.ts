// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/config/generate.mjs

import ServiceSettings from '@/components/service/ServiceSettings.vue';
import ServiceStorage from '@/components/service/ServiceStorage.vue';
import ServiceGPIO from '@/components/service/ServiceGPIO.vue';

export const SERVICE_PAGES = {
  settings: ServiceSettings,
  storage: ServiceStorage,
  gpio: ServiceGPIO,
} as const;
