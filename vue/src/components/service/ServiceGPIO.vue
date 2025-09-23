<template>
  <div>
    <card-gray title="Ports">
      <div class="relative flex flex-col gap-4">
        <div v-for="port in main.gpio" :key="port.gpio" class="flex flex-col md:gap-4 md:flex-row">
          <v-select class="max-w-[250px]" :value="port.mode" :label="`GPIO: ${port.gpio}`" :disabled="!port.state" hideMessage :list="listMode" @change="onMode(port, $event)" />

          <v-select
            v-if="isInput(port)"
            class="max-w-[250px]"
            hideMessage
            :value="port.interrupt"
            :disabled="!port.state"
            :label="`Interrupt: ${port.gpio}`"
            :list="listInterrupt"
            @change="onInterrupt(port, $event)"
          />

          <div class="md:hidden flex-auto"></div>

          <v-button v-if="isOutput(port)" color="blue" class="w-20" :disabled="!port.mode" @click="onSetPort(port, port.value ? 0 : 1)">
            {{ port.value ? 'ON' : 'OFF' }}
          </v-button>

          <v-text-field
            v-if="isInput(port)"
            hideMessage
            :modelValue="port.value"
            :disabled="isInput(port)"
            label="Value"
            class="max-w-20"
            @update:modelValue="onInputValue(port, $event)"
          ></v-text-field>
        </div>
      </div>
    </card-gray>

    <Teleport to="[data-slot='device']">
      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <v-icons name="IconDots" class="rotate-90"></v-icons>
          </v-button>
        </template>

        <v-list v-slot="{ item }" :list="listMenu" @click="onMenu">
          <v-icons :name="item.icon" class="size-5"></v-icons>
          <span class="ms-2">{{ item.name }}</span>
        </v-list>
      </v-dropdown>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import type { IListItem, IMessagePort } from '@/types';
import { KEYS } from '@/types';

import { MODE, COMMAND, listMenu, listMode, listInterrupt } from '@/utils/gpio';

import { useConnection } from '@/composables/useConnection.js';

const isOutput = ({ mode = 0 }: IMessagePort) => [MODE.OUTPUT, MODE.OUTPUT_OPEN_DRAIN].includes(mode);
const isInput = ({ mode = 0 }: IMessagePort) => [MODE.INPUT, MODE.INPUT_PULLUP].includes(mode);

const onInputValue = ({ gpio }: IMessagePort, { value }: IListItem) => (main.value.gpio[gpio].value = value as number);

const onMode = ({ gpio }: IMessagePort, { value }: IListItem) => (main.value.gpio[gpio].mode = value as number);
const onInterrupt = ({ gpio }: IMessagePort, { value }: IListItem) => (main.value.gpio[gpio].interrupt = value as number);
const onSetPort = (port: IMessagePort, value: number) => onSend(KEYS.PORT, { ...port, command: COMMAND.GPIO_COMMAND_SET, value });

const { main, onSend } = useConnection((send) => {
  send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
});

const onMenu = (e: Event) => {
  onSave();
  console.log(e);
};

const onSave = async () => {
  onSend(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
};
</script>
