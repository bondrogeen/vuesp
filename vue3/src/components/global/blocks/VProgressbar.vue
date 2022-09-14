<template>
  <div class="progress">
    <span class="progress-left">
      <span class="progress-bar" :style="styleLeft"></span>
    </span>
    <span class="progress-right">
      <span class="progress-bar" :style="styleRight"></span>
    </span>
    <div class="progress-value text-h4">{{ value }}%</div>
  </div>
</template>

<script setup>
import { computed, defineProps } from 'vue';
const props = defineProps({
  value: { type: Number, default: 50 },
  color: { type: String, default: 'primary' },
  size: { type: String, default: 'normal' },
});

const styleRight = computed(() => {
  const value = props.value;
  let deg = value * 3.6;
  if (deg > 180) deg = 180;
  return { transform: `rotate(${deg}deg)` };
});

const styleLeft = computed(() => {
  const value = (props.value - 50 < 0) ? 0 : props.value - 50;
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
    border: 12px solid #fff;
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
  .progress-left {
    left: 0;
    .progress-bar {
      left: 100%;
      border-top-right-radius: 80px;
      border-bottom-right-radius: 80px;
      border-left: 0;
      -webkit-transform-origin: center left;
      transform-origin: center left;
      // transform: rotate(90deg);
    }
  }
  .progress-right {
    right: 0;
    .progress-bar {
      left: -100%;
      border-top-left-radius: 80px;
      border-bottom-left-radius: 80px;
      border-right: 0;
      -webkit-transform-origin: center right;
      transform-origin: center right;
      // transform: rotate(180deg);
    }
  }

  .progress-bar {
    width: 100%;
    height: 100%;
    background: none;
    border: 12px solid #049dff;
    position: absolute;
    top: 0;
  }

  .progress-value {
    // height: 20px;
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    z-index: 2;
  }
}

// .progress.blue .progress-left .progress-bar {
//   animation: loading-2 1.5s linear forwards 1.8s;
// }

// .progress-left .progress-bar {
//   animation: loading-3 1s linear forwards 1.8s;
// }

// .progress-left .progress-bar {
//   animation: loading-4 0.4s linear forwards 1.8s;
// }

// .progress-left .progress-bar {
//   animation: loading-5 1.2s linear forwards 1.8s;
// }
// @keyframes loading-1 {
//   0% {
//     -webkit-transform: rotate(0deg);
//     transform: rotate(0deg);
//   }
//   100% {
//     -webkit-transform: rotate(180deg);
//     transform: rotate(180deg);
//   }
// }
// @keyframes loading-2 {
//   0% {
//     -webkit-transform: rotate(0deg);
//     transform: rotate(0deg);
//   }
//   100% {
//     -webkit-transform: rotate(180deg);
//     transform: rotate(180deg);
//   }
// }
// @keyframes loading-3 {
//   0% {
//     -webkit-transform: rotate(0deg);
//     transform: rotate(0deg);
//   }
//   100% {
//     -webkit-transform: rotate(90deg);
//     transform: rotate(90deg);
//   }
// }
// @keyframes loading-4 {
//   0% {
//     -webkit-transform: rotate(0deg);
//     transform: rotate(0deg);
//   }
//   100% {
//     -webkit-transform: rotate(180deg);
//     transform: rotate(180deg);
//   }
// }
</style>
