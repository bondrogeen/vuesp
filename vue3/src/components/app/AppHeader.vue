<template>
  <header class="app-header">
    <div class="app-header__inner container">
      <div class="app-header__logo">
        <router-link to="/">
          <v-icons icon="logo"></v-icons>
        </router-link>
        <div class="app-header__indicator" :class="{ 'app-header__indicator--disabled': !state }"></div>
      </div>
      <div class="app-header__menu d-none d-lg-flex">
        <v-main-menu />
      </div>
      <div class="v-spacer"></div>
      <div class="app-header__right d-none d-lg-flex">
        <v-theme-button />
      </div>
      <div class="app-header__burger d-lg-none" @click="onDrawer">
        <v-icons icon="burger"></v-icons>
      </div>
    </div>
  </header>
</template>

<script setup>
import { defineEmits, defineProps } from 'vue';
defineProps({
  state: { type: Boolean, default: false },
});
const emit = defineEmits(['drawer']);
const onDrawer = e => emit('drawer', e);
</script>

<style lang="scss">
.app-header {
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.1);
  background-color: var(--bg-2);
  &__logo {
    margin-right: 60px;
    svg {
      height: 30px;
    }
  }
  &__indicator {
    height: 3px;
    width: 100%;
    background: color(app, primary);
    transition: all 0.3s ease-in-out;
    &--disabled {
      background: color('red', 'base');
    }
  }
  &__right {
    display: flex;
    align-items: center;
    svg {
      height: 20px;
    }
    .v-icons {
      cursor: pointer;
    }
  }
  &__inner {
    height: 60px;
    display: flex;
    align-items: center;
  }
}
</style>
