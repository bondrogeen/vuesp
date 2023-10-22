<template>
  <div>
    <v-expansion label="Device" value>
      <div class="row">
        <div class="col sm12 md6">
          <v-select :value="getSpeed" label="Speed" :list="listSpeed" @change="onSpeed"></v-select>
        </div>
        <div class="col sm12"></div>
        <div class="col sm12 md6">
          <v-select :value="getEffect" label="Effect" :list="listEffect" @change="onEffect"></v-select>
        </div>
        <div class="col sm12"></div>
        <div class="col sm12 md6">
          <v-select :value="getBrightness" label="Brightness" :list="listBrightness" @change="onBrightness"></v-select>
        </div>
        <div class="col sm12"></div>
      </div>
    </v-expansion>

    <div class="row mt-6">
      <div class="col sm12 d-flex j-end">
        <v-button @click="onSave(true)">Save</v-button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, defineProps, defineEmits } from 'vue';

const props = defineProps({
  modelValue: { type: Object, default: () => ({}) },
});

const emit = defineEmits(['update:modelValue', 'scan', 'save']);

const device = computed({
  set: value => emit('update:modelValue', value),
  get: () => props.modelValue,
});

const listSpeed = [
  { name: '20', value: 20 },
  { name: '40', value: 40 },
  { name: '60', value: 60 },
  { name: '80', value: 80 },
  { name: '100', value: 100 },
];

const listEffect = [
  { name: 'EFFECT_DRAW', value: 0 },
  { name: 'EFFECT_LIGHTERS', value: 1 },
  { name: 'EFFECT_FIRE', value: 2 },
];

const listBrightness = [
  { name: '0', value: 1 },
  { name: '10', value: 25 },
  { name: '20', value: 50 },
  { name: '30', value: 75 },
  { name: '40', value: 100 },
  { name: '50', value: 128 },
  { name: '60', value: 150 },
  { name: '70', value: 175 },
  { name: '80', value: 200 },
  { name: '90', value: 225 },
  { name: '100', value: 255 },
];

const getSpeed = computed(() => listSpeed.find(i => i.value === device.value.speed)?.name || '');
const getEffect = computed(() => listEffect.find(i => i.value === device.value.effect)?.name || '');
const getBrightness = computed(() => listBrightness.find(i => i.value === device.value.brightness)?.name || '');

const onSave = isSave => {
  device.value.command = isSave ? 4 : 1;
  emit('save', device.value);
};

const onSpeed = ({ value }) => {
  device.value.speed = value;
  onSave();
};
const onEffect = ({ value }) => {
  device.value.effect = value;
  onSave();
};
const onBrightness = ({ value }) => {
  device.value.brightness = value;
  onSave();
};
</script>
