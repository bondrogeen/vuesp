<template>
  <div class="service-gpio" @click="onSend">
    <div class="d-flex">
      <div>Name</div>
      <div>State</div>
      <div>Mode</div>
    </div>
    <div v-for="(pin, name) in pins" :key="pin">
      <div class="d-flex">
        <div class="mb-2 mr-2">{{ name }}</div>
        <div>{{ getBit(pin,16) }}</div>
      </div>
    </div>
    {{ gpio }}
    {{ title }}
  </div>
</template>

<script setup>
import { defineProps, defineEmits, computed } from 'vue';

const props = defineProps({
  pins: { type: Object, default: () => ({}) },
});

const emit = defineEmits(['click']);

const getBit = (byte, mask) => byte & mask;

const gpio = computed(() => {
  const arr = Object.entries(props.pins).filter(i => i?.[0] !== 'key');
  return arr.reduce((acc, item) => {
    acc[item[0]] = item[1];
    return acc;
  }, {});
});

const title = computed(() => props.name || '');

const onSend = event => {
  emit('click', event);
};
</script>

<style lang="scss">
.service-gpio {
  position: relative;
}
</style>
