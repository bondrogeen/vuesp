<template>
  <div>
    <div class="mb-6 flex items-center justify-between">
      <h1>{{ $t('home') }}</h1>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <v-icon name="Dots" class="rotate-90"></v-icon>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenu"></v-list>
      </v-dropdown>
    </div>

    <div class="w-full grid grid-cols-1 md:grid-cols-2 2xl:grid-cols-3 gap-4">
      <card-main class="p-4">
        <div class="flex justify-between items-center mb-4 text-xs">
          <span class="text-2xl font-bold">JBD BMS</span>

          <div class="flex flex-col">
            <span class="font-semibold">
              Ver:
              <span class="text-gray-500">{{ toFixed(+device.version.toString(16) / 10, 1) }}</span>
            </span>

            <span class="font-semibold">
              Date:
              <span class="text-gray-500">{{ `${device.dateDay}.${device.dateMonth}.${device.dateYear}` }}</span>
            </span>
          </div>
        </div>

        <hr class="my-4 h-px bg-gray-200 dark:bg-gray-800 border-0 rounded" />

        <div class="flex justify-between items-center mb-4 text-xs">
          <span class="text-2xl font-bold">Status</span>

          <div class="flex">
            <span
              class="flex justify-center flex-[0_0_110px] min-w-12 text-sm font-semibold px-2 py-1 rounded-full border border-gray-200 dark:border-gray-800 dark:bg-white/[0.03]"
              :class="{ 'animate-shake text-green-500': device.current < 0, 'animate-shake text-red-500/80': device.current > 0 }"
            >
              {{ getStatus(device.current) }}
            </span>
          </div>
        </div>

        <div class="flex justify-between mt-1">
          <div>
            <div class="text-gray-500 text-xs uppercase">SOC</div>

            <div class="text-4xl font-bold">
              {{ device.rsoc }}
              <span class="text-gray-500">%</span>
            </div>
          </div>

          <div class="text-right">
            <div class="text-gray-500 text-xs uppercase">Remaining capacity</div>

            <div class="text-xl font-bold">
              {{ device.balanceCapacity / 100 }}
              /
              {{ device.rateCapacity / 100 }}
              <span class="text-sm font-normal text-gray-500">AH</span>
            </div>
          </div>
        </div>
      </card-main>

      <card-main class="p-4">
        <div class="grid grid-cols-2 gap-4">
          <div class="flex flex-col gap-2">
            <div class="flex items-center justify-between gap-2">
              <span class="text-sm">ChgMos:</span>

              <span
                class="flex justify-center flex-[0_0_60px] min-w-12 text-sm font-semibold px-2 py-1 rounded-full border border-gray-200 dark:border-gray-800 dark:bg-white/[0.03]"
                :class="{ 'animate-shake text-green-500': device.fet & 1 }"
              >
                {{ device.fet & 1 ? 'ON' : 'OFF' }}
              </span>
            </div>
            <div class="flex items-center justify-between gap-2">
              <span class="text-sm">DisMos:</span>

              <span
                class="flex justify-center flex-[0_0_60px] min-w-12 text-sm font-semibold px-2 py-1 rounded-full border border-gray-200 dark:border-gray-800 dark:bg-white/[0.03]"
                :class="{ 'animate-shake text-green-500': device.fet & 2 }"
              >
                {{ device.fet & 2 ? 'ON' : 'OFF' }}
              </span>
            </div>
          </div>

          <div class="flex flex-col gap-2">
            <div class="flex items-center justify-between gap-2">
              <span class="text-sm">Balance:</span>

              <span
                class="flex justify-center flex-[0_0_60px] min-w-12 text-sm font-semibold px-5 py-1 rounded-full border border-gray-200 dark:border-gray-800 dark:bg-white/[0.03]"
                :class="{ 'animate-shake text-green-500': device.balanceStatus }"
              >
                {{ device.balanceStatus ? 'ON' : 'OFF' }}
              </span>
            </div>

            <div class="flex items-center justify-between gap-2">
              <span class="text-sm">Protection:</span>

              <span
                class="flex justify-center flex-[0_0_60px] min-w-12 text-sm font-semibold px-5 py-1 rounded-full border border-gray-200 dark:border-gray-800 dark:bg-white/[0.03]"
                :class="{ 'animate-shake text-red-500': device.protectionStatus }"
              >
                {{ device.protectionStatus ? 'ON' : 'OFF' }}
              </span>
            </div>
          </div>
        </div>

        <ul class="animate-pulse">
          <li v-for="item of getActiveProtections(device.protectionStatus)" :key="item" class="text-xs text-red-500/80">
            {{ item }}
          </li>
        </ul>
      </card-main>

      <card-main class="p-4 md:col-span-2 2xl:col-span-1">
        <div class="grid grid-cols-3 gap-2 text-center">
          <div>
            <div class="text-gray-500 text-xs uppercase font-medium">Total voltage</div>

            <div class="font-semibold text-lg">
              {{ toFixed(device.voltage / 100) }}
              <span class="text-xs text-gray-500">V</span>
            </div>
          </div>
          <div>
            <div class="text-gray-500 text-xs uppercase font-medium">Current</div>

            <div class="font-semibold text-lg">
              {{ toFixed(device.current / 100) }}
              <span class="text-xs text-gray-500">A</span>
            </div>
          </div>
          <div>
            <div class="text-gray-500 text-xs uppercase font-medium">Power</div>

            <div class="font-semibold text-lg">
              {{ Math.round(device.current ? (device.voltage / 100) * (device.current / 100) : 0) }}
              <span class="text-xs text-gray-500">W</span>
            </div>
          </div>
        </div>

        <hr class="my-4 h-px bg-gray-200 dark:bg-gray-800 border-0 rounded" />

        <div class="grid grid-cols-3 gap-2 text-center">
          <div>
            <div class="text-gray-500 text-xs uppercase font-medium">temp 1</div>

            <div class="font-semibold text-lg">
              {{ toFixed(device.ntc1 / 100) }}
              <span class="text-xs text-gray-500">°C</span>
            </div>
          </div>

          <div v-if="device.ntc > 1">
            <div class="text-gray-500 text-xs uppercase font-medium">temp 2</div>

            <div class="font-semibold text-lg">
              {{ toFixed(device.ntc2 / 100) }}
              <span class="text-xs text-gray-500">°C</span>
            </div>
          </div>

          <div v-if="device.ntc > 2">
            <div class="text-gray-500 text-xs uppercase font-medium">temp 3</div>

            <div class="font-semibold text-lg">
              {{ toFixed(device.ntc3 / 100) }}
              <span class="text-xs text-gray-500">°C</span>
            </div>
          </div>
        </div>
      </card-main>

      <card-main class="p-4 md:col-span-2 2xl:col-span-4">
        <div class="grid grid-cols-4 gap-1 text-center">
          <div>
            <div class="text-gray-500 text-xs uppercase font-medium">VolHigh</div>

            <div class="font-semibold">
              {{ toFixed(device.cellHigh / 1000, 3) }}
              <span class="text-xs text-gray-500">V</span>
            </div>
          </div>
          <div>
            <div class="text-gray-500 text-xs uppercase font-medium">VolLow</div>

            <div class="font-semibold">
              {{ toFixed(device.cellLow / 1000, 3) }}
              <span class="text-xs text-gray-500">V</span>
            </div>
          </div>
          <div>
            <div class="text-gray-500 text-xs uppercase font-medium">AveVol</div>

            <div class="font-semibold">
              {{ toFixed(device.cellAvg / 1000, 3) }}
              <span class="text-xs text-gray-500">V</span>
            </div>
          </div>
          <div>
            <div class="text-gray-500 text-xs uppercase font-medium">Δ</div>

            <div class="font-semibold">
              {{ toFixed(device.cellDiff / 1000, 3) }}
              <span class="text-xs text-gray-500">V</span>
            </div>
          </div>
        </div>

        <hr class="my-4 h-px bg-gray-200 dark:bg-gray-800 border-0 rounded" />

        <div class="grid grid-cols-2 md:grid-cols-2 xl:grid-cols-8 gap-2">
          <div v-for="i of device.series" :key="i" class="relative">
            <div class="text-center py-1.5 border border-gray-200 dark:border-gray-800 dark:bg-white/[0.03] rounded-xl relative overflow-hidden">
              <div class="text-sm text-gray-500">{{ `#${i}` }}</div>

              <div class="font-bold text-base" :class="getColorCell(device.cellVoltage[i - 1])">
                {{ toFixed(device.cellVoltage[i - 1] / 1000, 3) }}
                <span class="text-xs text-gray-500">V</span>
              </div>

              <div
                class="absolute top-0 left-0 h-full transition-all duration-300 ease-in-out opacity-10"
                :style="{
                  width: getBatteryPercent(device.cellVoltage[i - 1]) + '%',
                  backgroundColor: chargeColor(device.cellVoltage[i - 1]),
                }"
              ></div>

              <div v-if="isBalance(device.balanceStatus, i)" class="absolute top-1 left-2 size-5 rounded-full bg-gray-200 dark:bg-gray-800 flex items-center justify-center border border-blue-500">
                <span class="text-xs">B</span>
              </div>
            </div>

            <div class="absolute top-1/2 left-[calc(100%-1px)] -translate-y-1/2 h-8 w-1 rounded-tr-xl rounded-br-xl bg-gray-200 dark:bg-gray-800"></div>
          </div>
        </div>
      </card-main>
    </div>
  </div>
