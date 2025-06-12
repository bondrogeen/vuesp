<template>
  <div class="container mx-auto">
    <div class="mb-6 flex items-center justify-between">
      <h1>Dashboard</h1>

      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-dots class="rotate-90"></icon-dots>
          </v-button>
        </template>

        <!-- <v-list :list="listMenu" @click="onSave"></v-list> -->
      </v-dropdown>
    </div>

    {{ item }}
    <div class="grid grid-cols-[repeat(auto-fill,_minmax(100px,_1fr))] gap-4">
      <div v-for="(item, i) of list" :key="item.id" class="" @click="onDialog(i)">
        <ItemInfo v-bind="item" :value="getValueItem(item.id, device)" />
      </div>
    </div>

    <AppDialog v-if="dialogItem" size="lg" title="Config" @close="onClose">
      <div>
        <VTextField v-model="item.id" label="id" disabled></VTextField>
        <VTextField v-model="item.name" label="name"></VTextField>
        <VTextField v-model="item.key" label="key"></VTextField>
        <VTextField v-if="item.set" v-model="item.set" label="set"></VTextField>
        <VTextField v-model="item.get" label="get"></VTextField>
      </div>

      <template #footer>
        <v-button class="px-8" color="blue" @click="onSave">Save</v-button>
      </template>
    </AppDialog>
  </div>
</template>

<script setup>
import { onMounted, ref, computed } from 'vue';
import { storeToRefs } from 'pinia';
import { loadModule, saveModule } from '@/utils/fs.ts';
import { stringToFunction } from '@/utils/func.ts';
import { VuespData } from '@/utils/VuespData.ts';
import { pathListDef, pathList } from '@/utils/const.ts';

import { useWebSocketStore } from '@/stores/WebSocketStore.ts';

import AppDialog from '@/components/app/AppDialog.vue';
import ItemInfo from '@/components/dashboard/ItemInfo.vue';

const webSocketStore = useWebSocketStore();
const { dallas, device } = storeToRefs(webSocketStore);

const showDialog = ref(false);
const config = ref({});
const data = ref({});

const listMenu = [
  { id: 1, name: 'Config' },
  { id: 2, name: 'Remove' },
];

// const onSave = async () => {
//   await saveModule(config.value);
// };

const onClose = () => {
  dialogItem.value = false;
};

const dialogItem = ref(false);
const list = ref([]);
const item = ref({});

const getList = computed(() => data.value?.getList?.());

const getValueItem = (id, device) => {
  return data.value.get(id, { device });
};
const setState = ({ id, value }) => {
  device.value.output = data.value.set(id, { device: device.value }, value ? 0 : 1);
  webSocketStore.onSend('DEVICE', { ...device.value, command: 2 });
};

const onDialog = (i) => {
  const { set, get, ...arg } = list.value[i];
  item.value = { ...arg, set: set.toString(), get: get.toString() };

  dialogItem.value = true;
};

const onSave = () => {
  list.value = list.value.map((i) => (i.id === item.value.id ? stringToFunction(item.value) : i));
  dialogItem.value = false;

  data.value = new VuespData(list.value);
  saveModule(pathList, list.value);
};

onMounted(async () => {
  const object = dallas?.value || {};
  const ds = {};
  for (const key in object) {
    ds[key] = {};
  }

  let module;

  try {
    module = await loadModule(`/fs?file=${pathList}`);
    console.log(module);
  } catch (error) {
    console.log(error);
    try {
      module = await loadModule(pathListDef);
      console.log(module);
    } catch (error) {
      console.log(error);
    }
  }

  list.value = module.list;
  data.value = new VuespData(module.list);
});
</script>
