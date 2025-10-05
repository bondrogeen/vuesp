<template>
  <div>
    <card-gray title="Ports">
      <div class="relative grid grid-cols-1 xl:grid-cols-2 2xl:grid-cols-3 gap-4">
        <div v-for="port in main.ports" :key="port.gpio" class="p-4 bg-gray-100/40 dark:bg-gray-700/10 rounded-md border border-gray-200 dark:border-gray-700/40">
          <h5 class="text-sm mb-2">{{ `GPIO: ${port.gpio} ${!port.state ? '(Disabled)' : ''}` }}</h5>

          <div class="flex flex-col gap-4 md:flex-row">
            <div class="grid gap-4 flex-auto" :class="isInput(port) ? 'grid-cols-1 md:grid-cols-2' : 'grid-cols-1'">
              <v-select class="" :value="port.mode" label="Mode" :disabled="!port.state" hideMessage :list="listMode" @change="onMode(port, $event)" />

              <v-select v-if="isInput(port)" class="" hideMessage :value="port.interrupt" :disabled="!port.state" :label="`Interrupt`" :list="listInterrupt" @change="onInterrupt(port, $event)" />
            </div>

            <div class="md:flex-[0_0_80px]">
              <v-button v-if="isOutput(port)" color="blue" class="w-full" :disabled="!port.mode || !port.state" @click="onSetPort(port, port.value ? 0 : 1)">
                {{ port.value ? 'ON' : 'OFF' }}
              </v-button>

              <v-text-field
                v-if="isInput(port) || isPWM(port)"
                hideMessage
                :modelValue="port.value"
                :disabled="isInput(port) || !port.state"
                label="Value"
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

import { MODE, COMMAND, listMenu, listMode, listInterrupt } from '@/utils/gpio';

import { useConnection } from '@/composables/useConnection';

const { main, onSend, onDialog } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
});

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
const onSureReboot = () => onDialog({ value: true, message: 'Do you want to restart your device?', callback: onReboot });

const onSave = async () => {
  onSend(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_SAVE });
  onSureReboot();
};

const onMenu = ({ value }: IListItem) => {
  if (value === 1) onSave();
};
</script>
