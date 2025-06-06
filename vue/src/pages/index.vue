<template>
  <div>
    <div class="mb-6 flex items-center justify-between">
      <h1>Main</h1>

      <div class="v-spacer"></div>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-dots class="rotate-90"></icon-dots>
          </v-button>
        </template>

        <v-list :list="listPage" @click="onPage"></v-list>
      </v-dropdown>
    </div>

    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-5 gap-4">
      <div class="col-span-full">
        <VCardGray title="Date">
          <div class="flex items-center">
            <input :value="datetime" type="datetime-local" @change="onDate" />
          </div>
        </VCardGray>
      </div>

      <VCardGray class="col-span-full" title="GPIO">
        <div class="flex flex-col gap-2">
          <div v-for="pin in ports" :key="pin.gpio">
            <div v-if="pin" class="flex justify-between">
              <div>
                PIN: {{ pin.gpio }}
                <span class="text-gray-500 text-sm">( {{ getModeName(pin) }} )</span>
              </div>

              <v-button color="blue" class="min-w-20" :disabled="isDisabled(pin)" @click="onSetPort(pin, !getStateValue(gpio, pin))">{{ getStateValue(gpio, pin) ? 'ON' : 'OFF' }}</v-button>
            </div>
          </div>
        </div>
      </VCardGray>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue';
import { useRouter } from 'vue-router';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore.ts';

import { usePorts } from '@/composables/usePorts.ts';

const webSocketStore = useWebSocketStore();
const { device, gpio } = storeToRefs(webSocketStore);

const router = useRouter();

const listPage = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Save default' },
];

const datetime = computed(() => new Date((device.value.now || 0) * 1000).toISOString().slice(0, 16));

const now = ref(0);

const onSend = ({ comm, data }: any) => {
  webSocketStore.onSend(comm, data);
};

const { ports, init, isDisabled, getModeName, getStateValue, onSetPort } = usePorts(onSend);

const onPage = ({ id }: any) => {
  if (id === 1) {
    router.push('/config');
  }
  if (id === 2) {
    onSaveDef();
  }
};

const onSaveDef = () => {
  webSocketStore.onSend('DEVICE', { ...device.value, command: 4 });
};

const onDate = (e: any) => {
  const _now: number = e?.target?.valueAsNumber;
  if (_now) now.value = _now / 1000;
  webSocketStore.onSend('DEVICE', { now: now.value, command: 1 });
};

onMounted(async () => {
  await init();
});
</script>
