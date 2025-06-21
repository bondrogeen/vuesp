<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Modbus</h1>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-dots class="rotate-90"></icon-dots>
          </v-button>
        </template>

        <v-list :list="listMenu" @click="onMenuEvent"></v-list>
      </v-dropdown>
    </div>

    <div class="mt-4">
      <VCardGray title="Master">
        <h5 class="mb-6"></h5>
        <div class="grid grid-cols-1 gap-4">
          <BlockModBus :data="modbus" @send="onSendModBus" />
        </div>
      </VCardGray>
    </div>
  </div>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import BlockModBus from '@/components/blocks/modbus/BlockModBus.vue';

const webSocketStore = useWebSocketStore();
const { modbus } = storeToRefs(webSocketStore);

interface TypeList {
  id: number;
  name: string;
}

const listMenu: TypeList[] = [
  { id: 1, name: 'Add item' },
  { id: 2, name: 'Restore' },
  { id: 3, name: 'Save' },
  { id: 2, name: 'Save default' },
];

const onMenuEvent = async ({ id }: TypeList) => {
  if (id === 1) {
  }
};

const onSendModBus = (data: any) => {
  webSocketStore.onSend('MODBUS', { command: 1, data, size: data.length });
};
</script>
