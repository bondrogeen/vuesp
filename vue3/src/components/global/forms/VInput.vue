<template>
  <div class="v-text-field" :class="getClass">
    <div class="v-text-field__slot">
      <span class="v-text-field__label">
        {{ label }}
      </span>
      <input
        v-bind="$attrs"
        :value="value"
        :disabled="disabled"
        class="v-text-field__input"
        type="text"
        @focus="onFocus"
        @blur="onBlur"
        @input="onInput"
      />
      <div class="v-text-field__icon">
        <slot name="icon"></slot>
      </div>
    </div>
    <div class="v-text-field__message">
      <slot name="message">
        {{ isDirty ? error : '' }}
      </slot>
    </div>
  </div>
</template>
<script setup>
import { ref, defineProps, defineEmits, computed } from 'vue';
const emit = defineEmits(['update:modelValue']);
const props = defineProps({
  modelValue: { type: String, default: '' },
  label: { type: String, default: '' },
  disabled: { type: Boolean, default: false },
  rules: { type: Array, default: () => [] },
});

const value = ref(props.modelValue);
const error = ref('');
const isError = computed(() => isDirty.value && error.value);
const isDirty = ref(false);
const isFocus = ref(false);

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
  value.value = target.value;
  onRules(target.value);
};

const onBlur = () => {
  isFocus.value = false;
  isDirty.value = true;
};

const onFocus = () => (isFocus.value = true);

const getClass = computed(() => [
  { 'v-text-field--disabled': props.disabled },
  { 'v-text-field--focus': isFocus.value || value.value },
  { 'v-text-field--error': isError.value },
]);
</script>

<style lang="scss">
.v-text-field {
  position: relative;
  box-sizing: border-box;
  &__slot {
    position: relative;
    height: 50px;
    width: 100%;
    border: 1px solid color('app', 'secondary');
    border-radius: 4px;
    display: flex;
    align-items: center;
  }
  &__message {
    height: 10px;
    font-size: 14px;
    padding: 0 15px;
    color: color('red', 'base');
  }
  &__label {
    position: absolute;
    transform: translate(0, -50%);
    color: color('grey', 'base');
    line-height: 10px;
    top: 50%;
    left: 15px;
    transition: all 0.2s ease-in-out;
    background-color: white;
  }
  &__input {
    border: 0;
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
    &::placeholder {
      color: color('grey', 'darken-3');
    }
  }
  &:hover:not(.v-text-field--disabled) {
    @media (hover: hover) {
      border-color: color('app', 'primary');
    }
  }
  &__icon {
    height: 100%;
    flex: 0 0 40px;
    display: flex;
    align-items: center;
    justify-content: center;
  }
  &--disabled {
    .v-text-field__slot {
      border: 1px solid color('grey', 'base');
      background-color: color('grey', 'lighten-1');
      opacity: 0.5;
    }
  }
  &--error {
    .v-text-field__slot {
      border: 1px solid color('red', 'base');
    }
  }
  &--focus {
    .v-text-field__label {
      top: -1px;
      font-size: 14px;
      padding: 0 4px;
    }
  }
}
</style>
