<template>
  <div class="pt-6">
    <div class="grid grid-cols-1 gap-x-4">
      <div class="grid grid-cols-2 gap-x-4">
        <v-text-field v-model="item.id" label="ID" :disabled="!isNew" @click="emit('select', $event)"></v-text-field>

        <v-text-field v-model="item.keyValue" label="KeyValue"></v-text-field>
      </div>

      <v-text-field v-model="item.name" label="Name"></v-text-field>

      <div class="grid grid-cols-2 gap-x-4">
        <VSelect v-slot="{ item }" :value="item.icon" class="" label="Icon" :list="listIcon" @change="onIcon">
          <div class="flex gap-2 items-center">
            <component :is="getIcon(item.value)" class="size-5"></component>
            <div>{{ item.name }}</div>
          </div>
        </VSelect>

        <VSelect :value="item.type" class="" label="Type" :list="listType" @change="onType"></VSelect>
      </div>

      <div class="grid grid-cols-2 gap-x-4">
        <v-text-field v-if="isMinMax" v-model="item.min" label="Min"></v-text-field>

        <v-text-field v-if="isMinMax" v-model="item.max" label="Max"></v-text-field>
      </div>

      <v-textarea v-if="item.set" v-model="item.set" label="Set"></v-textarea>

      <v-textarea v-model="item.get" label="Get"></v-textarea>

      <v-textarea v-model="item.modifyValue" label="ModifyValue"></v-textarea>
    </div>

    <div class="flex gap-4 justify-end">
      <v-button class="px-4" color="red" outline @click="onRemove">Remove</v-button>
      <v-button class="px-4" color="blue" @click="onSave">Save</v-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import type { Ref } from 'vue';
import type { TypePropertyString } from '@/vuesp-data/types.ts';

import { computed, onMounted, ref } from 'vue';

interface Props {
  item: TypePropertyString;
  isNew: boolean;
}

const { item: data, isNew } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'remove', value: TypePropertyString): void;
  (e: 'save', value: TypePropertyString): void;
  (e: 'select', event: Event): void;
}>();

const item: Ref<TypePropertyString> = ref({ id: '', name: '', keyValue: '' });

interface TypeList {
  value: string;
  name: string;
}

const listIcon: TypeList[] = [
  { name: 'Air', value: 'air' },
  { name: 'Bulb', value: 'bulb' },
  { name: 'Cold', value: 'cold' },
  { name: 'Camera', value: 'camera' },
  { name: 'Vent', value: 'vent' },
  { name: 'Garage', value: 'garage' },
  { name: 'Video', value: 'video' },
  { name: 'Light', value: 'light' },
  { name: 'Tv', value: 'tv' },
  { name: 'Therm', value: 'therm' },
  { name: 'Store', value: 'store' },
  { name: 'Socket', value: 'socket' },
  { name: 'Search', value: 'search' },
  { name: 'Save', value: 'save' },
  { name: 'Noti', value: 'noti' },
  { name: 'Logout', value: 'logout' },
  { name: 'Lock', value: 'lock' },
  { name: 'Folder', value: 'folder' },
];

const listType: TypeList[] = [
  { name: 'Button', value: 'button' },
  { name: 'Info', value: 'info' },
  { name: 'Dimmer', value: 'dimmer' },
  { name: 'Switch', value: 'switch' },
  { name: 'Date', value: 'date' },
  { name: 'Input', value: 'input' },
];

const getIcon = (icon: string) => `icon-${icon}`;

const isMinMax = computed(() => item.value.type === 'dimmer');

const onIcon = ({ value }: any) => {
  item.value.icon = value;
};
const onType = ({ value }: any) => {
  item.value.type = value;
};

const onSave = () => {
  emit('save', item.value);
};
const onRemove = () => {
  emit('remove', item.value);
};

onMounted(() => {
  console.log(data);

  item.value = { ...data };
});
</script>
