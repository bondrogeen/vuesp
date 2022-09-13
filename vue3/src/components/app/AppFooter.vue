<template>
  <footer class="app-footer">
    <div class="app-footer__inner container">
      <div>© 2021 - {{ new Date().getFullYear() }} Vuesp. (ver. 0.0.1)</div>
      <div class="app-footer__link"><a href="https://github.com/bondrogeen/vuesp" target="_blank">GitHub</a></div>
    </div>
  </footer>
</template>

<script setup>
// const onChengeThems = () => {
// };
</script>

<style lang="scss">
.app-footer {
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.1);
  background-color: var(--var-bg-2);
  &__inner {
    height: 40px;
    display: flex;
    align-items: center;
    justify-content: space-between;
  }
  &__link {

  }
}
</style>
