<template>
  <div class="page-settings container">
    <div class="row">
      <v-tabs>
        <v-tab label="Ethernet">
          <div class="row">
            <div class="col sm12 mb-4">
              <p>DHCP</p>
              <v-checkbox v-model="settings.ethDhcp" label="DHCP">
                {{ settings.ethDhcp ? 'ON' : 'OFF' }}
              </v-checkbox>
              {{settings.ethSubnet}}
            </div>
            <div class="col sm12 md12 lg8 mb-4">
              <v-input v-model="settings.ethIp" label="IP" :disabled="true" />
            </div>
            <div class="col sm12 md12 lg8 mb-4">
              <v-input v-model="settings.ethSubnet" label="Subnet" :disabled="isEthernetDHCP" :rules="[rules.required, rules.ip]">
              </v-input>
            </div>
            <div class="col sm12 md12 lg8 mb-4">
              <p>Geteway</p>
              <v-input v-model="settings.ethGeteway" placeholder="192.168.1.1" :disabled="isEthernetDHCP"></v-input>
            </div>
            <div class="col sm12 md12 lg8 mb-4">
              <p>DNS</p>
              <v-input v-model="settings.ethDns" placeholder="8.8.8.8" :disabled="isEthernetDHCP"></v-input>
            </div>
          </div>
        </v-tab>
        <v-tab label="Wi-Fi">
          <div class="col sm12">
            <div class="row">
              <div class="col sm12">
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
                <div class="row">
                  <div class="col sm12 md12 lg8">
                    <p>SSID</p>
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
                    <p>Password</p>
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
                <p>DHCP</p>
                <v-checkbox v-model="settings.wifiDhcp" label="DHCP" :disabled="!settings.wifiMode">
                  {{ settings.wifiDhcp ? 'ON' : 'OFF' }}
                </v-checkbox>
              </div>
              <div class="col sm12 md12 lg8">
                <p>IP</p>
                <v-input v-model="settings.wifiIp" placeholder="192.168.1.10" :disabled="isWifiDHCP" />
              </div>
              <div class="col sm12 md12 lg8">
                <p>Subnet</p>
                <v-input v-model="settings.wifiSubnet" placeholder="255.255.255.0" :disabled="isWifiDHCP"></v-input>
              </div>
              <div class="col sm12 md12 lg8">
                <p>Geteway</p>
                <v-input v-model="settings.wifiGeteway" placeholder="192.168.1.1" :disabled="isWifiDHCP"></v-input>
              </div>
              <div class="col sm12 md12 lg8">
                <p>DNS</p>
                <v-input v-model="settings.wifiDns" placeholder="8.8.8.8" :disabled="isWifiDHCP"></v-input>
              </div>
            </div>
          </div>
        </v-tab>
        <v-tab label="Server">
          <div class="row">
            <div class="col sm12">
              <h3>Server</h3>
            </div>
            <div class="col sm12 md12 lg8">
              <p>IP</p>
              <v-input v-model="settings.serverUrl" label="IP" placeholder="192.168.4.1"></v-input>
            </div>
            <div class="col sm12 md12 lg8">
              <p>Port</p>
              <v-input v-model.number="settings.serverPort" placeholder="8080"></v-input>
            </div>
          </div>
        </v-tab>
        <v-tab label="Authentication" icon="icon-lock">
          <div class="row">
            <div class="col sm12">
              <h3>Authentication</h3>
            </div>
            <div class="col sm12 md12 lg8">
              <p>Login</p>
              <v-input v-model="settings.httpLogin" placeholder="admin"></v-input>
            </div>
            <div class="col sm12 md12 lg8">
              <p>Password</p>
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
        <v-tab label="Maintenance">
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
import { computed, onMounted, ref } from 'vue';
import { storeToRefs } from 'pinia';
import { validateIP } from '@/utils/validate/';

import { useWebSocketStore } from '@/stores/WebSocketStore';

const webSocketStore = useWebSocketStore();
const { settings } = storeToRefs(webSocketStore);

const showPass = ref(false);

const rules = {
  required: value => !!value || 'Required.',
  ip: v => validateIP(v) || 'Invalid ip address',
  min: v => v.length >= 8 || 'Min 8 characters',
};


const isWifiDHCP = computed(() => Boolean(settings.value.wifiDhcp || !settings.value.wifiMode));
const isWifi = computed(() => Boolean(!settings.value.wifiMode));
const isEthernetDHCP = computed(() => Boolean(false));

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
