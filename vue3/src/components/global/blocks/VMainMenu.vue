<template>
  <ul class="v-main-menu text-title-1">
    <li v-for="{ name, path } of routers" class="v-main-menu__item" :key="name">
      <router-link :to="path" @click="onRoute">{{ name }}</router-link>
    </li>
  </ul>
</template>

<script setup>
import { useRouter } from 'vue-router';
import { computed, defineEmits } from 'vue';
const emit = defineEmits(['route']);

const router = useRouter();
const routers = computed(() => (router?.options?.routes || []).filter(i => i.path !== '/'));
const onRoute = e => emit('route', e);
</script>

<style lang="scss">
.v-main-menu {
  display: flex;
  align-items: center;
  gap: 30px;
  &__item {
    cursor: pointer;
  }
}
</style>
