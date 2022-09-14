<template>
  <v-overlay v-if="!isConnect">
    <div>Disconnected</div>
    <v-loader></v-loader>
  </v-overlay>
  <AppDialog v-bind="dialog" @close="dialog = {}"/>
  <AppDrawer :value="drawer" @close="drawer = false">
    <component :is="DrawerMain" :isConnect="isConnect" @close="drawer = false" />
  </AppDrawer>
  <AppHeader :isConnect="isConnect" @drawer="drawer = !drawer" />
  <main class="v-spacer mt-12">
    <router-view />
  </main>
  <AppFooter v-bind="info" />
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue';
import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore';
import { useWebSocket } from '@/stores/WebSocket';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import DrawerMain from '@/components/app/drawers/DrawerMain';
import AppDialog from '@/components/app/AppDialog';
import AppHeader from '@/components/app/AppHeader';
import AppFooter from '@/components/app/AppFooter';
import AppDrawer from '@/components/app/AppDrawer';

const drawer = ref(false);

const webSocketStore = useWebSocketStore();
const { info } = storeToRefs(webSocketStore);

const appStore = useAppStore();
const { dialog } = storeToRefs(appStore);

const webSocket = useWebSocket();
const { socket, isConnect } = storeToRefs(webSocket);
const host = process.env.NODE_ENV === 'production' ? window.location.host : process.env.PROXY;

let ping = null;

const connect = () => {
  const instance = new WebSocket(`ws://${host}/ws`);
  instance.binaryType = 'arraybuffer';
  instance.onopen = webSocket.onopen;
  instance.onmessage = webSocket.onmessage;
  instance.onclose = e => {
    webSocket.onclose(e);
    if (e.code !== 1000) connect();
  };
  instance.onerror = webSocket.onerror;
  socket.value = instance;
};

onMounted(() => {
  ping = setInterval(webSocket.onPing, 1000);
  connect();
});

onUnmounted(() => {
  clearInterval(ping);
  socket.value.close(1000);
});
</script>

<style lang="scss">
</style>
