<template>
  <footer class="app-footer">
    <div class="d-flex a-center j-between container">
      <div class="d-flex a-center flex-wrap gap-4">
        <div>© 2021 - {{ new Date().getFullYear() }} Vuesp.</div>
        <div>Firmware (ver.{{ frimware }})</div>
        <div>ID: {{ id.toString(16) }}</div>
      </div>

      <div class="app-footer__link">
        <a href="https://github.com/bondrogeen/vuesp" target="_blank">GitHub</a>
      </div>
    </div>
  </footer>
</template>

<script setup>
import { defineProps } from 'vue';
defineProps({
  frimware: { type: String, default: '' },
  id: { type: Number, default: 0 },
});
</script>

<style lang="scss">
.app-footer {
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.1);
  background-color: var(--bg-2);
  padding: 30px 0;
  &__inner {
    flex-wrap: wrap;
  }
}
</style>
