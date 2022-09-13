<template>
  <div class="page-home container">
    <div class="row">
      <div class="page-home__logo col sm12 lg4 mt-16">
        <v-icons icon="logoMini" />
      </div>
      <div class="col sm12 lg8">
        <div class="text-h1 mt-16 mb-2">Vuesp is a web interface template for your projects on ESP8266 and EPS32</div>
        <div class="text-body-1 mb-6">
          Lorem ipsum dolor sit amet, consectetur adipiscing elit. Tellus rutrum a suspendisse massa. Elit dictumst
          turpis neque ac. Convallis neque, volutpat, donec ultrices ac in diam. Ultrices quam nibh id proin id et
          nascetur sed. Dolor consequat
        </div>
        <div class="">
          <v-button class="mr-2">More</v-button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
// import { onMounted } from 'vue';
// import { storeToRefs } from 'pinia';
// import { useWebSocketStore } from '@/stores/WebSocketStore';

// const webSocketStore = useWebSocketStore();
// const { settings } = storeToRefs(webSocketStore);

// onMounted(() => {
//   webSocketStore.onSend('SETTINGS');
// });
</script>

<style lang="scss">
.page-home {
  &__logo {
    .v-icons {
      justify-content: center;
      height: 200px;
    }
  }
}
</style>
