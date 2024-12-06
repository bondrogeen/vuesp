<template>
  <div class="v-text-field w-full relative" :class="getClass">
    <div class="v-text-field__slot relative w-full h-[40px] border flex items-center rounded transition" :class="getClassSlot">
      <span class="v-text-field__label absolute bg-white text-gray-600">
        {{ label }}
      </span>
      <input
        v-bind="$attrs"
        :value="modelValue"
        :disabled="disabled"
        :type="type"
        class="v-text-field__input"
        @focus="onFocus"
        @blur="onBlur"
        @input="onInput"
        @click="onClick"
        @keypress.enter="onEnter"
      />
      <button v-if="$slots.icon" :disabled="disabled" style="flex: 0 0 50px" class="h-full flex items-center justify-center text-gray-400 border-l border-primary" @click="onIcon">
        <slot name="icon"></slot>
      </button>
    </div>
    <div v-if="!hideMessage" class="text-red-500 h-[24px] px-2 text-small">
      <slot name="message">
        {{ isDirty ? error : '' }}
      </slot>
    </div>
  </div>
</template>

<script setup>
import { ref, defineProps, defineEmits, computed } from 'vue';
const props = defineProps({
  modelValue: { type: [String, Number], default: '' },
  label: { type: String, default: '' },
  disabled: { type: Boolean, default: false },
  type: { type: String, default: 'text' },
  active: { type: Boolean, default: false },
  hideMessage: { type: Boolean, default: false },
  rules: { type: Array, default: () => [] },
});

const emit = defineEmits(['update:modelValue', 'click', 'on-icon', 'enter']);

// const value = ref(props.modelValue);
const error = ref('');
const isError = computed(() => isDirty.value && error.value);
const isDirty = ref(false);
const isFocus = ref(false);

const getClassSlot = computed(() => [isError.value ? 'border-red-500' : 'hover:border-secondary', props.disabled ? 'border-gray-400 opacity-50 hover:border-gray-400' : 'border-primary']);

const getClass = computed(() => [{ 'v-text-field--focus': isFocus.value || typeof props.modelValue !== 'undefined' || props.active }]);

const onRules = value => {
  const errors = props.rules
    .map(rule => {
      const valid = rule(value);
      return valid === false || typeof valid === 'string' ? valid || '' : '';
    })
    .filter(i => i);
  error.value = errors.length ? errors[0] : '';
};

const onInput = ({ target }) => {
  emit('update:modelValue', target.value);
  // value.value = target.value;
  onRules(target.value);
};

const onBlur = () => {
  isFocus.value = false;
  isDirty.value = true;
};
const onEnter = e => emit('enter', e);

const onFocus = () => (isFocus.value = true);
const onClick = e => emit('click', e);
const onIcon = e => {
  if (props.disabled) return;
  emit('on-icon', e);
};
</script>

<style lang="scss">
.v-text-field {
  &__label {
    transform: translate(0, -50%);
    top: 50%;
    left: 15px;
  }
  &__input {
    height: 100%;
    padding: 0 15px;
    font-size: $font-size-root;
    width: calc(100% - 20px);
    text-overflow: ellipsis;
    white-space: nowrap;
    overflow: hidden;
    flex: 1 1 auto;
    background: inherit;
    outline: none !important;
    color: var(--text-1);
    &::placeholder {
      color: var(--border-1);
    }
  }

  &--focus {
    .v-text-field__label {
      top: -2px;
      font-size: 14px;
      padding: 0 4px;
    }
  }
}
</style>
