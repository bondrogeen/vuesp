#!/usr/bin/env node
// Generates firmware and web configuration from vuesp.config.yaml.
//
// Inputs:
//   vuesp.config.yaml
//   platformio.ini (firmware version fallback)
//
// Outputs:
//   src/generated/vuespConfig.h
//   src/generated/scriptManifest.h
//   vue/src/generated/vuespConfig.ts
//   vue/src/generated/modules.ts
//   vue/json/menu.json
//   vuesp.config.schema.json
//
// Usage:
//   node scripts/config/generate.mjs
//   node scripts/config/generate.mjs --check

import { createRequire } from 'node:module';
import { existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const requireFromWeb = createRequire(new URL('../../vue/package.json', import.meta.url));
let YAML;
try {
  YAML = requireFromWeb('yaml');
} catch {
  console.error('[config] missing "yaml" dependency; run: cd vue && npm install');
  process.exit(1);
}

const root = resolve(dirname(fileURLToPath(import.meta.url)), '../..');
const check = process.argv.includes('--check');

const BOARDS = {
  esp8266: { id: 1, label: 'ESP8266' },
  esp32: { id: 2, label: 'ESP32' },
  pico32: { id: 2, label: 'ESP32 Pico32' },
  'esp32-c3-devkitm-1': { id: 2, label: 'ESP32-C3' },
};

const PROFILES = {
  basic: { maxCodeBytes: 256, cacheSlots: 12 },
  standard: { maxCodeBytes: 512, cacheSlots: 6 },
  pro: { maxCodeBytes: 1024, cacheSlots: 3 },
};

const PAGES = ['gpio', 'settings', 'storage'];
const DASHBOARD_CARDS = ['button', 'date', 'dimmer', 'info', 'input', 'list', 'chart'];
const FUNCTION_RETURNS = ['int', 'uint', 'float', 'string', 'array', 'none'];

const SERVICE_PAGE_COMPONENTS = {
  settings: { name: 'ServiceSettings', from: '@/components/service/ServiceSettings.vue' },
  storage: { name: 'ServiceStorage', from: '@/components/service/ServiceStorage.vue' },
  gpio: { name: 'ServiceGPIO', from: '@/components/service/ServiceGPIO.vue' },
  modbus: { name: 'ServiceModbus', from: '@/components/service/ServiceModbus.vue', requires: 'modbus' },
};

const DASHBOARD_COMPONENTS = {
  button: { name: 'CardButton', from: 'vuesp-components/components' },
  date: { name: 'CardDate', from: 'vuesp-components/components' },
  dimmer: { name: 'CardDimmer', from: 'vuesp-components/components' },
  info: { name: 'CardInfo', from: 'vuesp-components/components' },
  input: { name: 'CardInput', from: 'vuesp-components/components' },
  list: { name: 'CardList', from: 'vuesp-components/components' },
  chart: { name: 'CardChart', from: '@/components/dashboard/CardChart.vue', default: true },
};

const PIN_MODES = {
  input: 'PORT_INPUT',
  input_pullup: 'PORT_INPUT_PULLUP',
  output: 'PORT_OUTPUT',
  output_open_drain: 'PORT_OUTPUT_OPEN_DRAIN',
  pwm: 'PORT_PWM',
  onewire: 'PORT_ONEWIRE',
  adc: 'PORT_ADC',
  dac: 'PORT_DAC',
};

const PIN_INTERRUPTS = {
  off: 'GPIO_INTERRUPT_OFF',
  rising: 'GPIO_INTERRUPT_RISING',
  falling: 'GPIO_INTERRUPT_FALLING',
  change: 'GPIO_INTERRUPT_CHANGE',
};
const EVENT_REQUIRES = ['modbus'];
const FUNCTION_REQUIRES = ['modbus'];

const errors = [];
const warnings = [];
const stale = [];

const fail = (message) => errors.push(message);
const warn = (message) => warnings.push(message);

const loadModules = () => {
  const dir = join(root, 'config/modules');
  if (!existsSync(dir)) {
    fail('config/modules: directory is missing');
    return {};
  }
  const result = {};
  for (const file of readdirSync(dir).filter((item) => item.endsWith('.yaml')).sort()) {
    let module;
    try {
      module = YAML.parse(readFileSync(join(dir, file), 'utf8')) ?? {};
    } catch (error) {
      fail(`config/modules/${file}: ${error.message}`);
      continue;
    }
    if (typeof module.id !== 'string' || module.id.length === 0) {
      fail(`config/modules/${file}: missing id`);
      continue;
    }
    if (typeof module.id !== 'string' || module.id.length === 0) {
      fail(`config/modules/${file}: missing id`);
      continue;
    }
    if (!module.label || typeof module.label.ru !== 'string' || typeof module.label.en !== 'string') {
      fail(`config/modules/${file}: label.ru and label.en are required`);
    }
    if (typeof module.category !== 'string' || module.category.length === 0) {
      fail(`config/modules/${file}: category is required`);
    }
    if (module.default !== undefined && typeof module.default !== 'boolean') {
      fail(`config/modules/${file}: default must be boolean`);
    }
    if (module.removable !== undefined && typeof module.removable !== 'boolean') {
      fail(`config/modules/${file}: removable must be boolean`);
    }
    if (module.firmware?.files !== undefined && !Array.isArray(module.firmware.files)) {
      fail(`config/modules/${file}: firmware.files must be an array`);
    }
    if (module.firmware?.srcFilter?.exclude !== undefined) {
      if (!Array.isArray(module.firmware.srcFilter.exclude) || module.firmware.srcFilter.exclude.some((item) => typeof item !== 'string')) {
        fail(`config/modules/${file}: firmware.srcFilter.exclude must be an array of strings`);
      }
    }
    if (module.web?.pages !== undefined && !Array.isArray(module.web.pages)) {
      fail(`config/modules/${file}: web.pages must be an array`);
    }
    if (module.requires?.board !== undefined && !Array.isArray(module.requires.board)) {
      fail(`config/modules/${file}: requires.board must be an array`);
    }
    if (module.requires?.features !== undefined && !Array.isArray(module.requires.features)) {
      fail(`config/modules/${file}: requires.features must be an array`);
    }
    if (module.conflicts?.features !== undefined && !Array.isArray(module.conflicts.features)) {
      fail(`config/modules/${file}: conflicts.features must be an array`);
    }
    if (result[module.id]) fail(`config/modules/${file}: duplicate module id "${module.id}"`);
    result[module.id] = module;
  }
  return result;
};

const readText = (path) => readFileSync(join(root, path), 'utf8');
const writeOrCheck = (path, content) => {
  const absolute = join(root, path);
  if (check) {
    if (!existsSync(absolute) || readFileSync(absolute, 'utf8') !== content) {
      stale.push(path);
    }
    return;
  }
  mkdirSync(dirname(absolute), { recursive: true });
  writeFileSync(absolute, content);
};

const cString = (value) =>
  `"${String(value)
    .replace(/\\/g, '\\\\')
    .replace(/"/g, '\\"')
    .replace(/\n/g, '\\n')
    .replace(/\r/g, '\\r')}"`;

const cArray = (values) => `{${values.join(', ')}}`;

const parseVersion = (value) => {
  const match = /^(\d+)\.(\d+)\.(\d+)$/.exec(String(value));
  if (!match) return null;
  return [Number(match[1]), Number(match[2]), Number(match[3])];
};

const parsePlatformVersion = () => {
  try {
    const text = readText('platformio.ini');
    const match = /firmware_version\s*=\s*'\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}'/.exec(text);
    return match ? [Number(match[1]), Number(match[2]), Number(match[3])] : [5, 0, 13];
  } catch {
    return [5, 0, 13];
  }
};

const parseIpv4 = (value, label) => {
  if (Array.isArray(value)) {
    if (value.length !== 4 || value.some((item) => !Number.isInteger(item) || item < 0 || item > 255)) {
      fail(`${label}: expected four numbers 0..255`);
      return [0, 0, 0, 0];
    }
    return value;
  }
  const parts = String(value ?? '').split('.').map((item) => Number(item));
  if (parts.length !== 4 || parts.some((item) => !Number.isInteger(item) || item < 0 || item > 255)) {
    fail(`${label}: expected IPv4 string like 192.168.1.12`);
    return [0, 0, 0, 0];
  }
  return parts;
};

const integer = (value, fallback, label, min, max) => {
  if (value === undefined || value === null) return fallback;
  if (!Number.isInteger(value)) {
    fail(`${label}: expected integer`);
    return fallback;
  }
  if (value < min || value > max) {
    fail(`${label}: expected ${min}..${max}, got ${value}`);
    return fallback;
  }
  return value;
};

const boolean = (value, fallback, label) => {
  if (value === undefined || value === null) return fallback;
  if (typeof value !== 'boolean') {
    fail(`${label}: expected boolean`);
    return fallback;
  }
  return value;
};

const requiredString = (value, label) => {
  if (typeof value !== 'string' || value.length === 0) {
    fail(`${label}: expected non-empty string`);
    return '';
  }
  return value;
};

const optionalString = (value, fallback, label) => {
  if (value === undefined || value === null) return fallback;
  if (typeof value !== 'string') {
    fail(`${label}: expected string`);
    return fallback;
  }
  return value;
};

const unique = (items, key, label) => {
  const seen = new Set();
  for (const item of items) {
    if (seen.has(item[key])) fail(`${label}: duplicate ${key} "${item[key]}"`);
    seen.add(item[key]);
  }
};

const loadConfig = () => {
  const path = join(root, 'vuesp.config.yaml');
  if (!existsSync(path)) {
    console.error(`[config] missing ${path}`);
    process.exit(1);
  }
  try {
    return YAML.parse(readFileSync(path, 'utf8')) ?? {};
  } catch (error) {
    console.error(`[config] cannot parse vuesp.config.yaml: ${error.message}`);
    process.exit(1);
  }
};

const config = loadConfig();
const MODULES = loadModules();
const FEATURES = Object.keys(MODULES);

// ---------------------------------------------------------------------------
// Normalize and validate
// ---------------------------------------------------------------------------

const rawProject = config.project ?? {};
const version = integer(config.version, 1, 'version', 1, 1);
const platformVersion = parsePlatformVersion();
const firmwareVersion = rawProject.firmwareVersion
  ? parseVersion(rawProject.firmwareVersion) ?? platformVersion
  : platformVersion;
const boardName = rawProject.board ?? 'esp8266';
const board = BOARDS[boardName];
if (!board) {
  fail(`project.board: unknown board "${boardName}", expected ${Object.keys(BOARDS).join(', ')}`);
}
const profileName = rawProject.buildProfile ?? 'standard';
const profile = PROFILES[profileName];
if (!profile) {
  fail(`project.buildProfile: unknown profile "${profileName}", expected ${Object.keys(PROFILES).join(', ')}`);
}

const project = {
  name: optionalString(rawProject.name, 'vuesp.', 'project.name'),
  board: board ? boardName : 'esp8266',
  boardId: board ? board.id : 1,
  buildProfile: profile ? profileName : 'standard',
  firmwareVersion: firmwareVersion.join('.'),
  firmwareMajor: firmwareVersion[0],
  firmwareMinor: firmwareVersion[1],
  firmwarePatch: firmwareVersion[2],
  deviceName: optionalString(rawProject.deviceName, 'vuesp.', 'project.deviceName'),
};

const rawFeatures = config.features ?? {};
const features = {};
for (const key of Object.keys(rawFeatures)) {
  if (!MODULES[key]) fail(`features.${key}: unknown feature`);
}
for (const id of FEATURES) {
  const module = MODULES[id];
  features[id] = boolean(rawFeatures[id], module.default === true, `features.${id}`);
}

for (const id of FEATURES) {
  const module = MODULES[id];
  if (!features[id]) {
    if (module.removable === false) fail(`features.${id}: module is not removable`);
    continue;
  }
  for (const required of module.requires?.features ?? []) {
    if (!MODULES[required]) fail(`config/modules/${id}.yaml: unknown required feature "${required}"`);
    else if (!features[required]) fail(`features.${id}: requires features.${required}`);
  }
  for (const conflict of module.conflicts?.features ?? []) {
    if (!MODULES[conflict]) fail(`config/modules/${id}.yaml: unknown conflict "${conflict}"`);
    else if (features[conflict]) fail(`features.${id}: conflicts with features.${conflict}`);
  }
  const boards = module.requires?.board;
  if (Array.isArray(boards) && !boards.includes(project.board)) {
    fail(`features.${id}: not supported on board "${project.board}"`);
  }
}

const rawNetwork = config.network ?? {};
const rawWifi = rawNetwork.wifi ?? {};
const rawAuth = rawNetwork.auth ?? {};
const rawDiscovery = rawNetwork.discovery ?? {};
const network = {
  wifi: {
    dhcp: boolean(rawWifi.dhcp, true, 'network.wifi.dhcp'),
    ip: parseIpv4(rawWifi.ip ?? '192.168.1.12', 'network.wifi.ip'),
    subnet: parseIpv4(rawWifi.subnet ?? '255.255.255.0', 'network.wifi.subnet'),
    gateway: parseIpv4(rawWifi.gateway ?? '192.168.1.1', 'network.wifi.gateway'),
    dns: parseIpv4(rawWifi.dns ?? '8.8.8.8', 'network.wifi.dns'),
    ssid: optionalString(rawWifi.ssid, '', 'network.wifi.ssid'),
    password: optionalString(rawWifi.password, '', 'network.wifi.password'),
  },
  auth: {
    enabled: boolean(rawAuth.enabled, true, 'network.auth.enabled'),
    login: optionalString(rawAuth.login, 'admin', 'network.auth.login'),
    password: optionalString(rawAuth.password, 'admin', 'network.auth.password'),
  },
  discovery: {
    enabled: features.discovery && boolean(rawDiscovery.enabled, true, 'network.discovery.enabled'),
    interval: integer(rawDiscovery.interval, 5, 'network.discovery.interval', 1, 255),
    port: integer(rawDiscovery.port, 12345, 'network.discovery.port', 1, 65535),
  },
};

const logging = {
  level: integer(config.logging?.level, 3, 'logging.level', 0, 3),
};

const rawPins = Array.isArray(config.hardware?.pins) ? config.hardware.pins : [];
const pins = rawPins.map((pin, index) => {
  const label = `hardware.pins[${index}]`;
  if (!pin || typeof pin !== 'object') {
    fail(`${label}: expected object`);
    return null;
  }
  const gpio = integer(pin.gpio, null, `${label}.gpio`, 0, 48);
  const mode = pin.mode;
  if (!PIN_MODES[mode]) fail(`${label}.mode: expected one of ${Object.keys(PIN_MODES).join(', ')}`);
  const interrupt = pin.interrupt ?? 'off';
  if (!PIN_INTERRUPTS[interrupt]) fail(`${label}.interrupt: expected one of ${Object.keys(PIN_INTERRUPTS).join(', ')}`);
  return {
    id: optionalString(pin.id, `gpio${gpio ?? 0}`, `${label}.id`),
    gpio: gpio ?? 0,
    mode,
    value: pin.value === undefined ? null : integer(pin.value, 0, `${label}.value`, 0, 65535),
    interrupt,
    disabled: boolean(pin.disabled, false, `${label}.disabled`),
    button: boolean(pin.button, false, `${label}.button`),
  };
}).filter(Boolean);
unique(pins, 'gpio', 'hardware.pins');

const rawScript = config.script ?? {};
const maxCodeBytes = integer(rawScript.maxCodeBytes, profile?.maxCodeBytes ?? 512, 'script.maxCodeBytes', 64, 1024);
if (![256, 512, 1024].includes(maxCodeBytes)) {
  fail(`script.maxCodeBytes: expected 256, 512 or 1024, got ${maxCodeBytes}`);
}
const cacheSlots = integer(rawScript.cacheSlots, profile?.cacheSlots ?? 6, 'script.cacheSlots', 1, 15);
if (cacheSlots * maxCodeBytes > 12 * 1024) {
  fail(`script: cacheSlots * maxCodeBytes must be <= 12 KiB (${cacheSlots} * ${maxCodeBytes})`);
}
const rawVars = rawScript.vars ?? {};
const vars = {
  u: integer(rawVars.u, 10, 'script.vars.u', 0, 40),
  i: integer(rawVars.i, 10, 'script.vars.i', 0, 40),
  f: integer(rawVars.f, 5, 'script.vars.f', 0, 40),
  s: integer(rawVars.s, 2, 'script.vars.s', 0, 40),
  a: integer(rawVars.a, 5, 'script.vars.a', 0, 40),
  p: integer(rawVars.p, 40, 'script.vars.p', 1, 64),
  e: integer(rawVars.e, 4, 'script.vars.e', 0, 40),
  c: integer(rawVars.c, 4, 'script.vars.c', 0, 40),
};

const maxEventParams = integer(rawScript.maxEventParams, 4, 'script.maxEventParams', 0, 4);
const scriptLimits = {
  maxCodeBytes,
  cacheSlots,
  maxSavedScripts: integer(rawScript.maxSavedScripts, 15, 'script.maxSavedScripts', 1, 255),
  maxScripts: integer(rawScript.maxScripts, 15, 'script.maxScripts', 1, 255),
  maxContexts: integer(rawScript.maxContexts, 15, 'script.maxContexts', 1, 255),
  stackSize: integer(rawScript.stackSize, 64, 'script.stackSize', 1, 65535),
  scratchBytes: integer(rawScript.scratchBytes, 256, 'script.scratchBytes', 1, 65535),
  maxCallDepth: integer(rawScript.maxCallDepth, 4, 'script.maxCallDepth', 1, 15),
  vars,
  maxStringLen: integer(rawScript.maxStringLen, 32, 'script.maxStringLen', 1, 255),
  maxArrayLen: integer(rawScript.maxArrayLen, 64, 'script.maxArrayLen', 1, 255),
  maxPwmValue: integer(rawScript.maxPwmValue, 255, 'script.maxPwmValue', 1, 65535),
  maxExternalFunctions: integer(rawScript.maxExternalFunctions, 16, 'script.maxExternalFunctions', 1, 255),
  maxFunctionParams: integer(rawScript.maxFunctionParams, 5, 'script.maxFunctionParams', 1, 15),
  maxCallParams: integer(rawScript.maxCallParams, 4, 'script.maxCallParams', 0, 15),
  maxEventParams,
  instrPerTick: integer(rawScript.instrPerTick, 32, 'script.instrPerTick', 1, 65535),
  usPerTick: integer(rawScript.usPerTick, 300, 'script.usPerTick', 1, 65535),
  updateBudgetUs: integer(rawScript.updateBudgetUs, 3000, 'script.updateBudgetUs', 1, 65535),
};

const rawEvents = Array.isArray(rawScript.events) ? rawScript.events : [];
const events = rawEvents.map((event, index) => {
  const label = `script.events[${index}]`;
  if (!event || typeof event !== 'object') {
    fail(`${label}: expected object`);
    return null;
  }
  const id = integer(event.id, null, `${label}.id`, 1, 255);
  const name = requiredString(event.name, `${label}.name`);
  const args = Array.isArray(event.args)
    ? event.args.map((arg) => requiredString(arg, `${label}.args`))
    : [];
  if (args.length > maxEventParams) {
    fail(`${label}.args: ${args.length} arguments exceed maxEventParams=${maxEventParams}`);
  }
  if (event.requires !== undefined && !EVENT_REQUIRES.includes(event.requires)) {
    fail(`${label}.requires: expected one of ${EVENT_REQUIRES.join(', ')}`);
  }
  return {
    id: id ?? 0,
    name,
    args,
    params: args.length,
    description: optionalString(event.description, '', `${label}.description`),
    requires: event.requires ?? null,
  };
}).filter(Boolean);
unique(events, 'id', 'script.events');
unique(events, 'name', 'script.events');

const rawFunctions = Array.isArray(rawScript.functions) ? rawScript.functions : [];
const functions = rawFunctions.map((fn, index) => {
  const label = `script.functions[${index}]`;
  if (!fn || typeof fn !== 'object') {
    fail(`${label}: expected object`);
    return null;
  }
  const id = integer(fn.id, null, `${label}.id`, 1, 255);
  const name = requiredString(fn.name, `${label}.name`);
  const minArgs = integer(fn.minArgs, 0, `${label}.minArgs`, 0, scriptLimits.maxFunctionParams);
  const maxArgs = integer(fn.maxArgs, minArgs, `${label}.maxArgs`, minArgs, scriptLimits.maxFunctionParams);
  const ret = fn.ret ?? 'int';
  if (!FUNCTION_RETURNS.includes(ret)) {
    fail(`${label}.ret: expected one of ${FUNCTION_RETURNS.join(', ')}`);
  }
  if (fn.requires !== undefined && !FUNCTION_REQUIRES.includes(fn.requires)) {
    fail(`${label}.requires: expected one of ${FUNCTION_REQUIRES.join(', ')}`);
  }
  return {
    id: id ?? 0,
    name,
    minArgs,
    maxArgs,
    ret,
    description: optionalString(fn.description, '', `${label}.description`),
    requires: fn.requires ?? null,
  };
}).filter(Boolean);
unique(functions, 'id', 'script.functions');
unique(functions, 'name', 'script.functions');

const rawWeb = config.web ?? {};
const rawPages = rawWeb.pages ?? {};
for (const key of Object.keys(rawPages)) {
  if (!PAGES.includes(key)) fail(`web.pages.${key}: unknown page`);
}
const pages = {};
for (const key of PAGES) {
  pages[key] = boolean(rawPages[key], true, `web.pages.${key}`);
}

const dashboardCards = Array.isArray(rawWeb.dashboard?.cards) ? rawWeb.dashboard.cards : [...DASHBOARD_CARDS];
for (const card of dashboardCards) {
  if (!DASHBOARD_CARDS.includes(card)) fail(`web.dashboard.cards: unknown card "${card}"`);
}
if (new Set(dashboardCards).size !== dashboardCards.length) {
  fail('web.dashboard.cards: duplicate entries');
}

const locales = Array.isArray(rawWeb.locales) ? rawWeb.locales : ['ru', 'en'];
for (const locale of locales) {
  if (typeof locale !== 'string') fail('web.locales: expected string');
  else if (!existsSync(join(root, 'vue/json/locales', `${locale}.json`))) {
    fail(`web.locales: missing vue/json/locales/${locale}.json`);
  }
}

const menuAuto = boolean(rawWeb.menu?.auto, true, 'web.menu.auto');
const menuExtra = Array.isArray(rawWeb.menu?.extra) ? rawWeb.menu.extra : [];
const extraMenu = menuExtra.map((item, index) => {
  const label = `web.menu.extra[${index}]`;
  if (!item || typeof item !== 'object') {
    fail(`${label}: expected object`);
    return null;
  }
  const path = requiredString(item.path, `${label}.path`);
  if (!path.startsWith('/')) fail(`${label}.path: expected path starting with /`);
  return {
    name: requiredString(item.name, `${label}.name`),
    icon: optionalString(item.icon, 'home', `${label}.icon`),
    path,
  };
}).filter(Boolean);

const menu = [];
const addMenu = (item) => {
  if (menu.some((entry) => entry.path === item.path)) {
    fail(`web.menu: duplicate path "${item.path}"`);
    return;
  }
  menu.push(item);
};
if (menuAuto) {
  addMenu({ name: 'menu.home', icon: 'home', path: '/' });
  if (features.dashboard) addMenu({ name: 'menu.dashboard', icon: 'dashboard', path: '/dashboard/' });
  if (features.script) addMenu({ name: 'menu.script', icon: 'script', path: '/script/' });
  if (pages.settings) addMenu({ name: 'menu.settings', icon: 'settings', path: '/device/settings/' });
  if (pages.storage) addMenu({ name: 'menu.fs', icon: 'storage', path: '/device/storage/' });
  if (pages.gpio) addMenu({ name: 'menu.gpio', icon: 'gpio', path: '/device/gpio/' });
  if (features.serial) addMenu({ name: 'menu.serial', icon: 'plug', path: '/serial/' });
}
for (const item of extraMenu) addMenu(item);

const web = {
  pages,
  dashboard: { cards: dashboardCards },
  locales,
  menu: { auto: menuAuto, items: menu },
};

const enabledServicePages = Object.entries(SERVICE_PAGE_COMPONENTS).filter(([id, meta]) => {
  if (meta.requires === 'modbus') return features.modbus;
  return pages[id] !== false;
});

const enabledDashboardCards = dashboardCards
  .filter((id) => DASHBOARD_COMPONENTS[id])
  .map((id) => [id, DASHBOARD_COMPONENTS[id]]);

for (const message of warnings) console.warn(`[config] warning: ${message}`);
if (errors.length > 0) {
  for (const message of errors) console.error(`[config] error: ${message}`);
  process.exit(1);
}

// ---------------------------------------------------------------------------
// Renderers
// ---------------------------------------------------------------------------

const renderConfigHeader = () => `#ifndef VUESP_GENERATED_CONFIG_H
#define VUESP_GENERATED_CONFIG_H

// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/config/generate.mjs

#define VUESP_CONFIG_VERSION ${version}
#define VUESP_PROJECT_NAME ${cString(project.name)}
#define VUESP_BOARD ${project.boardId}
#define VUESP_FIRMWARE_MAJOR ${project.firmwareMajor}
#define VUESP_FIRMWARE_MINOR ${project.firmwareMinor}
#define VUESP_FIRMWARE_PATCH ${project.firmwarePatch}

// Features. Build flags may override these values.
#ifndef VUESP_SCRIPT
#define VUESP_SCRIPT ${features.script ? 1 : 0}
#endif
#ifndef VUESP_MODBUS
#define VUESP_MODBUS ${features.modbus ? 1 : 0}
#endif
#ifndef VUESP_SERIAL
#define VUESP_SERIAL ${features.serial ? 1 : 0}
#endif
#ifndef VUESP_DISCOVERY
#define VUESP_DISCOVERY ${features.discovery ? 1 : 0}
#endif
#ifndef VUESP_DASHBOARD
#define VUESP_DASHBOARD ${features.dashboard ? 1 : 0}
#endif

// Logging and network defaults.
#ifndef VUESP_LOG_LEVEL
#define VUESP_LOG_LEVEL ${logging.level}
#endif
#ifndef DEF_WIFI_DHCP
#define DEF_WIFI_DHCP ${network.wifi.dhcp ? 1 : 0}
#endif
#ifndef DEF_WIFI_IP
#define DEF_WIFI_IP ${cArray(network.wifi.ip)}
#endif
#ifndef DEF_WIFI_SUBNET
#define DEF_WIFI_SUBNET ${cArray(network.wifi.subnet)}
#endif
#ifndef DEF_WIFI_GETEWAY
#define DEF_WIFI_GETEWAY ${cArray(network.wifi.gateway)}
#endif
#ifndef DEF_WIFI_DNS
#define DEF_WIFI_DNS ${cArray(network.wifi.dns)}
#endif
#ifndef DEF_WIFI_SSID
#define DEF_WIFI_SSID ${cString(network.wifi.ssid)}
#endif
#ifndef DEF_WIFI_PASS
#define DEF_WIFI_PASS ${cString(network.wifi.password)}
#endif
#ifndef DEF_AUTH_MODE
#define DEF_AUTH_MODE ${network.auth.enabled ? 1 : 0}
#endif
#ifndef DEF_AUTH_LOGIN
#define DEF_AUTH_LOGIN ${cString(network.auth.login)}
#endif
#ifndef DEF_AUTH_PASS
#define DEF_AUTH_PASS ${cString(network.auth.password)}
#endif
#ifndef DEF_DISCOVERY
#define DEF_DISCOVERY ${network.discovery.enabled ? 1 : 0}
#endif
#ifndef DEF_DISCOVERY_INTERVAL
#define DEF_DISCOVERY_INTERVAL ${network.discovery.interval}
#endif
#ifndef DEF_DISCOVERY_PORT
#define DEF_DISCOVERY_PORT ${network.discovery.port}
#endif
#ifndef DEF_DEVICE_NAME
#define DEF_DEVICE_NAME ${cString(project.deviceName)}
#endif
#ifndef DEF_DEVICE_FIRMWARE
#define DEF_DEVICE_FIRMWARE ${cArray([project.firmwareMajor, project.firmwareMinor, project.firmwarePatch])}
#endif
#ifndef DEF_BOARD
#define DEF_BOARD ${project.boardId}
#endif

// Script engine limits.
#ifndef VBC_MAX_CODE_BYTES
#define VBC_MAX_CODE_BYTES ${scriptLimits.maxCodeBytes}
#endif
#ifndef VBC_CACHE_SLOTS
#define VBC_CACHE_SLOTS ${scriptLimits.cacheSlots}
#endif
#ifndef VBC_MAX_SCRIPTS
#define VBC_MAX_SCRIPTS ${scriptLimits.maxScripts}
#endif
#ifndef VBC_MAX_SAVED_SCRIPTS
#define VBC_MAX_SAVED_SCRIPTS ${scriptLimits.maxSavedScripts}
#endif
#ifndef VBC_MAX_CONTEXTS
#define VBC_MAX_CONTEXTS ${scriptLimits.maxContexts}
#endif
#ifndef VBC_STACK_SIZE
#define VBC_STACK_SIZE ${scriptLimits.stackSize}
#endif
#ifndef VBC_SCRATCH_BYTES
#define VBC_SCRATCH_BYTES ${scriptLimits.scratchBytes}
#endif
#ifndef VBC_MAX_CALL_DEPTH
#define VBC_MAX_CALL_DEPTH ${scriptLimits.maxCallDepth}
#endif
#ifndef MAX_UINT_VARS
#define MAX_UINT_VARS ${scriptLimits.vars.u}
#endif
#ifndef MAX_INT_VARS
#define MAX_INT_VARS ${scriptLimits.vars.i}
#endif
#ifndef MAX_FLOAT_VARS
#define MAX_FLOAT_VARS ${scriptLimits.vars.f}
#endif
#ifndef MAX_STRING_VARS
#define MAX_STRING_VARS ${scriptLimits.vars.s}
#endif
#ifndef MAX_ARRAY_VARS
#define MAX_ARRAY_VARS ${scriptLimits.vars.a}
#endif
#ifndef MAX_STRING_LEN
#define MAX_STRING_LEN ${scriptLimits.maxStringLen}
#endif
#ifndef MAX_ARRAY_SIZE
#define MAX_ARRAY_SIZE ${scriptLimits.maxArrayLen}
#endif
#ifndef MAX_PWM_VALUE
#define MAX_PWM_VALUE ${scriptLimits.maxPwmValue}
#endif
#ifndef MAX_EXTERNAL_FUNCTIONS
#define MAX_EXTERNAL_FUNCTIONS ${scriptLimits.maxExternalFunctions}
#endif
#ifndef MAX_FUNCTION_PARAMS
#define MAX_FUNCTION_PARAMS ${scriptLimits.maxFunctionParams}
#endif
#ifndef MAX_CALL_PARAMS
#define MAX_CALL_PARAMS ${scriptLimits.maxCallParams}
#endif
#ifndef MAX_EVENT_PARAMS
#define MAX_EVENT_PARAMS ${scriptLimits.maxEventParams}
#endif

#endif
`;

const renderScriptManifest = () => {
  const lines = [];
  lines.push('#ifndef VUESP_SCRIPT_MANIFEST_H');
  lines.push('#define VUESP_SCRIPT_MANIFEST_H');
  lines.push('');
  lines.push('#include <stdint.h>');
  lines.push('');
  lines.push('#include "../const/features.h"');
  lines.push('#include "../runner/bytecode/BytecodeCapabilities.h"');
  lines.push('');
  lines.push('// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.');
  lines.push('// Run: node scripts/config/generate.mjs');
  lines.push('');
  lines.push('struct ScriptLimits {');
  lines.push('  uint8_t maxSavedScripts;');
  lines.push('  uint8_t cacheSlots;');
  lines.push('  uint8_t maxScripts;');
  lines.push('  uint8_t maxContexts;');
  lines.push('  uint16_t maxCodeBytes;');
  lines.push('  uint16_t stackSize;');
  lines.push('  uint16_t scratchBytes;');
  lines.push('  uint8_t maxStringLen;');
  lines.push('  uint8_t maxArrayLen;');
  lines.push('  uint8_t vars[8];  // u, i, f, s, a, p, e, c');
  lines.push('  uint8_t maxEventParams;');
  lines.push('  uint16_t instrPerTick;');
  lines.push('  uint16_t usPerTick;');
  lines.push('  uint16_t updateBudgetUs;');
  lines.push('};');
  lines.push('');
  lines.push('static const ScriptLimits kScriptLimits = {');
  lines.push(`    VBC_MAX_SAVED_SCRIPTS,`);
  lines.push(`    VBC_CACHE_SLOTS,`);
  lines.push(`    VBC_MAX_SCRIPTS,`);
  lines.push(`    VBC_MAX_CONTEXTS,`);
  lines.push(`    VBC_MAX_CODE_BYTES,`);
  lines.push(`    VBC_STACK_SIZE,`);
  lines.push(`    VBC_SCRATCH_BYTES,`);
  lines.push(`    MAX_STRING_LEN,`);
  lines.push(`    MAX_ARRAY_SIZE,`);
  lines.push(`    {MAX_UINT_VARS, MAX_INT_VARS, MAX_FLOAT_VARS, MAX_STRING_VARS, MAX_ARRAY_VARS,`);
  lines.push(`     ${scriptLimits.vars.p}, ${scriptLimits.vars.e}, ${scriptLimits.vars.c}},`);
  lines.push(`    MAX_EVENT_PARAMS,`);
  lines.push(`    ${scriptLimits.instrPerTick},`);
  lines.push(`    ${scriptLimits.usPerTick},`);
  lines.push(`    ${scriptLimits.updateBudgetUs},`);
  lines.push('};');
  lines.push('');
  lines.push('static const vuesp::bytecode::EventSpec kScriptEvents[] = {');
  for (const event of events) {
    if (event.requires === 'modbus') lines.push('#if VUESP_MODBUS');
    lines.push(
      `    {${event.id}, ${cString(event.name)}, ${event.params}, ${cString(event.args.join(','))}, ${cString(event.description)}},`,
    );
    if (event.requires === 'modbus') lines.push('#endif');
  }
  lines.push('};');
  lines.push('');
  lines.push('static const vuesp::bytecode::FunctionSpec kScriptFunctions[] = {');
  const retMap = {
    int: 'vuesp::bytecode::VALUE_INT',
    uint: 'vuesp::bytecode::VALUE_UINT',
    float: 'vuesp::bytecode::VALUE_FLOAT',
    string: 'vuesp::bytecode::VALUE_STRING',
    array: 'vuesp::bytecode::VALUE_ARRAY',
    none: 'vuesp::bytecode::VALUE_NONE',
  };
  for (const fn of functions) {
    if (fn.requires === 'modbus') lines.push('#if VUESP_MODBUS');
    lines.push(
      `    {${fn.id}, ${cString(fn.name)}, ${fn.minArgs}, ${fn.maxArgs}, ${retMap[fn.ret]}, ${cString(fn.description)}},`,
    );
    if (fn.requires === 'modbus') lines.push('#endif');
  }
  lines.push('};');
  lines.push('');
  lines.push('static const uint8_t kScriptEventCount =');
  lines.push('    (uint8_t)(sizeof(kScriptEvents) / sizeof(kScriptEvents[0]));');
  lines.push('static const uint8_t kScriptFunctionCount =');
  lines.push('    (uint8_t)(sizeof(kScriptFunctions) / sizeof(kScriptFunctions[0]));');
  lines.push('');
  lines.push('#endif');
  lines.push('');
  return lines.join('\n');
};

const configSnapshot = {
  version,
  project: {
    name: project.name,
    board: project.board,
    buildProfile: project.buildProfile,
    firmwareVersion: project.firmwareVersion,
    deviceName: project.deviceName,
  },
  features,
  network: {
    wifi: network.wifi,
    auth: network.auth,
    discovery: network.discovery,
  },
  logging,
  script: {
    maxCodeBytes: scriptLimits.maxCodeBytes,
    cacheSlots: scriptLimits.cacheSlots,
    maxSavedScripts: scriptLimits.maxSavedScripts,
    maxScripts: scriptLimits.maxScripts,
    maxContexts: scriptLimits.maxContexts,
    stackSize: scriptLimits.stackSize,
    scratchBytes: scriptLimits.scratchBytes,
    maxCallDepth: scriptLimits.maxCallDepth,
    vars: scriptLimits.vars,
    maxStringLen: scriptLimits.maxStringLen,
    maxArrayLen: scriptLimits.maxArrayLen,
    maxPwmValue: scriptLimits.maxPwmValue,
    maxExternalFunctions: scriptLimits.maxExternalFunctions,
    maxFunctionParams: scriptLimits.maxFunctionParams,
    maxCallParams: scriptLimits.maxCallParams,
    maxEventParams: scriptLimits.maxEventParams,
    instrPerTick: scriptLimits.instrPerTick,
    usPerTick: scriptLimits.usPerTick,
    updateBudgetUs: scriptLimits.updateBudgetUs,
    events,
    functions,
  },
  web: {
    pages,
    dashboard: web.dashboard,
    locales,
    menu,
  },
};

const renderTsConfig = () => `// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/config/generate.mjs

export const VUESP_CONFIG = ${JSON.stringify(configSnapshot, null, 2)} as const;

export const VUESP_FEATURES = VUESP_CONFIG.features;
export const VUESP_PAGES = VUESP_CONFIG.web.pages;
export const VUESP_MENU = VUESP_CONFIG.web.menu;
`;

const renderModulesTs = () => {
  const enabled = FEATURES.filter((key) => features[key]);
  return `// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/config/generate.mjs

export const ENABLED_MODULES = ${JSON.stringify(enabled)} as const;

export const VUESP_MENU = ${JSON.stringify(menu, null, 2)} as const;

export const VUESP_PAGES = ${JSON.stringify(pages, null, 2)} as const;

export const VUESP_DASHBOARD_CARDS = ${JSON.stringify(dashboardCards)} as const;
`;
};

const renderServicePages = () => {
  const lines = [
    '// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.',
    '// Run: node scripts/config/generate.mjs',
    '',
  ];
  for (const [, meta] of enabledServicePages) {
    lines.push(`import ${meta.name} from '${meta.from}';`);
  }
  if (enabledServicePages.length > 0) lines.push('');
  lines.push('export const SERVICE_PAGES = {');
  for (const [id, meta] of enabledServicePages) {
    lines.push(`  ${id}: ${meta.name},`);
  }
  lines.push('} as const;');
  return `${lines.join('\n')}\n`;
};

const renderDashboardCards = () => {
  const lines = [
    '// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.',
    '// Run: node scripts/config/generate.mjs',
    '',
  ];
  const builtins = enabledDashboardCards.filter(([, meta]) => meta.from === 'vuesp-components/components');
  const locals = enabledDashboardCards.filter(([, meta]) => meta.default);
  if (builtins.length > 0) {
    lines.push(`import { ${builtins.map(([, meta]) => meta.name).join(', ')} } from 'vuesp-components/components';`);
  }
  for (const [, meta] of locals) {
    lines.push(`import ${meta.name} from '${meta.from}';`);
  }
  if (builtins.length > 0 || locals.length > 0) lines.push('');
  lines.push('export const DASHBOARD_CARDS = {');
  for (const [id, meta] of enabledDashboardCards) {
    lines.push(`  ${id}: ${meta.name},`);
  }
  lines.push('} as const;');
  lines.push('');
  lines.push(`export const DASHBOARD_CARD_TYPES = ${JSON.stringify(enabledDashboardCards.map(([id]) => id))} as const;`);
  return `${lines.join('\n')}\n`;
};

const disabledPages = [];
if (!features.script) disabledPages.push('src/pages/script/**');
if (!features.serial) disabledPages.push('src/pages/serial/**');
if (!features.dashboard) disabledPages.push('src/pages/dashboard/**');
const renderPagesJson = () => `${JSON.stringify({ disabled: disabledPages }, null, 2)}\n`;

const droppedLocaleSections = [];
if (!features.script) droppedLocaleSections.push('script', 'scriptStudio');
if (!features.serial) droppedLocaleSections.push('serial');
if (!features.modbus) droppedLocaleSections.push('modbus');
const renderLocaleSections = () => `${JSON.stringify({ drop: droppedLocaleSections }, null, 2)}\n`;

const disabledSrcExcludes = [];
for (const id of FEATURES) {
  if (features[id]) continue;
  const patterns = MODULES[id]?.firmware?.srcFilter?.exclude ?? [];
  for (const pattern of patterns) disabledSrcExcludes.push(pattern);
}
const renderGpioConfig = () => {
  const lines = [
    '#ifndef VUESP_GENERATED_GPIO_CONFIG_H',
    '#define VUESP_GENERATED_GPIO_CONFIG_H',
    '',
    '// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.',
    '// Included by src/gpio/gpio.cpp after gpio.h.',
    '',
    'struct PinOverride {',
    '  uint8_t gpio;',
    '  uint8_t mode;',
    '  uint8_t interrupt;',
    '  uint8_t disabled;',
    '  uint8_t isButton;',
    '  uint8_t hasValue;',
    '  uint16_t value;',
    '};',
    '',
  ];
  if (pins.length > 0) {
    lines.push('static const PinOverride kPinOverrides[] = {');
    for (const pin of pins) {
      lines.push(
        `    {${pin.gpio}, ${PIN_MODES[pin.mode]}, ${PIN_INTERRUPTS[pin.interrupt]}, ` +
          `${pin.disabled ? 'GPIO_DISABLED_ON' : 'GPIO_DISABLED_OFF'}, ` +
          `${pin.button ? 'GPIO_EVENT_ON' : 'GPIO_EVENT_OFF'}, ` +
          `${pin.value === null ? 0 : 1}, ${pin.value ?? 0}},`,
      );
    }
    lines.push('};');
    lines.push('static const uint8_t kPinOverrideCount = (uint8_t)(sizeof(kPinOverrides) / sizeof(kPinOverrides[0]));');
  } else {
    lines.push('static const PinOverride kPinOverrides[1] = {{0, 0, 0, 0, 0, 0, 0}};');
    lines.push('static const uint8_t kPinOverrideCount = 0;');
  }
  lines.push('', '#endif', '');
  return lines.join('\n');
};

const renderPlatformioIni = () => {
  const filters = ['+<*>', '-<tests/*>', ...disabledSrcExcludes.map((pattern) => `-<${pattern}>`)];
  return `; AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.\n; Run: node scripts/config/generate.mjs\n\n[common]\nfirmware_version = '{${project.firmwareMajor}, ${project.firmwareMinor}, ${project.firmwarePatch}}'\ndevice_name = '${JSON.stringify(project.deviceName)}'\n\n[platformio]\ndefault_envs = ${project.board}\n\n[env]\nbuild_src_filter = ${filters.join(' ')}\n`;
};

const renderSchema = () => {
  const localeNames = existsSync(join(root, 'vue/json/locales'))
    ? readdirSync(join(root, 'vue/json/locales'))
        .filter((file) => file.endsWith('.json'))
        .map((file) => file.slice(0, -5))
    : ['ru', 'en'];

  const featureProperties = Object.fromEntries(
    FEATURES.map((id) => {
      const module = MODULES[id];
      return [
        id,
        {
          type: 'boolean',
          default: module.default === true,
          title: module.label?.ru ?? id,
          description: module.description?.ru ?? '',
          'x-vuesp': {
            category: module.category,
            removable: module.removable !== false,
            requires: module.requires ?? {},
            conflicts: module.conflicts ?? {},
          },
        },
      ];
    }),
  );

  const pageProperties = Object.fromEntries(
    PAGES.map((id) => [id, { type: 'boolean', default: true, title: id }]),
  );

  const schema = {
    $schema: 'http://json-schema.org/draft-07/schema#',
    title: 'Vuesp project configuration',
    description: 'Единый файл настроек проекта Vuesp.',
    type: 'object',
    required: ['version'],
    properties: {
      version: { const: 1, description: 'Версия формата конфигурации.' },
      project: {
        type: 'object',
        title: 'Проект',
        properties: {
          name: { type: 'string', default: 'vuesp.' },
          board: {
            enum: Object.keys(BOARDS),
            default: 'esp8266',
            enumDescriptions: Object.values(BOARDS).map((item) => item.label),
            description: 'Выбирает default_envs PlatformIO; pio run -e <env> переопределяет.',
          },
          buildProfile: {
            enum: Object.keys(PROFILES),
            default: 'standard',
            enumDescriptions: ['256 байт × 12 слотов', '512 байт × 6 слотов', '1024 байта × 3 слота'],
          },
          firmwareVersion: { type: 'string', pattern: '^\\d+\\.\\d+\\.\\d+$' },
          deviceName: { type: 'string' },
        },
        additionalProperties: true,
      },
      features: {
        type: 'object',
        title: 'Модули',
        description: 'Включение модулей прошивки и веб-части.',
        properties: featureProperties,
        additionalProperties: false,
      },
      hardware: {
        type: 'object',
        title: 'Железо',
        properties: {
          pins: {
            type: 'array',
            items: {
              type: 'object',
              properties: {
                id: { type: 'string' },
                gpio: { type: 'integer', minimum: 0, maximum: 48 },
                mode: {
                  enum: ['input', 'input_pullup', 'output', 'output_open_drain', 'pwm', 'onewire', 'adc', 'dac'],
                },
                value: { type: 'integer', minimum: 0, maximum: 65535 },
                interrupt: { enum: ['off', 'rising', 'falling', 'change'] },
                disabled: { type: 'boolean', default: false },
                button: { type: 'boolean', default: false },
              },
              required: ['id', 'gpio', 'mode'],
              additionalProperties: true,
            },
          },
          buses: { type: 'object', additionalProperties: true },
        },
        additionalProperties: true,
      },
      network: {
        type: 'object',
        title: 'Сеть',
        properties: {
          wifi: {
            type: 'object',
            properties: {
              dhcp: { type: 'boolean', default: true },
              ip: { type: 'string', default: '192.168.1.12' },
              subnet: { type: 'string', default: '255.255.255.0' },
              gateway: { type: 'string', default: '192.168.1.1' },
              dns: { type: 'string', default: '8.8.8.8' },
              ssid: { type: 'string', default: '' },
              password: { type: 'string', default: '' },
            },
            additionalProperties: true,
          },
          auth: {
            type: 'object',
            properties: {
              enabled: { type: 'boolean', default: true },
              login: { type: 'string', default: 'admin' },
              password: { type: 'string', default: 'admin' },
            },
            additionalProperties: true,
          },
          discovery: {
            type: 'object',
            properties: {
              enabled: { type: 'boolean', default: true },
              interval: { type: 'integer', minimum: 1, maximum: 255, default: 5 },
              port: { type: 'integer', minimum: 1, maximum: 65535, default: 12345 },
            },
            additionalProperties: true,
          },
        },
        additionalProperties: true,
      },
      logging: {
        type: 'object',
        title: 'Логи',
        properties: {
          level: {
            enum: [0, 1, 2, 3],
            default: 3,
            enumDescriptions: ['0 — тихо', '1 — ошибки и boot', '2 — события и жизненный цикл', '3 — подробно'],
          },
        },
        additionalProperties: true,
      },
      script: {
        type: 'object',
        title: 'ScriptRunner',
        properties: {
          maxCodeBytes: { enum: [256, 512, 1024], default: 512 },
          cacheSlots: { type: 'integer', minimum: 1, maximum: 15, default: 6 },
          maxSavedScripts: { type: 'integer', minimum: 1, maximum: 255, default: 15 },
          maxScripts: { type: 'integer', minimum: 1, maximum: 255, default: 15 },
          maxContexts: { type: 'integer', minimum: 1, maximum: 255, default: 15 },
          stackSize: { type: 'integer', minimum: 1, maximum: 65535, default: 64 },
          scratchBytes: { type: 'integer', minimum: 1, maximum: 65535, default: 256 },
          maxCallDepth: { type: 'integer', minimum: 1, maximum: 15, default: 4 },
          vars: {
            type: 'object',
            properties: {
              u: { type: 'integer', minimum: 0, maximum: 40, default: 10 },
              i: { type: 'integer', minimum: 0, maximum: 40, default: 10 },
              f: { type: 'integer', minimum: 0, maximum: 40, default: 5 },
              s: { type: 'integer', minimum: 0, maximum: 40, default: 2 },
              a: { type: 'integer', minimum: 0, maximum: 40, default: 5 },
              p: { type: 'integer', minimum: 1, maximum: 64, default: 40 },
              e: { type: 'integer', minimum: 0, maximum: 40, default: 4 },
              c: { type: 'integer', minimum: 0, maximum: 40, default: 4 },
            },
            additionalProperties: false,
          },
          maxStringLen: { type: 'integer', minimum: 1, maximum: 255, default: 32 },
          maxArrayLen: { type: 'integer', minimum: 1, maximum: 255, default: 64 },
          maxPwmValue: { type: 'integer', minimum: 1, maximum: 65535, default: 255 },
          maxExternalFunctions: { type: 'integer', minimum: 1, maximum: 255, default: 16 },
          maxFunctionParams: { type: 'integer', minimum: 1, maximum: 15, default: 5 },
          maxCallParams: { type: 'integer', minimum: 0, maximum: 15, default: 4 },
          maxEventParams: { type: 'integer', minimum: 0, maximum: 4, default: 4 },
          instrPerTick: { type: 'integer', minimum: 1, maximum: 65535, default: 32 },
          usPerTick: { type: 'integer', minimum: 1, maximum: 65535, default: 300 },
          updateBudgetUs: { type: 'integer', minimum: 1, maximum: 65535, default: 3000 },
          events: {
            type: 'array',
            items: {
              type: 'object',
              properties: {
                id: { type: 'integer', minimum: 1, maximum: 255 },
                name: { type: 'string' },
                args: { type: 'array', items: { type: 'string' } },
                description: { type: 'string' },
                requires: { enum: EVENT_REQUIRES },
              },
              required: ['id', 'name'],
              additionalProperties: true,
            },
          },
          functions: {
            type: 'array',
            items: {
              type: 'object',
              properties: {
                id: { type: 'integer', minimum: 1, maximum: 255 },
                name: { type: 'string' },
                minArgs: { type: 'integer', minimum: 0, maximum: 15 },
                maxArgs: { type: 'integer', minimum: 0, maximum: 15 },
                ret: { enum: FUNCTION_RETURNS },
                description: { type: 'string' },
                requires: { enum: FUNCTION_REQUIRES },
              },
              required: ['id', 'name'],
              additionalProperties: true,
            },
          },
        },
        additionalProperties: true,
      },
      web: {
        type: 'object',
        title: 'Веб-часть',
        properties: {
          menu: {
            type: 'object',
            properties: {
              auto: { type: 'boolean', default: true },
              extra: {
                type: 'array',
                items: {
                  type: 'object',
                  properties: {
                    name: { type: 'string' },
                    icon: { type: 'string' },
                    path: { type: 'string', pattern: '^/' },
                  },
                  required: ['name', 'path'],
                  additionalProperties: true,
                },
              },
            },
            additionalProperties: true,
          },
          pages: {
            type: 'object',
            title: 'Базовые страницы',
            properties: pageProperties,
            additionalProperties: false,
          },
          dashboard: {
            type: 'object',
            properties: {
              cards: {
                type: 'array',
                items: { enum: DASHBOARD_CARDS },
                uniqueItems: true,
              },
            },
            additionalProperties: true,
          },
          locales: {
            type: 'array',
            items: { enum: localeNames },
            uniqueItems: true,
          },
        },
        additionalProperties: true,
      },
    },
    additionalProperties: false,
  };
  return `${JSON.stringify(schema, null, 2)}\n`;
};

const resolved = {
  version,
  project: {
    name: project.name,
    board: project.board,
    buildProfile: project.buildProfile,
    firmwareVersion: project.firmwareVersion,
    deviceName: project.deviceName,
    defaultEnv: project.board,
  },
  features,
  enabledModules: FEATURES.filter((id) => features[id]),
  firmware: {
    files: FEATURES.filter((id) => features[id]).flatMap((id) => MODULES[id]?.firmware?.files ?? []),
    srcFilter: ['+<*>', '-<tests/*>', ...disabledSrcExcludes.map((pattern) => `-<${pattern}>`)].join(' '),
  },
  web: {
    pages,
    menu,
    servicePages: enabledServicePages.map(([id]) => id),
    dashboardCards,
    locales,
    disabledPages,
    droppedLocaleSections,
  },
  warnings,
};

// ---------------------------------------------------------------------------
// Write
// ---------------------------------------------------------------------------

writeOrCheck('src/generated/vuespConfig.h', renderConfigHeader());
writeOrCheck('src/generated/scriptManifest.h', renderScriptManifest());
writeOrCheck('vue/src/generated/vuespConfig.ts', renderTsConfig());
writeOrCheck('vue/src/generated/modules.ts', renderModulesTs());
writeOrCheck('vue/src/generated/servicePages.ts', renderServicePages());
writeOrCheck('vue/src/generated/dashboardCards.ts', renderDashboardCards());
writeOrCheck('vue/src/generated/pages.json', renderPagesJson());
writeOrCheck('vue/src/generated/localeSections.json', renderLocaleSections());
writeOrCheck('vue/src/generated/dashboardTypes.json', `${JSON.stringify({ types: dashboardCards }, null, 2)}\n`);
writeOrCheck('src/generated/gpioConfig.h', renderGpioConfig());
writeOrCheck('platformio.generated.ini', renderPlatformioIni());
writeOrCheck('config/project.resolved.json', `${JSON.stringify(resolved, null, 2)}\n`);
writeOrCheck('config/modules.generated.json', `${JSON.stringify({
  version: 1,
  modules: FEATURES.map((id) => ({
    id,
    label: MODULES[id].label,
    description: MODULES[id].description ?? {},
    category: MODULES[id].category,
    default: MODULES[id].default === true,
    removable: MODULES[id].removable !== false,
    requires: MODULES[id].requires ?? {},
    conflicts: MODULES[id].conflicts ?? {},
    firmware: MODULES[id].firmware ?? {},
    web: MODULES[id].web ?? {},
  })),
}, null, 2)}\n`);
writeOrCheck('vue/json/menu.json', `${JSON.stringify(menu, null, 2)}\n`);
writeOrCheck('vuesp.config.schema.json', renderSchema());

if (check) {
  if (stale.length > 0) {
    console.error('[config] generated files are stale:');
    for (const path of stale) console.error(`  ${path}`);
    console.error('[config] run: cd vue && npm run config');
    process.exit(1);
  }
  console.log('[config] up to date');
} else {
  console.log('[config] generated vuespConfig.h, scriptManifest.h, web config, pages, locales and menu');
}

if (process.argv.includes('--preview')) {
  console.log(JSON.stringify(resolved, null, 2));
}
