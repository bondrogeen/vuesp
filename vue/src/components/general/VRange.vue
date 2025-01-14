<template>
  <div class="">
    <input :value="modelValue" type="range" :min="min" :max="max" class="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer dark:bg-gray-700" @change="onChange" />
  </div>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue';

const props = defineProps({
  modelValue: { type: [String, Number], default: null },
  min: { type: [String, Number], default: null },
  max: { type: [String, Number], default: null },

  disabled: { type: Boolean, default: false },
});

const emit = defineEmits(['update:modelValue']);

const onChange = ({ target }) => {
  emit('update:modelValue', target.value);
};
</script>
