<template>
  <div v-outside="outside" class="v-dropdown" :class="{ 'v-dropdown--opened': isShown }">
    <div class="v-dropdown__activator">
      <slot name="activator" :on="{ click: onClick }" :show="isShown"></slot>
    </div>
    <transition name="slide-up">
      <div v-if="isShown" class="v-dropdown__content" :style="getStyle" @click="onClick">
        <slot :show="onShow" :hide="hide" :is-shown="isShown"></slot>
      </div>
    </transition>
  </div>
</template>

<script setup>
import { ref, computed, defineProps, defineEmits } from 'vue';

const props = defineProps({
  top: { type: String, default: 'calc(100% + 5px)' },
  left: { type: String, default: '0' },
  right: { type: String, default: '' },
  hideOnClick: { type: Boolean, default: true },
  height: { type: String, default: '200px' },
});

const emit = defineEmits(['click', 'show', 'close']);

const isShown = ref(false);
const getStyle = computed(() => ({ top: props.top, left: props.left, right: props.right, 'max-height': props.height }));

const outside = () => {
  if (isShown.value) hide();
};
const onClick = () => {
  if (!isShown.value) onShow();
  else if (props.hideOnClick) hide();
  emit('click', isShown.value);
};
const onShow = e => {
  isShown.value = true;
  emit('show', e);
};
const hide = e => {
  isShown.value = false;
  emit('close', e);
};
</script>

<style lang="scss">
.v-dropdown {
  position: relative;
  &__activator {
    display: flex;
    align-items: center;
    cursor: pointer;
  }
  &__content {
    position: absolute;
    overflow: auto;
    top: calc(100% + 5px);
    box-shadow: 0 6px 12px rgb(0 0 0 / 18%);
    border: none;
    border-radius: 0;
    min-width: 100%;
    z-index: 2;
    transition: 0.3s cubic-bezier(0.25, 0.8, 0.5, 1);
    background-color: var(--bg-1);
  }
  &--opened {
    .b-dropdown__activator::before {
      transform: rotate(180deg);
    }
  }
}
</style>
