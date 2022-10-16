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
        <v-main-menu class="d-flex a-center gap-6 text-title-1 fw-600" />
      </div>
      <div class="v-spacer"></div>
      <div class="app-header__right d-none d-lg-flex gap-4">
        <v-dropdown left="unset" right="0" top="0">
          <template #activator="{ on }">
            <v-icons icon="esp" @click="on.click"></v-icons>
          </template>
          <v-list v-slot="{ item }" :list="listMenu" @click="onMenu">
            <v-icons :icon="item.icon"></v-icons>
            {{ item.name }}
          </v-list>
        </v-dropdown>
      </div>
      <div class="app-header__burger d-lg-none" @click="onDrawer">
        <v-icons icon="burger"></v-icons>
      </div>
    </div>
  </header>
</template>

<script setup>
import { computed, defineEmits, defineProps, inject } from 'vue';
const props = defineProps({
  state: { type: Boolean, default: false },
  changeTheme: { type: Function, default: () => {} },
});
const emit = defineEmits(['drawer']);
const theme = inject('theme');

const listMenu = computed(() => [
  { name: 'Theme', icon: !theme.value ? 'dark' : 'light' },
  { name: 'Logout', icon: 'logout' },
]);
const onDrawer = e => emit('drawer', e);
const onLogout = async () => await fetch('/', { method: 'get', headers: { Authorization: 'Basic AAAAAAAAAAAAAAAAAAA=' } });
const onMenu = ({ name }) => {
  if (name == 'Logout') onLogout();
  if (name == 'Theme') props.changeTheme();
};
</script>

<style lang="scss">
.app-header {
  position: fixed;
  width: 100%;
  left: 0;
  top: 0;
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.1);
  background-color: var(--bg-2);
  z-index: 10;
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
    .v-list {
      svg {
        margin-right: 5px;
        height: 20px;
      }
    }
  }
  &__inner {
    height: 60px;
    display: flex;
    align-items: center;
  }
}
</style>
