// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/generate-script-manifest.mjs

export const SCRIPT_EVENTS = [
  { id: 1, name: 'btn', params: 2, args: ['gpio', 'value'], description: 'button edge: press or release' },
  { id: 2, name: 'btn_c', params: 3, args: ['gpio', 'value', 'count'], description: 'button click with click count' },
  { id: 3, name: 'btn_l', params: 2, args: ['gpio', 'value'], description: 'button long press' },
  { id: 4, name: 'btn_r', params: 2, args: ['gpio', 'value'], description: 'button hold repeat' },
  { id: 5, name: 'modbus', params: 0, args: [], description: 'Modbus event' },
  { id: 6, name: 'time', params: 2, args: ['hours', 'minutes'], description: 'device uptime clock' },
] as const;

export const SCRIPT_EVENT_ARG_NAMES: Readonly<Record<string, readonly string[]>> = {
  btn: ['gpio', 'value'],
  btn_c: ['gpio', 'value', 'count'],
  btn_l: ['gpio', 'value'],
  btn_r: ['gpio', 'value'],
  modbus: [],
  time: ['hours', 'minutes'],
};

export const SCRIPT_FUNCTIONS = [
  { id: 1, name: 'fade', min: 3, max: 3, ret: 'int', description: 'PWM fade' },
  { id: 2, name: 'udp', min: 1, max: 2, ret: 'int', description: 'UDP send' },
  { id: 3, name: 'http', min: 2, max: 5, ret: 'int', description: 'HTTP request' },
  { id: 4, name: 'var', min: 1, max: 2, ret: 'float', description: 'read a Modbus variable' },
  { id: 5, name: 'modbus', min: 2, max: 3, ret: 'float', description: 'Modbus operation' },
] as const;

export const SCRIPT_LIMITS = {
  maxSavedScripts: 15,
  cacheSlots: 6,
  maxScripts: 15,
  maxContexts: 15,
  maxCodeBytes: 512,
  stackSize: 64,
  scratchBytes: 256,
  maxStringLen: 32,
  maxArrayLen: 64,
  maxEventParams: 4,
  instrPerTick: 32,
  usPerTick: 300,
  updateBudgetUs: 3000,
  vars: { u: 10, i: 10, f: 5, s: 2, a: 5, p: 40, e: 4, c: 4 },
} as const;
