<template>
  <div class="row v-settings mb-4">
    <div class="col-24">
      <h1>{{ $route.name }}</h1>
    </div>

    <div class="col-24">
      <at-tabs type="card">
        <at-tab-pane class="v-settings__tab" label="Ethernet" icon="icon-inbox">
          <div class="row v-settings__item">
            <div class="col-24 flex flex-middle">
              <h3>Ethernet</h3>
            </div>
            <div class="col-24">
              <p class="label">DHCP</p>
              <at-checkbox v-model="settings.ethDhcp" label="DHCP">
                {{ settings.ethDhcp ? 'ON' : 'OFF' }}
              </at-checkbox>
            </div>
            <div class="col-24 col-md-12 col-lg-8">
              <p class="label">IP</p>
              <at-input v-model="settings.ethIp" placeholder="192.168.1.10" :disabled="isEthernetDHCP" />
            </div>
            <div class="col-24 col-md-12 col-lg-8">
              <p class="label">Subnet</p>
              <at-input v-model="settings.ethSubnet" placeholder="255.255.255.0" :disabled="isEthernetDHCP"></at-input>
            </div>
            <div class="col-24 col-md-12 col-lg-8">
              <p class="label">Geteway</p>
              <at-input v-model="settings.ethGeteway" placeholder="192.168.1.1" :disabled="isEthernetDHCP"></at-input>
            </div>
            <div class="col-24 col-md-12 col-lg-8">
              <p class="label">DNS</p>
              <at-input v-model="settings.ethDns" placeholder="8.8.8.8" :disabled="isEthernetDHCP"></at-input>
            </div>
          </div>
        </at-tab-pane>
        <at-tab-pane class="v-settings__tab" label="Wi-Fi" icon="icon-wifi">
          <div class="col-24">
            <div class="row v-settings__item">
              <div class="col-24 flex flex-middle">
                <h3>Wi-Fi</h3>
                <div class="spacer" />
                <p class="mr-4">Mode</p>
                <at-select v-model="settings.wifiMode" style="width: 150px">
                  <at-option :value="0">OFF</at-option>
                  <at-option :value="1">STA</at-option>
                  <at-option :value="2">AP</at-option>
                  <at-option :value="3">STA + AP</at-option>
                </at-select>
              </div>
              <div class="col-24">
                <div class="row v-settings__item">
                  <div class="col-24 col-md-12 col-lg-8">
                    <p class="label">SSID</p>
                    <at-input
                      v-model="settings.wifiSsid"
                      placeholder="SSID"
                      :disabled="isWifi"
                      :append-button="!isWifi"
                      :maxlength="32"
                    >
                      <template slot="append">
                        <span>Search</span>
                      </template>
                    </at-input>
                  </div>
                  <div class="col-24 col-md-12 col-lg-8">
                    <p class="label">Password</p>
                    <at-input
                      v-model="settings.wifiPass"
                      placeholder="Password"
                      :disabled="isWifi"
                      :type="showPass ? 'text' : 'password'"
                      :append-button="!isWifi"
                      :minlength="8"
                      :maxlength="32"
                    >
                      <template slot="append">
                        <i :class="`icon ${showPass ? 'icon-eye' : 'icon-eye-off'}`" @click="showPass = !showPass"></i>
                      </template>
                    </at-input>
                  </div>
                </div>
              </div>
              <div class="col-24">
                <p class="label">DHCP</p>
                <at-checkbox v-model="settings.wifiDhcp" label="DHCP" :disabled="!settings.wifiMode">
                  {{ settings.wifiDhcp ? 'ON' : 'OFF' }}
                </at-checkbox>
              </div>
              <div class="col-24 col-md-12 col-lg-8">
                <p class="label">IP</p>
                <at-input v-model="settings.wifiIp" placeholder="192.168.1.10" :disabled="isWifiDHCP" />
              </div>
              <div class="col-24 col-md-12 col-lg-8">
                <p class="label">Subnet</p>
                <at-input v-model="settings.wifiSubnet" placeholder="255.255.255.0" :disabled="isWifiDHCP"></at-input>
              </div>
              <div class="col-24 col-md-12 col-lg-8">
                <p class="label">Geteway</p>
                <at-input v-model="settings.wifiGeteway" placeholder="192.168.1.1" :disabled="isWifiDHCP"></at-input>
              </div>
              <div class="col-24 col-md-12 col-lg-8">
                <p class="label">DNS</p>
                <at-input v-model="settings.wifiDns" placeholder="8.8.8.8" :disabled="isWifiDHCP"></at-input>
              </div>
            </div>
          </div>
        </at-tab-pane>
        <at-tab-pane class="v-settings__tab" label="Server" icon="icon-server">
          <div class="row v-settings__item">
            <div class="col-24 flex flex-middle">
              <h3>Server</h3>
            </div>
            <div class="col-24 col-md-12 col-lg-8">
              <p class="label">IP</p>
              <at-input v-model="settings.serverUrl" placeholder="192.168.4.1"></at-input>
            </div>
            <div class="col-24 col-md-12 col-lg-8">
              <p pclass="label">Port</p>
              <at-input v-model.number="settings.serverPort" placeholder="8080"></at-input>
            </div>
          </div>
        </at-tab-pane>
        <at-tab-pane class="v-settings__tab" label="Authentication" icon="icon-lock">
          <div class="row v-settings__item">
            <div class="col-24 flex flex-middle">
              <h3>Authentication</h3>
            </div>
            <div class="col-24 col-md-12 col-lg-8">
              <p class="label">Login</p>
              <at-input v-model="settings.httpLogin" placeholder="admin"></at-input>
            </div>
            <div class="col-24 col-md-12 col-lg-8">
              <p class="label">Password</p>
              <at-input
                v-model="settings.httpPass"
                placeholder="Password"
                :type="showPass ? 'text' : 'password'"
                append-button
              >
                <template slot="append">
                  <i :class="`icon ${showPass ? 'icon-eye' : 'icon-eye-off'}`" @click="showPass = !showPass"></i>
                </template>
              </at-input>
            </div>
          </div>
        </at-tab-pane>
        <at-tab-pane class="v-settings__tab" label="Maintenance" icon="icon-refresh-ccw">
          <Maintenance />
        </at-tab-pane>
      </at-tabs>
    </div>
    <div class="col-24 flex flex-end v-settings__btn">
      <at-button type="primary" @click="onSave">Save</at-button>
    </div>
  </div>