</template>

<script setup lang="ts">
import type { IListItem } from '@/types';

import { KEYS } from '@/types';

import { useConnection } from '@/composables/useConnection';

import { useLocale } from '@/composables/useLocale';
import { computed } from 'vue';

const { $t } = useLocale();

const { main, onSend } = useConnection((send) => {
  send(KEYS.DEVICE);
});

const device = computed(() => main.value.device);

const listMenu: IListItem[] = [{ name: $t('btnUpdate'), value: 2 }];

const onUpdate = () => {
  onSend('INFO');
};

const onMenu = ({ value }: IListItem) => {
  if (value === 2) onUpdate();
};

const toFixed = (num: number, i = 2) => num.toFixed(i);
const getColorCell = (v: number) => {
  if (v === device.value.cellLow) return 'animate-shake text-red-400/80';
  if (v === device.value.cellHigh) return 'animate-shake text-green-600';
  return '';
};
const getStatus = (v: number) => {
  if (v < 0) return 'Charge';
  if (v > 0) return 'Discharge';
  return 'Normal';
};

const isBalance = (value: number, i: number) => value & (1 << (i - 1));

const PROTECTION_FLAGS = {
  CELL_OVERVOLTAGE: 1 << 0,
  CELL_UNDERVOLTAGE: 1 << 1,
  PACK_OVERVOLTAGE: 1 << 2,
  PACK_UNDERVOLTAGE: 1 << 3,
  CHARGE_OVERTEMP: 1 << 4,
  CHARGE_LOWTEMP: 1 << 5,
  DISCHARGE_OVERTEMP: 1 << 6,
  DISCHARGE_LOWTEMP: 1 << 7,
  CHARGE_OVERCURRENT: 1 << 8,
  DISCHARGE_OVERCURRENT: 1 << 9,
  FRONTEND_IC_ERROR: 1 << 10,
  SOFTWARE_LOCK_MOS: 1 << 11,
} as const;

