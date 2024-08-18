<template>
  <AppOverlay v-if="isOverlay" @click="onClose">
    <template v-if="!isConnect">
      <div>Disconnected</div>
      <v-loader></v-loader>
    </template>
  </AppOverlay>
  <AppDialog v-bind="dialog" :progress="progress" @close="dialog = {}" />
  <AppDrawer :value="drawer" :change-theme="appStore.changeTheme" @close="drawer = false">
    <component :is="DrawerMain" :state="isConnect" :info="info" @close="drawer = false" />
  </AppDrawer>
  <AppHeader :state="isConnect" :change-theme="appStore.changeTheme" @drawer="drawer = !drawer" />
  <main class="v-spacer mt-16 mb-6">
    <router-view />
  </main>
  <AppFooter v-bind="info" />
</template>

<script setup>
import { ref, onMounted, onUnmounted, provide, computed } from 'vue';
import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore';
import { useWebSocket } from '@/stores/WebSocket';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import DrawerMain from '@/components/app/drawers/DrawerMain';
import AppDialog from '@/components/app/AppDialog';
import AppHeader from '@/components/app/AppHeader';
import AppFooter from '@/components/app/AppFooter';
import AppDrawer from '@/components/app/AppDrawer';
import AppOverlay from '@/components/app/AppOverlay';

const appStore = useAppStore();
const webSocket = useWebSocket();
const webSocketStore = useWebSocketStore();
const { dialog, theme } = storeToRefs(appStore);
const { socket, isConnect } = storeToRefs(webSocket);
const { info, progress } = storeToRefs(webSocketStore);

const drawer = ref(false);
const overlay = ref(false);

const isOverlay = computed(() => Boolean(!isConnect.value || isDialog.value || drawer.value || overlay.value));
const isDialog = computed(() => Boolean(dialog.value?.value));

let ping = null;

provide('overlay', overlay);
provide('theme', theme);
provide('dialog', appStore.setDialog);

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

const onClose = () => {
  drawer.value = false;
  dialog.value = {};
};

onMounted(() => {
  ping = setInterval(webSocket.onPing, 1000);
  setTimeout(connect, 100);
  appStore.init();
});

onUnmounted(() => {
  clearInterval(ping);
  socket.value.close(1000);
});
</script>
