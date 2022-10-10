<template>
  <v-icons :icon="!theme ? 'dark' : 'light'" @click="onChangeTheme"></v-icons>
</template>

<script setup>
import { ref, onMounted } from 'vue';
const theme = ref(false);

const onChangeTheme = () => {
  theme.value = !theme.value;
  document.documentElement.setAttribute('data-theme', theme.value ? 'dark' : 'light');
  localStorage.setItem('theme', theme.value ? 'dark' : 'light');
};

onMounted(() => {
  theme.value = localStorage.getItem('theme') === 'dark';
  document.documentElement.setAttribute('data-theme', theme.value ? 'dark' : 'light');
});
</script>
