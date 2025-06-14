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

    <!-- <div class="grid grid-cols-[repeat(auto-fill,_minmax(100px,_1fr))] gap-4">
      <div v-for="(item, i) of list" :key="item.id" class="">
        <ItemInfo v-bind="item" :value="data.get(item.id)" @click="setState" />
      </div>
    </div> -->

    <VCardGray title="INPUT">
      <div class="relative">
        <VListObject :items="{ dallas, device }" @click="console.log"></VListObject>
      </div>
    </VCardGray>

    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-2 xl:grid-cols-4 2xl:grid-cols-5 gap-4">
      <VCardGray class="col-span-full" title="Date">
        <div class="flex items-center">
          <input :value="datetime" type="datetime-local" @change="onDate" />
        </div>
      </VCardGray>

      <VCardGray title="INPUT">
        <div class="grid grid-cols-2 lg:grid-cols-3 gap-2">
          <div v-for="item of getListItems('input')" :key="item.id">
            <div class="text-body mb-1 text-gray-600">{{ item.name }}</div>

            <v-button block disabled>{{ getState(item.id) ? 'OFF' : 'ON' }}</v-button>
          </div>
        </div>
      </VCardGray>

      <VCardGray title="OUTPUT">
        <div class="grid grid-cols-2 lg:grid-cols-3 gap-2">
          <div v-for="item of getListItems('output')" :key="item.id">
            <div class="text-body mb-1 text-gray-600">{{ item.name }}</div>

            <v-button block @click="setState(item.id, getState(item.id) ? 0 : 1, 2)">{{ getState(item.id) ? 'OFF' : 'ON' }}</v-button>
          </div>
        </div>
      </VCardGray>

      <VCardGray title="ADC">
        <div class="grid gap-2 grid-cols-2">
          <div v-for="item of getListItems('adc')" :key="item.id">
            <span class="text-body text-gray-600 mr-2">{{ item.name }}:</span>
            <span class="font-bold">{{ getState(item.id) }}</span>
          </div>
        </div>
      </VCardGray>

      <VCardGray v-if="isDallas" title="DS18B20">
        <div class="grid gap-2 grid-cols-1">
          <div v-for="(ds, key) in dallas" :key="`adc_${key}`">
            <span class="text-body text-gray-600 mr-2">{{ key }}:</span>
            <span class="font-bold">{{ ds.temp.toFixed(2) }} ℃</span>
          </div>
        </div>
      </VCardGray>

      <VCardGray title="DAC">
        <div class="grid grid-cols-1 gap-4">
          <div v-for="item of getListItems('dac')" :key="item.id" class="grid gap-2 grid-cols-4">
            <VTextField :modelValue="getState(item.id)" class="col-span-2" :label="item.name" hideMessage @keypress.enter="setState(item.id, $event.target.value, 3)" />

            <!-- <v-button class="col-span-2" block  @click="onDac(i + 1, dac[`dac${i + 1}`])">Send</v-button> -->
          </div>
        </div>
      </VCardGray>

      <!-- <VCardGray>
        <h5 class="mb-6">Electric counter</h5>
        <div class="grid gap-2 grid-cols-1">
          <div v-for="(value, key) in getModbus(modbus)" :key="`modbus_${key}`">
            <span class="text-body text-gray-600 mr-2">{{ key }}:</span>

            <span class="font-bold">{{ value }}</span>
          </div>
        </div>
      </VCardGray> -->
    </div>

    <!-- <div class="mt-4">
      <VCardGray>
        <h5 class="mb-6">Modbus (master)</h5>
        <div class="grid grid-cols-1 gap-4">
          <BlockModbus :data="modbus" @send="onSendModBus" />
        </div>
      </VCardGray>
    </div> -->
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue';
import { useRouter } from 'vue-router';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import { useModule } from '@/composables/useModule.ts';

import event from '@/assets/js/event';

import VListObject from '@/components/general/VListObject.vue';

const router = useRouter();

const webSocketStore = useWebSocketStore();
const { device, dallas } = storeToRefs(webSocketStore);

const listPage = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Save default' },
];

const { getList, getState, setState, onSaveDef, onSend } = useModule();

const datetime = computed(() => new Date((device.value.now || 0) * 1000).toISOString().slice(0, 16));
const isDallas = computed(() => Boolean(Object.values(dallas?.value)?.length));
const now = ref(0);

event.on('init', () => {
  onSend('DEVICE');
});

const onPage = ({ id }) => {
  if (id === 1) router.push('/config');
  if (id === 2) onSaveDef();
};

const getListItems = (find: string) => getList.value?.filter((i) => i.id.includes(find));

const onDate = (e: any) => {
  const _now = e?.target?.valueAsNumber;
  if (_now) now.value = _now / 1000;
  onSend('DEVICE', { now: now.value, command: 1 });
};
</script>
