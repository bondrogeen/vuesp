<template>
  <v-overlay v-if="!isConnect">
    <div>Disconnected</div>
    <v-loader></v-loader>
  </v-overlay>
  <AppDialog v-bind="dialog" :progress="progress" @close="dialog = {}" />
  <AppDrawer :value="drawer" :change-theme="appStore.changeTheme" @close="drawer = false">
    <component :is="DrawerMain" :state="isConnect" :info="info" @close="drawer = false" />
  </AppDrawer>
  <AppHeader :state="isConnect" :change-theme="appStore.changeTheme" @drawer="drawer = !drawer" />
  <main class="v-spacer mt-16 mb-6">
    <router-view v-bind="bindView" />
  </main>
  <AppFooter v-bind="info" />
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted, provide } from 'vue';
import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore';
import { useWebSocket } from '@/stores/WebSocket';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import DrawerMain from '@/components/app/drawers/DrawerMain';
import AppDialog from '@/components/app/AppDialog';
import AppHeader from '@/components/app/AppHeader';
import AppFooter from '@/components/app/AppFooter';
import AppDrawer from '@/components/app/AppDrawer';

const appStore = useAppStore();
const webSocket = useWebSocket();
const webSocketStore = useWebSocketStore();
const { dialog, theme } = storeToRefs(appStore);
const { socket, isConnect } = storeToRefs(webSocket);
const { info, progress } = storeToRefs(webSocketStore);

let ping = null;
const drawer = ref(false);
provide('theme', theme);

const bindView = computed(() => ({ setDialog: appStore.setDialog }));
const host = process.env.NODE_ENV === 'production' ? window.location.host : process.env.PROXY;

const connect = () => {
  const instance = new WebSocket(`ws://${host}/esp`);
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
  appStore.init();
});

onUnmounted(() => {
  clearInterval(ping);
  socket.value.close(1000);
});
</script>

<style lang="scss"></style>
