<template>
  <div class="h-[100dvh] min-h-full flex flex-col">
    <AppOverlay v-if="isOverlay" @click="onClose">
      <div class="mb-4">Disconnected</div>
      <div class="flex justify-center">
        <VLoader class="text-primary"></VLoader>
      </div>
    </AppOverlay>

    <AppDialog v-bind="dialog" :progress="progress" @close="dialog = {}" />

    <AppDrawer :value="drawer" :change-theme="appStore.changeTheme" @close="drawer = false">
      <component :is="DrawerMain" :state="isConnect" :info="info" @close="drawer = false" />
    </AppDrawer>

    <AppHeader :state="isConnect" :change-theme="appStore.changeTheme" @drawer="drawer = !drawer" />

    <main class="pt-20 px-4 py-6 sm:px-6 lg:px-8 flex-auto">
      <router-view />
    </main>

    <AppNavigation class="md:hidden" v-bind="info" />

    <AppFooter v-bind="info" class="pb-20 md:pb-4" />
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, provide, computed } from 'vue';
import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore';
import { useWebSocket } from '@/stores/WebSocket';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import DrawerMain from '@/components/app/drawers/DrawerMain';

import VLoader from '@/components/general/VLoader';

import AppDialog from '@/components/app/AppDialog';
import AppHeader from '@/components/app/AppHeader';
import AppFooter from '@/components/app/AppFooter';
import AppDrawer from '@/components/app/AppDrawer';
import AppOverlay from '@/components/app/AppOverlay';
import AppNavigation from '@/components/app/AppNavigation';

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
