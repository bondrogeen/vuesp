<template>
  <div>
    <VCardGray title="Ports">
      <div class="relative flex flex-col">
        {{ gpio }}
        <div v-for="port in ports" :key="port.gpio">
          <div v-if="port" class="flex justify-between">
            <VSelect class="max-w-[250px]" :value="port.mode" :label="`GPIO: ${port.gpio}`" :list="listMode" @change="onMode(port, $event)" />

            <VSelect class="max-w-[250px]" :value="port.interrupt" :disabled="!!port.mode" :label="`Interrupt: ${port.gpio}`" :list="listInterrupt" @change="onInterrupt(port, $event)" />

            <v-button color="blue" class="ml-2 min-w-20" :disabled="!port.mode" @click="onSetPort(port, getStateValue(gpio, port) ? 0 : 1)">{{ getStateValue(gpio, port) ? 'ON' : 'OFF' }}</v-button>
          </div>
        </div>
      </div>
    </VCardGray>

    <Teleport to="[data-slot='device']">
      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-dots class="rotate-90"></icon-dots>
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
import { defineProps, defineEmits, onMounted } from 'vue';

// import { pathGPIO } from '@/utils/const.ts';

import type { IWSSend, IStateGpio } from '@/utils/types/types.ts';

import { usePorts } from '@/composables/usePorts.ts';

interface Props {
  gpio?: IStateGpio;
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

const { ports, listMode, listInterrupt, init, onSetPort, onMode, onInterrupt, getStateValue } = usePorts(onSend);

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