</template>

<script>
import { mapActions, mapGetters } from 'vuex';
import Maintenance from '@/components/settings/Maintenance.vue';

export default {
  components: {
    Maintenance,
  },
  data: () => ({
    showPass: false,
    settings: {
      wifiDhcp: 0,
      wifiMode: 1,
      wifiIp: '',
      wifiSubnet: '',
      wifiGeteway: '',
      wifiDns: '',
      wifiSsid: '',
      wifiPass: '',
      ethDhcp: 0,
      ethIp: '',
      ethSubnet: '',
      ethGeteway: '',
      ethDns: '',
      serverUrl: '',
      serverPort: 8080,
      httpLogin: '',
      httpPass: '',
    },
  }),
  computed: {
    ...mapGetters('app', ['getSettings']),
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
  methods: {
    ...mapActions('socket', ['onSend']),
    onSave() {
      console.log('onSave');
      const settings = { ...this.getSettings, ...this.settings };
      this.onSend({ comm: 'SETTINGS', data: settings });
    },
  },
  mounted() {
    console.log('mounted');
    this.onSend({ comm: 'SETTINGS' });
  },
  watch: {
    getSettings(value) {
      this.settings = { ...value };
      console.log(value);
    },
  },
};
</script>

<style lang="scss">
.v-settings {
  &__item {
    margin-bottom: 20px;
    .label {
      margin: 10px 0;
      user-select: none;
    }
  }
  &__tab {
    padding: 0 20px;
  }
  &__btn {
    padding: 0 25px;
  }
}
.at-tabs-nav__item {
  padding: 0 10px;
}
</style>
