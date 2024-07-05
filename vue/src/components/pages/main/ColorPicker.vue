<template>
  <div class="color-picker">
    <div class="color-picker__main">
      <input class="color-picker__input color-picker__input--color" type="color" list="presetColors" :value="valueColor" @input="onChange('color', $event)" />
      <input class="color-picker__input color-picker__input--fill" type="color" list="presetColors" :value="valueFill" @input="onChange('fill', $event)" />
      <datalist id="presetColors">
        <option v-for="item of colors" :key="item" :value="getColor(item)" />
      </datalist>
    </div>
    <button class="color-picker__reverse" @click="onReverse">
      <v-icons icon="reverse"></v-icons>
    </button>
  </div>
</template>

<script setup>
import { defineProps, defineEmits, ref } from 'vue';

import { hexToRGBA } from '@/utils/helpers';

const props = defineProps({
  color: { type: String, default: '#ffffff' },
  fill: { type: String, default: '#000000' },
  colors: { type: Array, default: () => [] },
});

const emit = defineEmits(['color', 'close', 'fill']);

const valueColor = ref(props.color);
const valueFill = ref(props.fill);

const getColor = color =>
  `#${color
    .map(i => i.toString(16))
    .map(i => (i.length ? `0${i}` : i))
    .join('')}`;

const onChange = (type, e) => {
  const value = e.target.value;
  const color = hexToRGBA(value);
  if (type === 'color') valueColor.value = value;
  if (type === 'fill') valueFill.value = value;
  emit(type, color);
};
const onReverse = e => {
  console.log(e);
  const temp = valueColor.value;
  valueColor.value = valueFill.value;
  valueFill.value = temp;
  emit('color', hexToRGBA(valueColor.value));
  emit('fill', hexToRGBA(valueFill.value));
};
</script>

<style lang="scss">
.color-picker {
  display: flex;
  padding: 4px;

  &__reverse {
    width: 15px;
    height: 15px;
    position: relative;
    top: -5px;
    right: -10px;
  }
  &__colors {
    display: grid;
    grid-template-columns: repeat(6, 1fr);
  }

  &__item {
    height: 15px;
    width: 15px;
    cursor: pointer;
  }

  &__main {
    position: relative;
    display: block;
  }

  &__input {
    &::-webkit-color-swatch-wrapper {
      padding: 0;
    }
    cursor: pointer;
    overflow: hidden;
    height: 20px;
    width: 20px;
    outline: none;
    border: none;
    display: flex;
    padding: 0;
    margin: 0;

    &--color {
      top: 0;
      left: 0;
    }

    &--fill {
      position: absolute;
      top: 10px;
      left: 10px;
    }
  }
}
</style>
