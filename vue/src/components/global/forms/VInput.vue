<template>
  <div class="v-text-field" :class="getClass">
    <div class="v-text-field__slot">
      <span class="v-text-field__label">
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
      <div v-if="$slots.icon" class="v-text-field__icon grey-base" @click="onIcon">
        <slot name="icon"></slot>
      </div>
    </div>
    <div v-if="!hideMessage" class="v-text-field__message">
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

const getClass = computed(() => [
  { 'v-text-field--disabled': props.disabled },
  { 'v-text-field--focus': isFocus.value || typeof props.modelValue !== 'undefined' || props.active },
  { 'v-text-field--error': isError.value },
]);

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
  position: relative;
  box-sizing: border-box;
  width: 100%;
  &__slot {
    position: relative;
    height: 40px;
    width: 100%;
    border: 1px solid color('app', 'secondary');
    border-radius: 4px;
    display: flex;
    align-items: center;
    background-color: var(--list-bg);
  }
  &__message {
    height: 30px;
    font-size: 14px;
    padding: 0 15px;
    color: color('red', 'base');
  }
  &__label {
    position: absolute;
    transform: translate(0, -50%);
    color: var(--label-1);
    line-height: 10px;
    top: 50%;
    left: 15px;
    transition: all 0.2s ease-in-out;
    background-color: var(--bg-1);
  }
  &__input {
    border: 0;
    border-radius: 4px;
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
  &:hover:not(.v-text-field--disabled) {
    @media (hover: hover) {
      border-color: color('app', 'primary');
    }
  }
  &__icon {
    height: 100%;
    flex: 0 0 50px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-left: 1px solid color('app', 'primary');
    cursor: pointer;
  }
  &--disabled {
    .v-text-field__icon {
      cursor: default;
    }
    .v-text-field__slot {
      border: 1px solid var(--border-1);
      opacity: 0.4;
    }
  }
  &--error {
    .v-text-field__slot {
      border: 1px solid color('red', 'base');
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
