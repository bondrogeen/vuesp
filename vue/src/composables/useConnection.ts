import { onMounted, ref } from 'vue';
import { storeToRefs } from 'pinia';
import { event } from '@/assets/js/';

import { useAppStore } from '@/stores/AppStore.js';
import { useWebSocketStore } from '@/stores/WebSocketStore.ts';

import type { TypeMessage, TypeSend } from '@/types';

export const useConnection = (init?: (send: TypeSend) => void, messages?: (message: TypeMessage) => void) => {
  console.log('useConnection');

  const webSocketStore = useWebSocketStore();
  const { ...socket } = storeToRefs(webSocketStore);

  const appStore = useAppStore();
  const { ...app } = storeToRefs(appStore);

  const isConnected = ref(false);

  event.on('init', () => {
    if (init) init(webSocketStore.onSend);
  });

  if (messages) {
    event.on('messages', (message: TypeMessage) => {
      messages(message);
    });
  }

  event.on('connected', (value) => {
    isConnected.value = value;
  });

  const onSend = webSocketStore.onSend;

  onMounted(() => {
    if (init) init(webSocketStore.onSend);
  });

  return { ...socket, ...app, isConnected, onSend };
};
