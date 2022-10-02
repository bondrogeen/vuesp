<template>
  <v-dropdown class="v-select" v-bind="$attrs">
    <template #activator="{ on }">
      <v-input :key="value" :modelValue="value" readonly active hideMessage :label="label" @click="on.click" @on-icon="on.click">
        <template #icon>
          <svg width="10" height="6" viewBox="0 0 10 6" fill="none" xmlns="http://www.w3.org/2000/svg">
            <path d="M1 1L5 5L9 1" stroke="#ABB5BE" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
          </svg>
        </template>
      </v-input>
    </template>
    <v-list v-slot="{ item }" :list="list" @click="onChange">
      <slot :item="item">{{ item.name }}</slot>
    </v-list>
  </v-dropdown>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue';

defineProps({
  value: { type: String, default: '' },
  modelValue: { type: [String, Number], default: '' },
  valueName: { type: String, default: 'name' },
  label: { type: String, default: '' },
  list: { type: Array, default: () => [] },
});

const emit = defineEmits(['change']);

const onChange = e => emit('change', e);
</script>

<style lang="scss">
.v-select {
  position: relative;
  width: 100%;
  margin-bottom: 30px;
}
</style>
