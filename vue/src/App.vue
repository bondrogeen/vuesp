<template>
  <div id="app" class="app">
    <header class="container">
      <AppHeader />
    </header>
    <main class="container">
      <router-view />
    </main>
    <footer >
      <AppFooter />
    </footer>
  </div>
</template>

<script>
import AppHeader from '@/components/app/AppHeader';
import AppFooter from '@/components/app/AppFooter';
export default {
  components: {
    AppHeader,
    AppFooter,
  },
};
</script>

<style lang="scss">
.app {
  display: flex;
  flex-direction: column;
  height: 100%;
  background-color: #f7f7f7;
  header {
    background-color: #fff;
    flex: 0 0 50px;
  }
  main {
    background-color: #fff;
    flex: 1 1 100%;

    padding: 0 20px;
  }
  footer {
    background-color: #d7e4fe;
    flex: 0 0 50px;
    padding: 0 20px;
  }
}
</style>
