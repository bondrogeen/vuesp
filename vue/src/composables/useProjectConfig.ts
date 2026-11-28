import { VUESP_CONFIG } from '@/generated/vuespConfig';
import { ENABLED_MODULES, VUESP_DASHBOARD_CARDS, VUESP_MENU, VUESP_PAGES } from '@/generated/modules';
import { SCRIPT_LIMITS } from '@/generated/scriptManifest';

// Single entry point for build-time values generated from vuesp.config.yaml.
// Runtime device capabilities remain authoritative where they overlap
// (for example /vm/capabilities.maxScripts).
export const useProjectConfig = () => ({
  config: VUESP_CONFIG,
  project: VUESP_CONFIG.project,
  features: VUESP_CONFIG.features,
  web: VUESP_CONFIG.web,
  script: VUESP_CONFIG.script,
  enabledModules: ENABLED_MODULES,
  menu: VUESP_MENU,
  pages: VUESP_PAGES,
  dashboardCards: VUESP_DASHBOARD_CARDS,
  maxSavedScripts: SCRIPT_LIMITS.maxSavedScripts,
  cacheSlots: SCRIPT_LIMITS.cacheSlots,
});
