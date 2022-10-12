<template>
  <v-dropdown class="v-select" v-bind="$attrs">
    <template #activator="{ on }">
      <v-input :key="value" :model-value="value" readonly active hide-message :label="label" @click="on.click" @on-icon="on.click">
        <template #icon>
          <v-icons icon="select"></v-icons>
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
