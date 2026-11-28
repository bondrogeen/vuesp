// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/config/generate.mjs

export const ENABLED_MODULES = ["dashboard","discovery","script","serial"] as const;

export const VUESP_MENU = [
  {
    "name": "menu.home",
    "icon": "home",
    "path": "/"
  },
  {
    "name": "menu.dashboard",
    "icon": "dashboard",
    "path": "/dashboard/"
  },
  {
    "name": "menu.script",
    "icon": "script",
    "path": "/script/"
  },
  {
    "name": "menu.settings",
    "icon": "settings",
    "path": "/device/settings/"
  },
  {
    "name": "menu.fs",
    "icon": "storage",
    "path": "/device/storage/"
  },
  {
    "name": "menu.gpio",
    "icon": "gpio",
    "path": "/device/gpio/"
  },
  {
    "name": "menu.serial",
    "icon": "plug",
    "path": "/serial/"
  }
] as const;

export const VUESP_PAGES = {
  "gpio": true,
  "settings": true,
  "storage": true
} as const;

export const VUESP_DASHBOARD_CARDS = ["button","date","dimmer","info","input","list","chart"] as const;
