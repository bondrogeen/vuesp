<template>
  <div class="app-drawer h-full overflow-hidden shadow z-[101]" :class="[{ 'translate-x-0': value }, { 'fixed h-[100dvh]': position }]">
    <div class="overflow-auto min-w-[360px] absolute h-full w-full">
      <div class="bg-blue-50 flex justify-between p-4">
        <button @click="changeTheme">
          <IconDark v-if="!theme" />
          <IconLight v-else />
        </button>
        <div @click="onClose">
          <router-link to="/">
            <IconLogo class="h-[30px]"></IconLogo>
          </router-link>
        </div>
        <button @click="onClose">
          <IconClose></IconClose>
        </button>
      </div>
      <div class="app-drawer__body px-4 bg-white">
        <slot></slot>
      </div>
    </div>
  </div>
</template>

<script setup>
import { defineProps, defineEmits, inject } from 'vue';

import IconLogo from '@/components/icons/IconLogo';
import IconClose from '@/components/icons/IconClose';
import IconDark from '@/components/icons/IconDark';
import IconLight from '@/components/icons/IconLight';

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
  left: 0;
  top: 0;
  transition: all 0.2s ease-in-out;

  transform: translateX(-100%);
  width: 100%;

  @include above($sm) {
    width: 360px;
  }

  @include above($md) {
    transform: translateX(-100%);
  }

  &__body {
    height: calc(100dvh - 62px);
  }
}
</style>
