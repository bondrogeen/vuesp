<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Dashboard</h1>
      <!-- <VDropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <IconMenu @click="on.click"></IconMenu>
        </template>

        <VList :list="listMenu" class="rounded" @click="onUploadFile"></VList>
      </VDropdown> -->
    </div>
    {{ device }}
    <div class="grid">
      <div v-for="item of list" :key="item" @click="onClick(item, getValueItem(item, device) ? 0 : 1)">{{ getValueItem(item, device) }}</div>
    </div>

    <!-- <div class="grid grid-cols-6 gap-4">
      <div class="relative rounded-2xl border border-gray-200 bg-white p-5 dark:border-gray-800 dark:bg-white/[0.03] sm:p-6" :class="{ 'overflow-hidden': loading }">
        <div
          v-if="loading"
          class="loader-line w-full absolute top-0 left-0 h-[3px] bg-gray-100 dark:bg-gray-800 before:bg-blue-600 dark:before:bg-blue-800 before:h-1 rounded-xl z-20 before:absolute before:w-[40%] before:-left-1/2 before:content-[''] before:animate-[lineAnim_1s_linear_infinite]"
        ></div>
        <div v-if="loading" class="absolute top-0 left-0 w-full h-full bg-gray-50 dark:bg-gray-900 z-10 opacity-50"></div>

        <div class="mb-6 flex items-center justify-between">
          <h3 class="text-lg font-semibold text-gray-800 dark:text-white/90">{{ title }}</h3>

          <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="currentColor">
            <path
              d="M6.26489 3.80698L7.41191 5.44558C5.34875 6.89247 4 9.28873 4 12C4 16.4183 7.58172 20 12 20C16.4183 20 20 16.4183 20 12C20 9.28873 18.6512 6.89247 16.5881 5.44558L17.7351 3.80698C20.3141 5.61559 22 8.61091 22 12C22 17.5228 17.5228 22 12 22C6.47715 22 2 17.5228 2 12C2 8.61091 3.68594 5.61559 6.26489 3.80698ZM11 12V2H13V12H11Z"
            ></path>
          </svg>
          <div>
            <slot name="header"></slot>
          </div>
        </div>
      </div>
    </div> -->
  </div>
</template>

<script setup>
import { onMounted, ref, computed, reactive } from 'vue';
import { storeToRefs } from 'pinia';
import { getConfig, saveConfig } from '@/utils/fs/';

import { useWebSocketStore } from '@/stores/WebSocketStore';

import AppDialog from '@/components/app/AppDialog.vue';

const webSocketStore = useWebSocketStore();
const { dallas, device } = storeToRefs(webSocketStore);

const showDialog = ref(false);
const config = ref({});

const listMenu = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Remove' },
];

const getFun = (field) => field?.fun || '(v)=>v*10';

// console.log(eval(text)(15));

const onSave = async () => {
  await saveConfig(config.value);
};
const onClose = () => {
  showDialog.value = false;
};
const getValue = (field, k) => field?.name || k;
const isFunction = (field) => typeof field?.fun !== 'undefined';

const onName = (name, key, value) => (config.value[name][key].name = value);
const onFunction = (name, key, value) => (config.value[name][key].fun = value);

const list = ref([]);
const items = reactive();

let modules = null;

const getList = computed(() => modules?.data?.getList());

const getValueItem = (name, device) => {
  return modules.data.get(name, { device });
};
const onClick = (name, value) => {
  console.log(value);

  device.value.output = modules.data.set(name, { device: device.value }, value)
  webSocketStore.onSend('DEVICE', { ...device.value, command: 2 });
};

onMounted(async () => {
  const object = dallas?.value || {};
  const ds = {};
  for (const key in object) {
    ds[key] = {};
  }

  config.value = await getConfig({ ds });

  const modulesPath = '/';
  const files = ['data.js'];
  modules = await import(`${modulesPath}${files[0]}`);

  list.value = modules.data.getList();
});
</script>
