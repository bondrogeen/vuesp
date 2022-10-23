<template>
  <div class="progress">
    <span class="progress-left">
      <span class="progress-bar" :style="styleLeft"></span>
    </span>
    <span class="progress-right">
      <span class="progress-bar" :style="styleRight"></span>
    </span>
    <div class="progress-value text-h4">{{ getValue }}%</div>
  </div>
</template>

<script setup>
import { computed, defineProps } from 'vue';
const props = defineProps({
  value: { type: Number, default: 50 },
  color: { type: String, default: 'primary' },
  size: { type: String, default: 'normal' },
});

const getValue = computed(() => (Math.round(props.value > 100 ? 100 : props.value)));

const styleRight = computed(() => {
  const value = getValue.value;
  let deg = value * 3.6;
  if (deg > 180) deg = 180;
  return { transform: `rotate(${deg}deg)` };
});

const styleLeft = computed(() => {
  const value = getValue.value - 50 < 0 ? 0 : getValue.value - 50;
  let deg = value * 3.6;
  if (deg > 180) deg = 180;
  return { transform: `rotate(${deg}deg)` };
});
</script>

<style lang="scss">
.progress {
  position: relative;
  width: 150px;
  height: 150px;
  background: none;
  margin: 0 auto;
  box-shadow: none;
  &:after {
    content: '';
    width: 100%;
    height: 100%;
    border-radius: 50%;
    border: 12px solid color('app', 'white');
    position: absolute;
    top: 0;
    left: 0;
  }
  & > span {
    width: 50%;
    height: 100%;
    overflow: hidden;
    position: absolute;
    top: 0;
    z-index: 1;
  }
  &-bar {
    width: 100%;
    height: 100%;
    background: none;
    border: 12px solid color('app', 'primary');
    position: absolute;
    top: 0;
  }

  &-value {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    z-index: 2;
  }
  &-left {
    left: 0;
    .progress-bar {
      left: 100%;
      border-top-right-radius: 80px;
      border-bottom-right-radius: 80px;
      border-left: 0;
      -webkit-transform-origin: center left;
      transform-origin: center left;
    }
  }
  &-right {
    right: 0;
    .progress-bar {
      left: -100%;
      border-top-left-radius: 80px;
      border-bottom-left-radius: 80px;
      border-right: 0;
      -webkit-transform-origin: center right;
      transform-origin: center right;
    }
  }
}
</style>
