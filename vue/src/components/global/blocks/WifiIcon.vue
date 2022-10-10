<template>
  <div class="wifi-icon">
    <div
      v-for="(item, i) of ['four', 'three', 'two', 'one']"
      :key="item"
      :class="['wifi-icon__item', `wifi-icon__item--${item}`, { 'wifi-icon__item--active': isActive(i) }]"
      :title="`rssi: ${rssi} channel: ${channel}`"
    ></div>
    <div class="wifi-icon__channel">{{ channel }}</div>
  </div>
</template>

<script setup>
import { defineProps } from 'vue';

const props = defineProps({
  rssi: { type: Number, default: 0 },
  channel: { type: Number, default: 0 },
});
const quality = 2 * (props.rssi + 100);
const isActive = i => (4 - i) * 15 < quality;
</script>

<style lang="scss">
.wifi-icon {
  width: 40px;
  display: flex;
  flex-direction: column;
  align-items: center;
  position: relative;
  &__channel {
    position: absolute;
    right: 3px;
    bottom: -5px;
    font-size: 10px;
    font-weight: 700;
    line-height: 10px;
  }
  &__item {
    height: 3px;
    background-color: color('grey', 'lighten-1');
    margin-bottom: 2px;
    border-radius: 2px;
    &--four {
      width: 25px;
    }
    &--three {
      width: 18px;
    }
    &--two {
      width: 12px;
    }
    &--one {
      width: 6px;
    }
    &--active {
      background-color: var(--text-1);
    }
  }
}
</style>
