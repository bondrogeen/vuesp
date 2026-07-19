<template>
  <div>
    <card-main :title="$t('ports')">
      <div class="relative grid grid-cols-1 xl:grid-cols-2 2xl:grid-cols-3 gap-4">
        <div v-for="port in main.ports" :key="port.gpio" class="p-4 bg-gray-100/40 dark:bg-gray-700/10 rounded-md border border-gray-200 dark:border-gray-700/40">
          <h5 class="text-sm mb-2">{{ `GPIO: ${port.gpio} ${port.disabled ? `(${$t('dis')})` : ''}` }}</h5>

          <div class="flex flex-col gap-4 md:flex-row">
            <div class="grid gap-4 flex-auto" :class="isInput(port) ? 'grid-cols-1 md:grid-cols-2' : 'grid-cols-1'">
              <v-select :model-value="getNameList(port)" :label="$t('mode')" :disabled="Boolean(port.disabled)" hideMessage :items="getListMode(port)" @change="onMode(port, $event)" />

              <v-select
                v-if="isInput(port)"
                hideMessage
                :model-value="getNameListInterrupt(port)"
                :disabled="Boolean(port.disabled)"
                :label="$t('interrupt')"
                :items="listInterrupt"
                @change="onInterrupt(port, $event)"
              />
            </div>

            <div class="md:flex-[0_0_80px]">
              <v-button v-if="isOutput(port)" color="blue" class="w-full" :disabled="!port.mode || Boolean(port.disabled)" @click="onSetPort(port, port.value ? 0 : 1)">
                {{ port.value ? $t('on') : $t('off') }}
              </v-button>

              <v-text-field
                v-if="isInput(port) || isPWM(port) || isADC(port)"
                hideMessage
                :modelValue="port.value"
                :disabled="isInput(port) || Boolean(port.disabled)"
                :label="$t('value')"
                class="md:max-w-20"
                @update:modelValue="onInputValue(port, $event)"
              ></v-text-field>
            </div>
          </div>
        </div>
      </div>
    </card-main>

    <Teleport to="[data-slot='device']">
      <v-select :items="listMenu" @change="onMenu">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
          </v-button>
        </template>
      </v-select>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import type { IListItem, IMessagePort } from '@/types';
import { KEYS } from '@/types';

import { MODE_BOARD_1, MODE_BOARD_2, COMMAND, LIST } from '@/utils/gpio';

import { useDebounceFn } from '@vueuse/core';

import { useConnection } from '@/composables/useConnection';
import { useFetch } from '@vueuse/core';
import { PATH_FS, pathGPIO } from '@/utils/const';
import { useLocale } from '@/composables/useLocale';
import { computed } from 'vue';

const { $t } = useLocale();
const { main, onSend, onDialog } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
});

const MODE = computed(() => {
  if (main.value.info.board === 2) return MODE_BOARD_2;
  return MODE_BOARD_1;
});

const listMenu: IListItem[] = [
  { name: $t('save'), value: 1 },
  { name: $t('restore'), value: 2 },
];

const getListMode = (port: IMessagePort): IListItem[] => {
  const list = [];
  const data = port?.list || 0;

  // { name: 'INPUT_PULLDOWN_16', value: 12 }, // 0x04
  // { name: 'WAKEUP_PULLUP', value: 13 }, // 0x05
  // { name: 'WAKEUP_PULLDOWN', value: 15 }, // 0x07
  if (data & LIST.GPIO_BIT_INPUT) list.push({ name: 'INPUT', value: MODE.value.INPUT });
  if (data & LIST.GPIO_BIT_INPUT_PULLUP) list.push({ name: 'INPUT_PULLUP', value: MODE.value.INPUT_PULLUP });
  if (data & LIST.GPIO_BIT_OUTPUT) list.push({ name: 'OUTPUT', value: MODE.value.OUTPUT });
  if (data & LIST.GPIO_BIT_OUTPUT_OPEN_DRAIN) list.push({ name: 'OUTPUT_OPEN_DRAIN', value: MODE.value.OUTPUT_OPEN_DRAIN });
  if (data & LIST.GPIO_BIT_PWM) list.push({ name: 'PWM', value: MODE.value.PWM });
  if (data & LIST.GPIO_BIT_ONEWIRE) list.push({ name: 'ONEWIRE', value: MODE.value.ONEWIRE });
  if (data & LIST.GPIO_BIT_ADC) list.push({ name: 'ADC', value: MODE.value.ADC });
  if (data & LIST.GPIO_BIT_DAC) list.push({ name: 'DAC', value: MODE.value.DAC });

  return list;
};

const getNameList = (port: IMessagePort) => getListMode(port).find((i) => i.value === port.mode)?.name;
const getNameListInterrupt = (port: IMessagePort) => listInterrupt.find((i) => i.value === port.interrupt)?.name;

const listInterrupt: IListItem[] = [
  { name: 'OFF', value: 0 }, // 0x00
  { name: 'RISING', value: 1 }, // 0x01
  { name: 'FALLING', value: 2 }, // 0x02
  { name: 'CHANGE', value: 3 }, // 0x03
];

const isOutput = ({ mode = 0 }: IMessagePort) => [MODE.value.OUTPUT, MODE.value.OUTPUT_OPEN_DRAIN].includes(mode);
const isInput = ({ mode = 0 }: IMessagePort) => [MODE.value.INPUT, MODE.value.INPUT_PULLUP].includes(mode);
const isPWM = ({ mode = 0 }: IMessagePort) => [MODE.value.PWM].includes(mode);
const isADC = ({ mode = 0 }: IMessagePort) => [MODE.value.ADC].includes(mode);

const onSetPort = (port: IMessagePort, value: number) => onSend(KEYS.PORT, { ...port, command: COMMAND.GPIO_COMMAND_SET, value });
const onInputValue = useDebounceFn((port: IMessagePort, value: string) => onSend(KEYS.PORT, { ...port, command: COMMAND.GPIO_COMMAND_SET, value: +value }), 500);

const onMode = ({ gpio }: IMessagePort, { value }: IListItem) => {
  main.value.ports[gpio].mode = value as number;
  onSend(KEYS.PORT, { ...main.value.ports[gpio], command: COMMAND.GPIO_COMMAND_CHANGE });
};

const onInterrupt = ({ gpio }: IMessagePort, { value }: IListItem) => {
  main.value.ports[gpio].interrupt = value as number;
  onSend(KEYS.PORT, { ...main.value.ports[gpio], command: COMMAND.GPIO_COMMAND_CHANGE });
};

const onReboot = () => onSend(KEYS.REBOOT);
const onSureReboot = () => onDialog({ value: true, message: $t('dialog.doRestart'), callback: onReboot });

const onSave = async () => {
  onSend(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_SAVE });
  onSureReboot();
};
const onRestore = async () => {
  const { data } = await useFetch(`${PATH_FS}?file=${pathGPIO}`).delete().json();
  if (data.value?.state) onReboot();
};

const onMenu = ({ value }: IListItem) => {
  if (value === 1) onSave();
  if (value === 2) onRestore();
};
</script>
