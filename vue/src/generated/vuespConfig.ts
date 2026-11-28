// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/config/generate.mjs

export const VUESP_CONFIG = {
  "version": 1,
  "project": {
    "name": "vuesp.",
    "board": "esp8266",
    "buildProfile": "standard",
    "firmwareVersion": "5.0.13",
    "deviceName": "vuesp."
  },
  "features": {
    "dashboard": true,
    "discovery": true,
    "modbus": false,
    "script": true,
    "serial": true
  },
  "network": {
    "wifi": {
      "dhcp": true,
      "ip": [
        192,
        168,
        1,
        12
      ],
      "subnet": [
        255,
        255,
        255,
        0
      ],
      "gateway": [
        192,
        168,
        1,
        1
      ],
      "dns": [
        8,
        8,
        8,
        8
      ],
      "ssid": "",
      "password": ""
    },
    "auth": {
      "enabled": true,
      "login": "admin",
      "password": "admin"
    },
    "discovery": {
      "enabled": true,
      "interval": 5,
      "port": 12345
    }
  },
  "logging": {
    "level": 3
  },
  "script": {
    "maxCodeBytes": 512,
    "cacheSlots": 6,
    "maxSavedScripts": 15,
    "maxScripts": 15,
    "maxContexts": 15,
    "stackSize": 64,
    "scratchBytes": 256,
    "maxCallDepth": 4,
    "vars": {
      "u": 10,
      "i": 10,
      "f": 5,
      "s": 2,
      "a": 5,
      "p": 40,
      "e": 4,
      "c": 4
    },
    "maxStringLen": 32,
    "maxArrayLen": 64,
    "maxPwmValue": 255,
    "maxExternalFunctions": 16,
    "maxFunctionParams": 5,
    "maxCallParams": 4,
    "maxEventParams": 4,
    "instrPerTick": 32,
    "usPerTick": 300,
    "updateBudgetUs": 3000,
    "events": [
      {
        "id": 1,
        "name": "btn",
        "args": [
          "gpio",
          "value"
        ],
        "params": 2,
        "description": "button edge: press or release",
        "requires": null
      },
      {
        "id": 2,
        "name": "btn_c",
        "args": [
          "gpio",
          "value",
          "count"
        ],
        "params": 3,
        "description": "button click with click count",
        "requires": null
      },
      {
        "id": 3,
        "name": "btn_l",
        "args": [
          "gpio",
          "value"
        ],
        "params": 2,
        "description": "button long press",
        "requires": null
      },
      {
        "id": 4,
        "name": "btn_r",
        "args": [
          "gpio",
          "value"
        ],
        "params": 2,
        "description": "button hold repeat",
        "requires": null
      },
      {
        "id": 5,
        "name": "modbus",
        "args": [],
        "params": 0,
        "description": "Modbus event",
        "requires": "modbus"
      },
      {
        "id": 6,
        "name": "time",
        "args": [
          "hours",
          "minutes"
        ],
        "params": 2,
        "description": "device uptime clock",
        "requires": null
      }
    ],
    "functions": [
      {
        "id": 1,
        "name": "fade",
        "minArgs": 3,
        "maxArgs": 3,
        "ret": "int",
        "description": "PWM fade",
        "requires": null
      },
      {
        "id": 2,
        "name": "udp",
        "minArgs": 1,
        "maxArgs": 2,
        "ret": "int",
        "description": "UDP send",
        "requires": null
      },
      {
        "id": 3,
        "name": "http",
        "minArgs": 2,
        "maxArgs": 5,
        "ret": "int",
        "description": "HTTP request",
        "requires": null
      },
      {
        "id": 4,
        "name": "var",
        "minArgs": 1,
        "maxArgs": 2,
        "ret": "float",
        "description": "read a Modbus variable",
        "requires": "modbus"
      },
      {
        "id": 5,
        "name": "modbus",
        "minArgs": 2,
        "maxArgs": 3,
        "ret": "float",
        "description": "Modbus operation",
        "requires": "modbus"
      }
    ]
  },
  "web": {
    "pages": {
      "gpio": true,
      "settings": true,
      "storage": true
    },
    "dashboard": {
      "cards": [
        "button",
        "date",
        "dimmer",
        "info",
        "input",
        "list",
        "chart"
      ]
    },
    "locales": [
      "ru",
      "en"
    ],
    "menu": [
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
    ]
  }
} as const;

export const VUESP_FEATURES = VUESP_CONFIG.features;
export const VUESP_PAGES = VUESP_CONFIG.web.pages;
export const VUESP_MENU = VUESP_CONFIG.web.menu;
