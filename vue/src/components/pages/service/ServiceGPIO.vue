<template>
  <div>
    <CardGray title="Ports">
      <div class="relative flex flex-col gap-4">
        <div v-for="pin in ports" :key="pin.gpio">
          <div v-if="pin" class="flex justify-between">
            <VSelect class="max-w-[250px]" :value="getModeName(pin)" :label="`GPIO: ${pin.gpio}`" :list="listMode" @change="onMode(pin, $event)" />

            <VButton color="blue" class="ml-2 min-w-20" :disabled="isDisabled(pin)" @click="onSetPort(pin, !getStateValue(gpio, pin))">{{ getStateValue(gpio, pin) ? 'ON' : 'OFF' }}</VButton>
          </div>
        </div>
      </div>
    </CardGray>

    <Teleport to="[data-slot='device']">
      <VDropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <VButton type="" @click="on.click">
            <IconDots class="rotate-90"></IconDots>
          </VButton>
        </template>

        <VList v-slot="{ item }" :list="listMenu" @click="onMenu">
          <component :is="item.icon"></component>
          <span class="ms-2">{{ item.name }}</span>
        </VList>
      </VDropdown>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import { defineProps, defineEmits, onMounted } from 'vue';

import { pathGPIO } from '@/utils/const.ts';

import type { TypeWSSend } from '@/types/types.ts';

import { usePorts } from '@/composables/usePorts.ts';

import VSelect from '@/components/general/VSelect.vue';
import VButton from '@/components/general/VButton.vue';
import VList from '@/components/general/VList.vue';
import VDropdown from '@/components/general/VDropdown.vue';
import CardGray from '@/components/cards/CardGray.vue';

import IconDots from '@/components/icons/IconDots.vue';
import IconSave from '@/components/icons/IconSave.vue';
import IconUpdate from '@/components/icons/IconUpdate.vue';

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
  { name: 'Update', icon: IconUpdate },
  { name: 'Save', icon: IconSave },
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
