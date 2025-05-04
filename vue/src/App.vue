<template>
  <div class="h-[100dvh] min-h-full flex flex-col">
    <AppOverlay v-if="!isConnect" @click="onClose">
      <div class="mb-4">Disconnected</div>

      <div class="flex justify-center">
        <VLoader class="text-primary"></VLoader>
      </div>
    </AppOverlay>

    <AppDialog v-bind="dialog" @close="dialog = {}" />

    <div class="flex h-screen overflow-hidden">
      <AppAside v-if="!isIframe" :info="info" :menu="menu" :sidebarToggle="sidebarToggle" @sidebar="sidebarToggle = !sidebarToggle" />

      <div class="relative flex flex-1 flex-col overflow-y-auto overflow-x-hidden">
        <AppHeader v-if="!isIframe" :state="isConnect" :change-theme="appStore.changeTheme" @sidebar="sidebarToggle = !sidebarToggle" />

        <AppNotification class="fixed right-4 md:right-10 lg:right-20 top-20 z-20" :notifications="notifications" @close="onNotifications" />

        <main class="px-4 py-6 sm:px-6 lg:px-8 flex-auto">
          <div :class="isIframe ? '' : 'container mx-auto'">
            <router-view />
          </div>
        </main>

        <!-- <AppNavigation class="md:hidden" v-bind="info" /> -->
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, provide } from 'vue';
import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore.js';
import { useWebSocket } from '@/stores/WebSocket.js';
import { useWebSocketStore } from '@/stores/WebSocketStore.ts';

import VLoader from '@/components/general/VLoader.vue';

import AppAside from '@/components/app/AppAside.vue';
import AppDialog from '@/components/app/AppDialog.vue';
import AppHeader from '@/components/app/AppHeader.vue';
import AppOverlay from '@/components/app/AppOverlay.vue';
import AppNotification from '@/components/app/AppNotification.vue';

import { useRoute } from 'vue-router';

import type { MenuType, TypeNotificationItem } from '@/types/types.ts';

import data from '../public/menu.json';

// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
const menu: MenuType[] = data?.menu || [];

import { ThemeKey, DialogKey, NotificationKey } from '@/simbol/index.ts';

console.log(menu);

const appStore = useAppStore();
const webSocket = useWebSocket();
const webSocketStore = useWebSocketStore();
const { dialog, theme, notifications } = storeToRefs(appStore);
const { socket, isConnect } = storeToRefs(webSocket);
const { info } = storeToRefs(webSocketStore);

const drawer = ref(false);
const isIframe = ref(false);
const sidebarToggle = ref(false);

let ping: ReturnType<typeof setTimeout> | null = null;

const route = useRoute();

provide(ThemeKey, theme);
provide(DialogKey, appStore.setDialog);
provide(NotificationKey, appStore.setNotification);

const mode = import.meta.env.MODE;
const proxy = import.meta.env.VITE_PROXY;

const host = mode === 'production' ? window.location.host : proxy;

const connect = () => {
  const instance: any = new WebSocket(`ws://${host}/esp`);
  instance.binaryType = 'arraybuffer';
  instance.onopen = webSocket.onopen;
  instance.onmessage = webSocket.onmessage;
  instance.onclose = (e: any) => {
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

const onNotifications = (item: TypeNotificationItem) => {
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
  if (ping) clearInterval(ping);
  socket.value.close(1000);
});
</script>
