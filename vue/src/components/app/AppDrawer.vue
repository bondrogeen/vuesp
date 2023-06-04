<template>
  <div class="app-drawer" :class="[{ 'app-drawer--open': value }, { 'app-drawer--fixed': position }]">
    <div class="app-drawer__content">
      <div class="app-drawer__header">
        <div class="app-drawer__theme" @click="changeTheme">
          <v-icons :icon="!theme ? 'dark' : 'light'" />
        </div>
        <div @click="onClose">
          <router-link to="/">
            <v-icons icon="logo"></v-icons>
          </router-link>
        </div>
        <div class="app-drawer__close" @click="onClose">
          <v-icons icon="close"></v-icons>
        </div>
      </div>
      <div class="app-drawer__body">
        <slot></slot>
      </div>
    </div>
  </div>
</template>

<script setup>
import { defineProps, defineEmits, inject } from 'vue';
const props = defineProps({
  value: { type: Boolean, default: false },
  position: { type: Boolean, default: true },
  changeTheme: { type: Function, default: () => {} },
});

const emit = defineEmits(['close']);
const theme = inject('theme');
const onClose = e => {
  if (props.value) {
    emit('close', e);
  }
};
</script>

<style lang="scss">
.app-drawer {
  position: absolute;
  left: 0;
  top: 0;
  height: 100%;
  z-index: 101;
  transition: all 0.2s ease-in-out;
  overflow: hidden;
  background-color: var(--bg-1);
  box-shadow: 0px 8px 35px -2px var(--shadow-1);
  transform: translateX(-100%);
  width: 100%;
  @include above($sm) {
    transform: translateX(-100%);
    width: 360px;
  }

  &--fixed {
    position: fixed;
    height: 100%;
  }
  &--open {
    transform: translateX(0);
  }
  @include above($md) {
    transform: translateX(-100%);
  }
  &__header {
    height: 60px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    background-color: var(--bg-2);
    box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.1);
    padding: 0 15px;
    svg {
      height: 30px;
    }
  }
  &__content {
    min-width: 360px;
    background-color: var(--bg-1);
    position: absolute;
    width: 100%;
    height: 100%;
    overflow: auto;
  }
  &__body {
    height: calc(100% - 60px);
    padding: 15px;
  }
  &__theme,
  &__close {
    cursor: pointer;
    svg {
      height: 24px;
    }
  }
}
</style>
