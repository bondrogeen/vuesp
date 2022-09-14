<template>
  <transition name="fade">
    <div v-if="value" class="app-dialog">
      <div class="app-dialog__overlay" @click="onClose"></div>
      <div class="app-dialog__card">
        <div class="app-dialog__header text-h6">
          <slot name="header">Attention !</slot>
        </div>
        <div class="app-dialog__body">
          <slot></slot>
        </div>
        <div class="app-dialog__footer">
          <slot name="footer">
            <v-button @click="onClose">OK</v-button>
          </slot>
        </div>
      </div>
    </div>
  </transition>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue';
const emit = defineEmits(['close']);
defineProps({
  value: { type: Boolean, default: false },
  content: { type: Boolean, default: true },
});

const onClose = e => emit('close', e);
</script>

<style lang="scss">
.app-dialog {
  position: fixed;
  top: 0;
  left: 0;
  height: 100%;
  width: 100%;
  //   background-color: #5c575740;
  z-index: 5;
  &__overlay {
    position: absolute;
    background-color: #5c575740;
    left: 0;
    top: 0;
    width: 100%;
    height: 100%;
  }
  &__card {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    min-height: 200px;
    min-width: 280px;
    border: 1px solid #dee2e6;
    border-radius: 4px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    background-color: white;
  }
  &__header {
    padding: 10px 15px;
    border-bottom: 1px solid #dee2e6;
  }
  &__body {
    flex: 1 1 auto;
    padding: 10px 15px;
  }
  &__footer {
    text-align: end;
    padding: 10px 15px;
    flex: 0 0 40px;
  }
}
</style>
