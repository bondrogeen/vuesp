<template>
  <div class="h-[100dvh] min-h-full flex flex-col">
    <AppOverlay v-if="!isConnect" @click="onClose">
      <div class="mb-4">Disconnected</div>

      <div class="flex justify-center">
        <VLoader class="text-primary"></VLoader>
      </div>
    </AppOverlay>
    <template v-else>
      <div class="flex h-screen overflow-hidden">
        <AppAside v-if="!isIframe" :nameDevice="nameDevice" :fullPath="fullPath" :menu="menu" :sidebarToggle="sidebarToggle" @sidebar="sidebarToggle = !sidebarToggle">
          <ServiceInfo v-bind="main.info" class="mb-4 w-full rounded-2xl bg-gray-50 px-4 py-4 text-center dark:bg-white/[0.03]" :class="sidebarToggle ? 'lg:hidden' : ''">
            <v-button href="https://github.com/bondrogeen/vuesp" class="w-full" target="_blank" color="blue">Github</v-button>
          </ServiceInfo>
        </AppAside>

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
import { ref, onMounted, onUnmounted, provide, computed, inject } from 'vue';
import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore';
import { useWebSocket } from '@/stores/WebSocket';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import type { INotificationItem } from 'vuesp-components/types';

import { useRoute, useRouter } from 'vue-router';

// import AppAside from '@/components/app/AppAside.vue';

import { VuespKey, DialogKey, NotificationKey } from '@/utils/types/simbol';

import ServiceInfo from '@/components/service/ServiceInfo.vue';

const vuesp = inject(VuespKey);

const appStore = useAppStore();
const webSocket = useWebSocket();
const webSocketStore = useWebSocketStore();

const { menu, dialog, notifications } = storeToRefs(appStore);
const { socket, isConnect } = storeToRefs(webSocket);
const { main } = storeToRefs(webSocketStore);

const drawer = ref(false);
const isIframe = ref(false);
const sidebarToggle = ref(false);

let ping: ReturnType<typeof setInterval> | null = null;
let messageListener: ((event: MessageEvent) => void) | null = null;

const route = useRoute();
const router = useRouter();

provide(DialogKey, appStore.setDialog);
provide(NotificationKey, appStore.setNotification);

const mode = import.meta.env.MODE;
const proxy = import.meta.env.VITE_PROXY;

const host = mode === 'production' ? window.location.host : proxy;

const fullPath = computed(() => route.fullPath);
const nameDevice = computed(() => main.value?.info?.name || 'Device');

const connect = () => {
  const instance: WebSocket = new WebSocket(`ws://${host}/esp`);
  instance.binaryType = 'arraybuffer';
  instance.onopen = webSocket.onopen;
  instance.onmessage = webSocket.onmessage;
  instance.onclose = (e: CloseEvent) => {
    webSocket.onclose(e);
    if (e.code !== 1000) {
      setTimeout(connect, 2000);
    }
  };
  instance.onerror = webSocket.onerror;
  socket.value = instance;
};

const onClose = () => {
  drawer.value = false;
  dialog.value = {};
};

const onNotifications = (item: INotificationItem) => {
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

    messageListener = (event: MessageEvent) => {
      console.log('Data:', event);
      if (event?.data?.type === 'theme') {
        appStore.changeTheme(event.data.value);
      }
      if (event?.data?.type === 'route') {
        const data = event.data.data;
        router.push(data);
      }
    };
    window.addEventListener('message', messageListener);
  }
});

onUnmounted(() => {
  if (ping) clearInterval(ping);
  if (socket.value) socket.value.close(1000);
  if (messageListener) window.removeEventListener('message', messageListener);
});
</script>
