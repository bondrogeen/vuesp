<template>
  <div class="h-[100dvh] min-h-full flex flex-col">
    <AppOverlay v-if="!isConnect" @click="onClose">
      <div class="mb-4">Disconnected</div>

      <div class="flex justify-center">
        <VLoader class="text-primary"></VLoader>
      </div>
    </AppOverlay>

    <AppDialog v-bind="dialog" @close="dialog = {}" />

    <AppDrawer :value="drawer" :change-theme="appStore.changeTheme" @close="drawer = false">
      <component :is="DrawerMain" :state="isConnect" :info="info" @close="drawer = false" />
    </AppDrawer>

    <div class="flex h-screen overflow-hidden">
      <AppAside v-bind="info" :menu="menu" :sidebarToggle="sidebarToggle" @sidebar="sidebarToggle = !sidebarToggle" />

      <div class="relative flex flex-1 flex-col overflow-y-auto overflow-x-hidden">
        <AppHeader v-if="!isIframe" :state="isConnect" :change-theme="appStore.changeTheme" @sidebar="sidebarToggle = !sidebarToggle" />

        <AppNotification class="fixed right-4 md:right-10 lg:right-20 top-20 z-20" :notifications="notifications" @close="onNotifications" />

        <main class="px-4 py-6 sm:px-6 lg:px-8 flex-auto">
          <div :class="isIframe ? '' : 'container mx-auto'">
            <router-view />
          </div>
        </main>

        <!-- <AppNavigation class="md:hidden" v-bind="info" /> -->

        <!-- <AppFooter v-if="!isIframe" v-bind="info" class="" /> -->
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, provide } from 'vue';
import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore';
import { useWebSocket } from '@/stores/WebSocket';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import DrawerMain from '@/components/app/drawers/DrawerMain';

import VLoader from '@/components/general/VLoader';

import AppAside from '@/components/app/AppAside.vue';
import AppDialog from '@/components/app/AppDialog';
import AppHeader from '@/components/app/AppHeader';
import AppFooter from '@/components/app/AppFooter';
import AppDrawer from '@/components/app/AppDrawer';
import AppOverlay from '@/components/app/AppOverlay';
import AppNavigation from '@/components/app/AppNavigation';
import AppNotification from '@/components/app/AppNotification';
import { useRoute } from 'vue-router';

import { menu } from '@/temp.js';

const appStore = useAppStore();
const webSocket = useWebSocket();
const webSocketStore = useWebSocketStore();
const { dialog, theme, notifications } = storeToRefs(appStore);
const { socket, isConnect } = storeToRefs(webSocket);
const { info } = storeToRefs(webSocketStore);

const drawer = ref(false);
const isIframe = ref(false);
const sidebarToggle = ref(false);

let ping = null;

const route = useRoute();

provide('theme', theme);
provide('dialog', appStore.setDialog);
provide('notification', appStore.setNotification);

const mode = import.meta.env.MODE;
const proxy = import.meta.env.VITE_PROXY;

const host = mode === 'production' ? window.location.host : proxy;

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

const onNotifications = item => {
  console.log(item);

  notifications.value = notifications.value.filter(i => i.id !== item.id);
};

onMounted(() => {
  ping = setInterval(webSocket.onPing, 1000);
  setTimeout(connect, 100);
  appStore.init(route.query);

  if (window.self !== window.top) {
    isIframe.value = true;

    console.log('isIframe:');

    window.addEventListener('message', event => {
      // if (event.origin !== 'https://parent-domain.com') return;

      console.log('Данные:', event);
      if (event.data.type === 'myEvent') {
        console.log('Данные:', event.data.data);
      }
    });
  }
});

onUnmounted(() => {
  clearInterval(ping);
  socket.value.close(1000);
});
</script>
