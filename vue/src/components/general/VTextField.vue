<template>
  <div class="w-full relative">
    <label class="relative w-full h-[40px] border flex items-center rounded transition" :class="getClassSlot">
      <span class="absolute left-4 transition-all duration-300 -translate-y-1/2 bg-white text-gray-600" :class="getClassLabel">
        {{ label }}
      </span>

      <input
        v-bind="$attrs"
        :value="modelValue"
        :disabled="disabled"
        :type="type"
        class="w-full px-4 h-100 outline-none overflow-ellipsis flex-auto overflow-hidden w-[calc(100% - 20px)]"
        @focus="onFocus"
        @blur="onBlur"
        @input="onInput"
        @click="onClick"
        @keypress.enter="onEnter"
      />

      <button v-if="$slots.icon" :disabled="disabled" style="flex: 0 0 50px" class="h-full flex items-center justify-center text-gray-400 border-l border-primary" @click="onIcon">
        <slot name="icon"></slot>
      </button>
    </label>

    <div v-if="!hideMessage && message" class="text-red-500 h-[24px] px-2 text-small">
      <slot name="message">
        {{ message }}
      </slot>
    </div>
  </div>
</template>

<script setup>
import { ref, defineProps, defineEmits, computed } from 'vue';

const props = defineProps({
  modelValue: { type: [String, Number], default: null },
  label: { type: String, default: '' },
  disabled: { type: Boolean, default: false },
  type: { type: String, default: 'text' },
  active: { type: Boolean, default: false },
  hideMessage: { type: Boolean, default: false },
  rules: { type: Array, default: () => [] },
  message: { type: String, default: '' },
});

const emit = defineEmits(['update:modelValue', 'click', 'on-icon', 'enter', 'blur']);

const isFocus = ref(false);

const getClassSlot = computed(() => [
  props.message ? 'border-red-500 hover:border-red-400' : '',
  props.disabled ? 'border-gray-400 opacity-50 hover:border-gray-400' : 'border-primary hover:border-secondary',
]);

const isValue = value => value || value === 0;

const getClassLabel = computed(() => [isFocus.value || isValue(props.modelValue) || props.active ? '-top-1 left-1 text-small px-1 py-0' : 'top-1/2']);

const onInput = ({ target }) => {
  emit('update:modelValue', target.value);
};

const onBlur = e => {
  isFocus.value = false;
  emit('blur', e);
};
const onEnter = e => emit('enter', e);

const onFocus = () => (isFocus.value = true);
const onClick = e => emit('click', e);
const onIcon = e => {
  if (props.disabled) return;
  emit('on-icon', e);
};
</script>
