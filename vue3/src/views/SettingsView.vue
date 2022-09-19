<template>
  <div class="page-settings container">
    <div class="row">
      {{ settings }}
      <v-tabs type="card">
        <v-tab class="" label="Ethernet">
          <div class="row ">
            <div class="col sm12 flex flex-middle">
              <h3>Ethernet</h3>
            </div>
            <div class="col sm12">
              <p class="label">DHCP</p>
              <v-checkbox v-model="settings.ethDhcp" label="DHCP">
                {{ settings.ethDhcp ? 'ON' : 'OFF' }}
              </v-checkbox>
            </div>
            <div class="col sm12 md12 lg8">
              <p class="label">IP</p>
              <v-input v-model="settings.ethIp" placeholder="192.168.1.10" :disabled="isEthernetDHCP" />
            </div>
            <div class="col sm12 md12 lg8">
              <p class="label">Subnet</p>
              <v-input v-model="settings.ethSubnet" placeholder="255.255.255.0" :disabled="isEthernetDHCP"></v-input>
            </div>
            <div class="col sm12 md12 lg8">
              <p class="label">Geteway</p>
              <v-input v-model="settings.ethGeteway" placeholder="192.168.1.1" :disabled="isEthernetDHCP"></v-input>
            </div>
            <div class="col sm12 md12 lg8">
              <p class="label">DNS</p>
              <v-input v-model="settings.ethDns" placeholder="8.8.8.8" :disabled="isEthernetDHCP"></v-input>
            </div>
          </div>
        </v-tab>
        <v-tab class="" label="Wi-Fi">
          <div class="col sm12">
            <div class="row ">
              <div class="col sm12 flex flex-middle">
                <h3>Wi-Fi</h3>
                <div class="spacer" />
                <p class="mr-4">Mode</p>
                <v-select v-model="settings.wifiMode" style="width: 150px">
                  <!-- <at-option :value="0">OFF</at-option>
                  <at-option :value="1">STA</at-option>
                  <at-option :value="2">AP</at-option>
                  <at-option :value="3">STA + AP</at-option> -->
                </v-select>
              </div>
              <div class="col sm12">
                <div class="row ">
                  <div class="col sm12 md12 lg8">
                    <p class="label">SSID</p>
                    <v-input
                      v-model="settings.wifiSsid"
                      placeholder="SSID"
                      :disabled="isWifi"
                      :append-button="!isWifi"
                      :maxlength="32"
                      @onAppend="onScan"
                    >
                      <template #append>
                        <span>Search</span>
                      </template>
                    </v-input>
                  </div>
                  <div class="col sm12 md12 lg8">
                    <p class="label">Password</p>
                    <v-input
                      v-model="settings.wifiPass"
                      placeholder="Password"
                      :disabled="isWifi"
                      :type="showPass ? 'text' : 'password'"
                      :append-button="!isWifi"
                      :minlength="8"
                      :maxlength="32"
                      @onAppend="showPass = !showPass"
                    >
                      <template #append>
                        <i :class="`icon ${showPass ? 'icon-eye' : 'icon-eye-off'}`"></i>
                      </template>
                    </v-input>
                  </div>
                </div>
              </div>
              <div class="col sm12">
                <p class="label">DHCP</p>
                <v-checkbox v-model="settings.wifiDhcp" label="DHCP" :disabled="!settings.wifiMode">
                  {{ settings.wifiDhcp ? 'ON' : 'OFF' }}
                </v-checkbox>
              </div>
              <div class="col sm12 md12 lg8">
                <p class="label">IP</p>
                <v-input v-model="settings.wifiIp" placeholder="192.168.1.10" :disabled="isWifiDHCP" />
              </div>
              <div class="col sm12 md12 lg8">
                <p class="label">Subnet</p>
                <v-input v-model="settings.wifiSubnet" placeholder="255.255.255.0" :disabled="isWifiDHCP"></v-input>
              </div>
              <div class="col sm12 md12 lg8">
                <p class="label">Geteway</p>
                <v-input v-model="settings.wifiGeteway" placeholder="192.168.1.1" :disabled="isWifiDHCP"></v-input>
              </div>
              <div class="col sm12 md12 lg8">
                <p class="label">DNS</p>
                <v-input v-model="settings.wifiDns" placeholder="8.8.8.8" :disabled="isWifiDHCP"></v-input>
              </div>
            </div>
          </div>
        </v-tab>
        <v-tab class="" label="Server">
          <div class="row ">
            <div class="col sm12">
              <h3>Server</h3>
            </div>
            <div class="col sm12 md12 lg8">
              <p class="label">IP</p>
              <v-input v-model="settings.serverUrl" placeholder="192.168.4.1"></v-input>
            </div>
            <div class="col sm12 md12 lg8">
              <p class="label">Port</p>
              <v-input v-model.number="settings.serverPort" placeholder="8080"></v-input>
            </div>
          </div>
        </v-tab>
        <v-tab class="" label="Authentication" icon="icon-lock">
          <div class="row ">
            <div class="col sm12 flex flex-middle">
              <h3>Authentication</h3>
            </div>
            <div class="col sm12 md12 lg8">
              <p class="label">Login</p>
              <v-input v-model="settings.httpLogin" placeholder="admin"></v-input>
            </div>
            <div class="col sm12 md12 lg8">
              <p class="label">Password</p>
              <v-input
                v-model="settings.httpPass"
                placeholder="Password"
                :type="showPass ? 'text' : 'password'"
                append-button
                @onAppend="showPass = !showPass"
              >
                <template #append>
                  <i :class="`icon ${showPass ? 'icon-eye' : 'icon-eye-off'}`"></i>
                </template>
              </v-input>
            </div>
          </div>
        </v-tab>
        <v-tab class="" label="Maintenance">
          <!-- <Maintenance /> -->
        </v-tab>
      </v-tabs>
    </div>
    <div class="col sm12 flex flex-end">
      <v-button @click="onSave">Save</v-button>
    </div>
    <!-- <DialogScan v-model="dialogScan" @click="onSelect" @scan="onScan" /> -->
  </div>
</template>

<script setup>
// import Maintenance from '@/components/settings/Maintenance.vue';
// import DialogScan from '../../components/settings/dialogs/DialogScan.vue';

import { computed, onMounted, ref } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

const webSocketStore = useWebSocketStore();
const { settings } = storeToRefs(webSocketStore);

const showPass =ref(false)

const isWifiDHCP = computed(() => Boolean(settings.value.wifiDhcp || !settings.value.wifiMode));
const isWifi = computed(() => Boolean(!settings.value.wifiMode));
const isEthernetDHCP = computed(() => Boolean(settings.value.ethDhcp));

const onSave = () => {
  // const settings = { ...this.getSettings, ...this.settings };
  // onSend({ comm: 'SETTINGS', data: settings });
  // onReboot();
};
const onScan = () => {
  // clearScanList();
  // onSend({ comm: 'SCAN' });
};

// const onSelect = () => {
  // dialogScan = false;
  // settings.wifiSsid = value;
// };
// const onReboot = async () => {
//   try {
//     await this.$Modal.confirm({
//       title: 'Reboot',
//       content: 'Do you want to reboot the unit?',
//     });
//     this.onSend({ comm: 'REBOOT' });
//   } catch (error) {
//     console.log(error);
//   }
// };

onMounted(() => {
  webSocketStore.onSend('SETTINGS');
});
</script>

<style lang="scss">
.page-settings {
  position: relative;
}
</style>
