<template>
  <label class="b-checkbox">
    <input :checked="value" class="b-checkbox__input" type="checkbox" @change="onChange" />
    <span class="b-checkbox__mark"></span>
    <slot></slot>
  </label>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue';
const emit = defineEmits(['input']);
defineProps({
  value: { type: Boolean, default: false },
});
const onChange = ({ target }) => {
  emit('input', target.checked);
};
</script>

<style lang="scss">
$borderRadius: 4px;
$size: 16px;
.b-checkbox {
  position: relative;
  display: flex;
  align-items: center;
  &__input {
    position: absolute;
    opacity: 0;
    cursor: pointer;
    height: 0;
    width: 0;
  }

  &__mark {
    position: relative;
    display: block;
    height: $size;
    width: $size;
    border: 1px solid map-get($grey, 'base');
    border-radius: $borderRadius;
    margin-right: 5px;
    &::after {
      content: '';
      display: block;
      position: absolute;
      top: 45%;
      left: 50%;
      height: calc(60%);
      width: calc(30%);
      transform: translate(-50%, -50%) rotate(43deg);
      border-right: 2px solid $white;
      border-bottom: 2px solid $white;
    }
  }
  &__input:checked ~ &__mark {
    background-color: $primary;
  }
}
</style>