const PROTECTION_NAMES: Record<number, string> = {
  [PROTECTION_FLAGS.CELL_OVERVOLTAGE]: 'Single cell overvoltage protection',
  [PROTECTION_FLAGS.CELL_UNDERVOLTAGE]: 'Single cell undervoltage protection',
  [PROTECTION_FLAGS.PACK_OVERVOLTAGE]: 'Whole pack overvoltage protection',
  [PROTECTION_FLAGS.PACK_UNDERVOLTAGE]: 'Whole pack undervoltage protection',
  [PROTECTION_FLAGS.CHARGE_OVERTEMP]: 'Charging over-temperature protection',
  [PROTECTION_FLAGS.CHARGE_LOWTEMP]: 'Charging low temperature protection',
  [PROTECTION_FLAGS.DISCHARGE_OVERTEMP]: 'Discharge over temperature protection',
  [PROTECTION_FLAGS.DISCHARGE_LOWTEMP]: 'Discharge low temperature protection',
  [PROTECTION_FLAGS.CHARGE_OVERCURRENT]: 'Charging overcurrent protection',
  [PROTECTION_FLAGS.DISCHARGE_OVERCURRENT]: 'Discharge overcurrent protection',
  [PROTECTION_FLAGS.FRONTEND_IC_ERROR]: 'Front-end detection IC error',
  [PROTECTION_FLAGS.SOFTWARE_LOCK_MOS]: 'Software lock MOS',
};

const getActiveProtections = (value: number): string[] => {
  return Object.entries(PROTECTION_FLAGS)
    .filter(([, bit]) => value & bit)
    .map(([key]) => PROTECTION_NAMES[PROTECTION_FLAGS[key as keyof typeof PROTECTION_FLAGS]]);
};

const getBatteryPercent = (mv: number): number => {
  const p: [number, number][] = [
    [2500, 0],
    [3000, 20],
    [3100, 40],
    [3200, 50],
    [3250, 60],
    [3300, 70],
    [3350, 85],
    [3400, 95],
    [3450, 99],
    [3500, 100],
  ];

  if (mv <= p[0][0]) return 0;
  if (mv >= p[p.length - 1][0]) return 100;

  const i = p.findIndex(([voltage]) => mv < voltage);
  if (i <= 0) return 0;

  const [v1, p1] = p[i - 1];
  const [v2, p2] = p[i];

  return Math.round(p1 + ((p2 - p1) * (mv - v1)) / (v2 - v1));
};

// const getLiIonPercent = (mv: number): number => {
//   const p: [number, number][] = [
//     [3000, 0],
//     [3200, 5],
//     [3400, 15],
//     [3500, 30],
//     [3600, 50],
//     [3700, 70],
//     [3800, 85],
//     [3900, 95],
//     [4100, 99],
//     [4200, 100],
//   ];

//   if (mv <= p[0][0]) return 0;
//   if (mv >= p[p.length - 1][0]) return 100;

//   const i = p.findIndex(([voltage]) => mv < voltage);
//   if (i <= 0) return 0;

//   const [v1, p1] = p[i - 1];
//   const [v2, p2] = p[i];

//   return Math.round(p1 + ((p2 - p1) * (mv - v1)) / (v2 - v1));
// };

const chargeColor = (v: number) => {
  const progress = Math.min(Math.max(getBatteryPercent(v) / 100, 0), 1);
  const startColor = { r: 239, g: 68, b: 68 };
  const endColor = { r: 34, g: 197, b: 94 };
  const red = Math.round(startColor.r + (endColor.r - startColor.r) * progress);
  const green = Math.round(startColor.g + (endColor.g - startColor.g) * progress);
  const blue = Math.round(startColor.b + (endColor.b - startColor.b) * progress);
  return `#${red.toString(16).padStart(2, '0')}${green.toString(16).padStart(2, '0')}${blue.toString(16).padStart(2, '0')}`;
};
</script>
