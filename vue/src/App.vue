<template>
  <div id="app" class="app">
    <AppOverlay v-if="overlay" />
    <header class="container">
      <AppHeader />
    </header>
    <main class="container">
      <router-view />
    </main>
    <footer>
      <AppFooter />
    </footer>
  </div>
</template>

<script>
import AppHeader from '@/components/app/AppHeader';
import AppFooter from '@/components/app/AppFooter';
import AppOverlay from '@/components/app/AppOverlay';
import { mapActions, mapGetters } from 'vuex';
export default {
  components: {
    AppHeader,
    AppFooter,
    AppOverlay,
  },
  data: () => ({
    interval: null,
  }),
  computed: {
    ...mapGetters({
      overlay: 'app/getOverlay',
      ping: 'app/getPing',
      isConnect: 'app/isConnect',
    }),
  },
  methods: {
    ...mapActions({
      setConnect: 'app/setConnect',
    }),
    onInterval() {
      this.setConnect((Date.now() - this.ping) < 2000);
    },
  },
  mounted() {
    document.addEventListener('DOMContentLoaded', function () {
      const element = document.getElementById('overlay');
      element.style.display = 'none';
    });
    if (!this.interval) {
      this.interval = setInterval(this.onInterval, 1000);
    }
  },
  beforeDestroy() {
    if (this.interval) {
      clearInterval(this.interval);
      this.interval = null;
    }
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
