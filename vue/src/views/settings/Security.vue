<template>
  <div class="row v-settings">
    <div class="col-24 mb-4">
      <h1>{{ $route.name }}</h1>
    </div>
    <div class="col-24">
      <div class="row v-settings__item">
        <div class="col-24 flex flex-middle">
          <h3>Auth</h3>
        </div>
        <div class="col-24 col-md-12 col-lg-8">
          <div class="label">Login</div>
          <at-input v-model="settings.httpLogin" placeholder="admin"></at-input>
        </div>
        <div class="col-24 col-md-12 col-lg-8">
          <div class="label">Password</div>
          <at-input
            v-model="settings.httpPass"
            placeholder="Password"
            :type="showPass ? 'text' : 'password'"
            :append-button="!isWifi"
          >
            <template slot="append">
              <i :class="`icon ${showPass ? 'icon-eye' : 'icon-eye-off'}`" @click="showPass = !showPass"></i>
            </template>
          </at-input>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  data: () => ({
    showPass: false,
    settings: {
      httpLogin: '',
      httpPass: '',
    },
  }),
  computed: {
    isWifiDHCP() {
      return Boolean(this.settings.wifiDhcp || !this.settings.wifiMode);
    },
    isWifi() {
      return Boolean(!this.settings.wifiMode);
    },
    isEthernetDHCP() {
      return Boolean(this.settings.ethDhcp);
    },
  },
};
</script>

<style lang="scss">
.v-settings {
  &__item {
    margin-bottom: 20px;
    .label {
      margin: 20px 0 10px 0;
      user-select: none;
    }
  }
}
</style>
