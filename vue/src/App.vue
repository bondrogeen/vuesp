<template>
  <div class="h-[100dvh] min-h-full flex flex-col">
    <AppOverlay v-if="!isConnect" @click="onClose">
      <div class="mb-4">Disconnected</div>

      <div class="flex justify-center">
        <v-loader class="text-primary"></v-loader>
      </div>
    </AppOverlay>

    <template v-else>
      <div class="flex h-screen overflow-hidden">
        <AppAside v-if="!isIframe" :info="main.info" :menu="menu" :sidebarToggle="sidebarToggle" @sidebar="sidebarToggle = !sidebarToggle" />

        <div class="relative flex flex-1 flex-col overflow-y-auto overflow-x-hidden scrollbar">
          <AppHeader v-if="!isIframe" :change-theme="appStore.changeTheme" @sidebar="sidebarToggle = !sidebarToggle" />

          <AppNotification class="fixed right-4 md:right-10 lg:right-20 top-20 z-20" :notifications="notifications" @close="onNotifications" />

          <main :class="isIframe ? 'no-scrollbar' : 'px-4 py-6 sm:px-6 lg:px-8 flex-auto'">
            <div :class="isIframe ? '' : 'container mx-auto'">
              <router-view />
            </div>
          </main>
        </div>
      </div>
    </template>

    <AppDialog v-if="dialog.value" v-bind="dialog" @close="dialog = {}" />
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, provide } from 'vue';
import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore.js';
import { useWebSocket } from '@/stores/WebSocket.js';
import { useWebSocketStore } from '@/stores/WebSocketStore.ts';

import type { TypeNotificationItem } from '@/types/types.ts';

import { useRoute, useRouter } from 'vue-router';

import AppAside from '@/components/app/AppAside.vue';
import AppDialog from '@/components/app/AppDialog.vue';
import AppHeader from '@/components/app/AppHeader.vue';
import AppOverlay from '@/components/app/AppOverlay.vue';
import AppNotification from '@/components/app/AppNotification.vue';

import { DialogKey, NotificationKey } from '@/simbol/index.ts';

const appStore = useAppStore();
const webSocket = useWebSocket();
const webSocketStore = useWebSocketStore();
const { menu, dialog, notifications } = storeToRefs(appStore);
const { socket, isConnect } = storeToRefs(webSocket);
const { main } = storeToRefs(webSocketStore);

const drawer = ref(false);
const isIframe = ref(false);
const sidebarToggle = ref(false);

let ping: ReturnType<typeof setTimeout> | null = null;

const route = useRoute();
const router = useRouter();

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
  notifications.value = notifications.value.filter((i) => i.id !== item.id);
};

onMounted(() => {
  ping = setInterval(webSocket.onPing, 1000);
  setTimeout(connect, 100);
  appStore.init(route.query);

  if (window.self !== window.top) {
    isIframe.value = true;
    const html = document.querySelector('html');
    if (html) {
      html.classList.add('no-scrollbar');
    }

    window.addEventListener('message', (event) => {
      console.log('Данные:', event);
      if (event?.data?.type === 'theme') {
        appStore.changeTheme(event.data.value);
      }
      if (event?.data?.type === 'route') {
        const data = event.data.data;
        router.push(data);
      }
    });
  }
});

onUnmounted(() => {
  if (ping) clearInterval(ping);
  socket.value.close(1000);
});
</script>
