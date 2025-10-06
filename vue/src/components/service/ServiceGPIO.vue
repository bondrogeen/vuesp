<template>
  <div>
    <card-gray :title="$t('ports')">
      <div class="relative grid grid-cols-1 xl:grid-cols-2 2xl:grid-cols-3 gap-4">
        <div v-for="port in main.ports" :key="port.gpio" class="p-4 bg-gray-100/40 dark:bg-gray-700/10 rounded-md border border-gray-200 dark:border-gray-700/40">
          <h5 class="text-sm mb-2">{{ `GPIO: ${port.gpio} ${!port.state ? `(${$t('dis')})` : ''}` }}</h5>

          <div class="flex flex-col gap-4 md:flex-row">
            <div class="grid gap-4 flex-auto" :class="isInput(port) ? 'grid-cols-1 md:grid-cols-2' : 'grid-cols-1'">
              <v-select :value="port.mode" :label="$t('mode')" :disabled="!port.state" hideMessage :list="listMode" @change="onMode(port, $event)" />

              <v-select v-if="isInput(port)" hideMessage :value="port.interrupt" :disabled="!port.state" :label="$t('interrupt')" :list="listInterrupt" @change="onInterrupt(port, $event)" />
            </div>

            <div class="md:flex-[0_0_80px]">
              <v-button v-if="isOutput(port)" color="blue" class="w-full" :disabled="!port.mode || !port.state" @click="onSetPort(port, port.value ? 0 : 1)">
                {{ port.value ? $t('on') : $t('off') }}
              </v-button>

              <v-text-field
                v-if="isInput(port) || isPWM(port)"
                hideMessage
                :modelValue="port.value"
                :disabled="isInput(port) || !port.state"
                :label="$t('value')"
                class="md:max-w-20"
                @update:modelValue="onInputValue(port, $event)"
              ></v-text-field>
            </div>
          </div>
        </div>
      </div>
    </card-gray>

    <Teleport to="[data-slot='device']">
      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <v-icons name="Dots" class="rotate-90"></v-icons>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenu" />
      </v-dropdown>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import type { IListItem, IMessagePort } from '@/types';
import { KEYS } from '@/types';

import { MODE, COMMAND } from '@/utils/gpio';

import { useConnection } from '@/composables/useConnection';
import { useFetch } from 'vuesp-components/helpers';
import { PATH_FS, pathGPIO } from '@/utils/const';
import { useLocale } from '@/composables/useLocale';

const { $t } = useLocale();
const { main, onSend, onDialog } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
});

const listMenu: IListItem[] = [
  { name: $t('save'), value: 1 },
  { name: $t('restore'), value: 2 },
];

const listMode: IListItem[] = [
  { name: 'INPUT', value: MODE.INPUT },
  { name: 'INPUT_PULLUP', value: MODE.INPUT_PULLUP },
  { name: 'OUTPUT', value: MODE.OUTPUT },
  { name: 'OUTPUT_OPEN_DRAIN', value: MODE.OUTPUT_OPEN_DRAIN },
  { name: 'PWM', value: MODE.PWM },
  { name: 'ONEWIRE', value: MODE.ONEWIRE },
  // { name: 'INPUT_PULLDOWN_16', value: 12 }, // 0x04
  // { name: 'WAKEUP_PULLUP', value: 13 }, // 0x05
  // { name: 'WAKEUP_PULLDOWN', value: 15 }, // 0x07
];

const listInterrupt: IListItem[] = [
  { name: 'OFF', value: 0 }, // 0x00
  { name: 'RISING', value: 1 }, // 0x01
  { name: 'FALLING', value: 2 }, // 0x02
  { name: 'CHANGE', value: 3 }, // 0x03
];

const isOutput = ({ mode = 0 }: IMessagePort) => [MODE.OUTPUT, MODE.OUTPUT_OPEN_DRAIN].includes(mode);
const isInput = ({ mode = 0 }: IMessagePort) => [MODE.INPUT, MODE.INPUT_PULLUP].includes(mode);
const isPWM = ({ mode = 0 }: IMessagePort) => [MODE.PWM].includes(mode);

const onSetPort = (port: IMessagePort, value: number) => onSend(KEYS.PORT, { ...port, command: COMMAND.GPIO_COMMAND_SET, value });
const onInputValue = (port: IMessagePort, value: string) => onSend(KEYS.PORT, { ...port, command: COMMAND.GPIO_COMMAND_SET, value: +value });

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
  const res = await useFetch.$delete(`${PATH_FS}?file=${pathGPIO}`);
  if (res?.state) onReboot();
};

const onMenu = ({ value }: IListItem) => {
  if (value === 1) onSave();
  if (value === 2) onRestore();
};
</script>
