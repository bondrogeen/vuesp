<template>
  <ul class="d-flex a-center gap-6 text-title-1 fw-600">
    <li v-for="{ name, path } of routers" :key="name" class="first-letter">
      <router-link :to="path" @click="onRoute">{{ name }}</router-link>
    </li>
  </ul>
</template>

<script setup>
import { useRouter } from 'vue-router';
import { computed, defineEmits } from 'vue';
const emit = defineEmits(['route']);

const router = useRouter();
const pathFilter = i => {
  return i.path !== '/' && !i.path.includes(':');
};
const routers = computed(() => (router?.options?.routes || []).filter(pathFilter));
const onRoute = e => emit('route', e);
</script>
