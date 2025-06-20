<template>
  <div>
    <div class="grid grid-cols-1 gap-x-4">
      <v-text-field v-model="item.id" label="id" :disabled="!isNew" @click="emit('select', $event)"></v-text-field>
      <v-text-field v-model="item.key" label="key"></v-text-field>
      <v-text-field v-model="item.name" label="name"></v-text-field>
      <VSelect :value="item.icon" class="" label="icon" :list="listIcon" @change="onIcon"></VSelect>
      <VSelect :value="item.type" class="" label="type" :list="listType" @change="onType"></VSelect>
      <v-text-field v-model="item.minMax" label="minMax"></v-text-field>
      <v-text-field v-model="item.modifyValue" label="modifyValue"></v-text-field>
      <v-text-field v-if="item.set" v-model="item.set" label="set"></v-text-field>
      <v-text-field v-model="item.get" label="get"></v-text-field>
    </div>

    <div class="flex gap-4 justify-end">
      <v-button class="px-4" color="red" @click="onRemove">Remove</v-button>
      <v-button class="px-4" color="gry" @click="onSave">Cancel</v-button>
      <v-button class="px-4" color="blue" @click="onSave">Save</v-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import type { Ref } from 'vue';
import type { TypePropertyString } from '@/vuesp-data/types.ts';

import { onMounted, ref } from 'vue';

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

const item: Ref<TypePropertyString> = ref({ id: '', name: '', key: '' });

interface TypeList {
  value: string;
  name: string;
}

const listIcon: TypeList[] = [
  { name: 'Bulb', value: 'bulb' },
  { name: 'Light', value: 'light' },
  { name: 'Vent', value: 'vent' },
];

const listType: TypeList[] = [
  { name: 'Button', value: 'button' },
  { name: 'Info', value: 'info' },
  { name: 'Dimmer', value: 'dimmer' },
  { name: 'Switch', value: 'switch' },
];

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
