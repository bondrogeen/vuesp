<template>
  <div :class="['v-loader', `v-loader--${color}`]">
    <div v-for="i of 4" :key="'v-loader_' + i"></div>
  </div>
</template>
<script setup>
import { defineProps } from 'vue';
defineProps({
  color: { type: String, default: 'primary' },
});
</script>

<style lang="scss">
.v-loader {
  position: absolute;
  display: inline-block;
  top: 50%;
  left: 50%;
  width: 23px;
  height: 23px;
  transform: translate(-50%, -50%);
  &--white {
    div {
      border-color: $white transparent transparent transparent !important;
    }
  }
  &--primary {
    div {
      border-color: $primary transparent transparent transparent !important;
    }
  }
  div {
    box-sizing: border-box;
    display: block;
    position: absolute;
    width: 100%;
    height: 100%;
    border: 3px solid;
    border-radius: 50%;
    animation: lds-ring 1.2s cubic-bezier(0.5, 0, 0.5, 1) infinite;
    &:nth-child(1) {
      animation-delay: -0.45s;
    }
    &:nth-child(2) {
      animation-delay: -0.3s;
    }
    &:nth-child(3) {
      animation-delay: -0.15s;
    }
  }
}
@keyframes lds-ring {
  0% {
    transform: rotate(0deg);
  }
  100% {
    transform: rotate(360deg);
  }
}
</style>
