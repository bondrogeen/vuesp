<template>
  <div>
    <card-gray title="Ports">
      <div class="relative flex flex-col">
        <div v-for="port in ports" :key="port.gpio" class="flex flex-col md:gap-4 md:flex-row">
          <v-select class="max-w-[250px]" :value="port.mode" :label="`GPIO: ${port.gpio}`" hideMessage :list="listMode" @change="onMode(port, $event)" />

          <v-select v-if="isInput(port)" class="max-w-[250px]" hideMessage :value="port.interrupt" :label="`Interrupt: ${port.gpio}`" :list="listInterrupt" @change="onInterrupt(port, $event)" />

          <div class="md:hidden flex-auto"></div>

          <v-button v-if="isOutput(port)" color="blue" class="w-20" :disabled="!port.mode" @click="onSetPort(port, getStateValue(gpio, port) ? 0 : 1)">
            {{ getStateValue(gpio, port) ? 'ON' : 'OFF' }}
          </v-button>

          <v-text-field
            v-if="isInput(port)"
            hideMessage
            :modelValue="getStateValue(gpio, port)"
            :disabled="isInput(port)"
            label="Value"
            class="max-w-20"
            @update:modelValue="onInputValue"
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
          <component :is="item.icon"></component>
          <span class="ms-2">{{ item.name }}</span>
        </v-list>
      </v-dropdown>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import type { IWSSend, IStoreGpio } from 'vuesp-components/types';

import { defineProps, defineEmits, onMounted } from 'vue';

// import { pathGPIO } from '@/utils/const.ts';

import { usePorts } from '@/composables/usePorts.ts';

interface Props {
  gpio?: Record<string, IStoreGpio>;
}

const { gpio = {} } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'click', value: boolean): void;
  (e: 'send', value: IWSSend): void;
  (e: 'reboot', value: Event): void;
}>();

const listMenu = [
  { name: 'Update', icon: 'IconUpdate' },
  { name: 'Save', icon: 'IconSave' },
];

const onSend = (data: any) => {
  emit('send', data);
};

const { ports, listMode, listInterrupt, init, onSetPort, onMode, onInterrupt, getStateValue, isOutput, isInput, onInputValue } = usePorts(onSend);

const onMenu = (e: Event) => {
  onSave(e);
};

const onSave = async (e: Event) => {
  // const data = stringifyDateGPIO(ports.value);
  // const buffer = new Uint8Array(data).buffer;
  // await onUploadBinary(pathGPIO, buffer);
  emit('reboot', e);
};

onMounted(async () => {
  init();
});
</script>
