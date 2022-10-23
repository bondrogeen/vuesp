<template>
  <div class="app-drawer" :class="[{ 'app-drawer--open': value }, { 'app-drawer--fixed': position }]">
    <div v-if="value" class="app-drawer__overlay" @click="onClose"></div>
    <div class="app-drawer__content">
      <div class="app-drawer__header">
        <div @click="changeTheme">
          <v-icons :icon="!theme ? 'dark' : 'light'" />
        </div>
        <div @click="onClose">
          <router-link to="/">
            <v-icons icon="logo"></v-icons>
          </router-link>
        </div>
        <div @click="onClose">
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
defineProps({
  value: { type: Boolean, default: false },
  position: { type: Boolean, default: true },
  changeTheme: { type: Function, default: () => {} },
});

const emit = defineEmits(['close']);
const theme = inject('theme');
const onClose = e => emit('close', e);
</script>

<style lang="scss">
.app-drawer {
  position: absolute;
  left: 0;
  top: 0;
  height: 100%;
  width: 0px;
  z-index: 101;
  transition: width 0.2s ease-out;
  overflow: hidden;
  background-color: var(--bg-1);
  &--fixed {
    position: fixed;
    height: 100%;
  }
  &--open {
    width: 100%;
    @include above($sm) {
      width: 50%;
    }
  }
  &__overlay {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: #26292e;
    opacity: 0.8;
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
}
</style>
