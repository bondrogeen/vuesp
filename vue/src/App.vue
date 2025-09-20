<template>
  <div class="h-[100dvh] min-h-full flex flex-col">
    <app-overlay v-if="!isConnect">
      <div class="text-2xl font-bold mb-4">Disconnected</div>

      <div class="flex justify-center">
        <v-loader class="text-primary"></v-loader>
      </div>
    </app-overlay>

    <div class="flex h-screen overflow-hidden">
      <app-aside v-if="!isIframe" :isSidebar="isSidebar" @sidebar="onSidebar">
        <h3 class="mb-4 text-xs h-5 uppercase flex items-center gap-2 justify-between" :class="isSidebar ? 'lg:hidden' : ''">
          <span v-if="nameDevice" class="text-gray-400 text-nowrap">{{ nameDevice }}</span>
        </h3>

        <app-nav :menu="menu" :isSidebar="isSidebar" :fullPath="fullPath" @sidebar="onSidebar" />

        <div class="flex-auto"></div>

        <BlockStatus v-bind="main.info" :pkg="pkg" class="mb-4 w-full rounded-2xl bg-gray-100 px-4 py-4 text-center dark:bg-white/[0.03]" :class="isSidebar ? 'lg:hidden' : ''" />
      </app-aside>

      <div class="relative flex flex-1 flex-col overflow-y-auto overflow-x-hidden scrollbar">
        <app-header v-if="!isIframe" :change-theme="appStore.changeTheme" @sidebar="isSidebar = !isSidebar" />

        <app-notification class="fixed right-4 md:right-10 lg:right-20 top-20 z-20" :notifications="notifications" @close="onNotifications" />

        <main :class="isIframe ? 'no-scrollbar' : 'px-4 py-6 sm:px-6 lg:px-8 flex-auto'">
          <div :class="isIframe ? '' : 'container mx-auto'">
            <router-view />
          </div>
        </main>
      </div>
    </div>

    <app-dialog v-if="dialog.value" v-bind="dialog" @close="dialog = {}" />

    <app-dialog v-if="dialogInfo" size="md" title="General information" @close="dialogInfo = false">
      <BlockInfo v-bind="main.info" :pkg="pkg" class="w-full" />
    </app-dialog>
  </div>
</template>

<script setup lang="ts">
import type { IStorePackage, INotificationItem } from 'vuesp-components/types';

import { ref, onMounted, onUnmounted, provide, computed, inject } from 'vue';
import { useRoute, useRouter } from 'vue-router';

import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore';
import { useWebSocket } from '@/stores/WebSocket';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import { BlockStatus, BlockInfo } from 'vuesp-components';

import { PKGKey, DialogKey, NotificationKey } from '@/utils/simbol';

const pkg: IStorePackage | undefined = inject(PKGKey);

const appStore = useAppStore();
const webSocket = useWebSocket();
const webSocketStore = useWebSocketStore();

const { menu, dialog, notifications, dialogInfo } = storeToRefs(appStore);
const { socket, isConnect } = storeToRefs(webSocket);
const { main } = storeToRefs(webSocketStore);

const isIframe = ref(false);
const isSidebar = ref(false);

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

const onSidebar = (value: boolean) => (isSidebar.value = value);

const onNotifications = (item: INotificationItem) => {
  notifications.value = notifications.value.filter((i) => i.id !== item.id);
};

onMounted(async () => {
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
