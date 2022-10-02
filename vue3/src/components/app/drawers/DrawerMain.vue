<template>
  <div class="drawer-main">
    <div class="drawer-main__menu">
      <v-main-menu @route="onClose" />
    </div>
    <div class="v-spacer"></div>
    <div class="drawer-main__btn mb-6">
      <v-button class="mr-2">{{ isConnect ? 'Connected' : 'Disconnected' }}</v-button>
    </div>
  </div>
</template>

<script setup>
import { defineEmits, defineProps } from 'vue';
defineProps({
  isConnect: { type: Boolean, default: false },
});
const emit = defineEmits(['close']);
const onClose = value => emit('close', value);
</script>

<style lang="scss">
.drawer-main {
  display: flex;
  flex-direction: column;
  height: 100%;
  &__btn {
    display: flex;
    justify-content: center;
  }
  &__menu {
    .v-main-menu {
      flex-direction: column;
    }
    .v-main-menu__item {
      font-size: 24px;
    }
  }
}
</style>
