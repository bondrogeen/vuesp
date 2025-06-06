<template>
  <div>
    <VCardGray title="Ports">
      <div class="relative flex flex-col">
        <div v-for="pin in ports" :key="pin.gpio">
          <div v-if="pin" class="flex justify-between">
            <VSelect class="max-w-[250px]" :value="getModeName(pin)" :label="`GPIO: ${pin.gpio}`" :list="listMode" @change="onMode(pin, $event)" />

            <v-button color="blue" class="ml-2 min-w-20" :disabled="isDisabled(pin)" @click="onSetPort(pin, !getStateValue(gpio, pin))">{{ getStateValue(gpio, pin) ? 'ON' : 'OFF' }}</v-button>
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

import { pathGPIO } from '@/utils/const.ts';

import type { TypeWSSend } from '@/types/types.ts';

import { usePorts } from '@/composables/usePorts.ts';

interface Props {
  gpio?: any;
}

const { gpio = {} } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'click', value: boolean): void;
  (e: 'send', value: TypeWSSend): void;
  (e: 'reboot', value: Event): void;
}>();

const listMenu = [
  { name: 'Update', icon: 'IconUpdate' },
  { name: 'Save', icon: 'IconSave' },
];

const onSend = (data: any) => {
  emit('send', data);
};

const { ports, portsDef, listMode, init, isDisabled, getModeName, getStateValue, onSetPort, onMode, onUploadBinary, stringifyDateGPIO } = usePorts(onSend);

const onMenu = (e: Event) => {
  onSave(e);
};

const onSave = async (e: Event) => {
  const data = stringifyDateGPIO(ports.value);
  const buffer = new Uint8Array(data).buffer;
  await onUploadBinary(pathGPIO, buffer);
  portsDef.value = JSON.parse(JSON.stringify(ports.value));
  emit('reboot', e);
};

onMounted(async () => {
  init();
});
</script>
