import { onMounted, ref } from 'vue';
import { storeToRefs } from 'pinia';
import { event } from '@/assets/js/';
import Queue from '@/assets/js/queue';

import { useAppStore } from '@/stores/AppStore';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import type { TypeMessage, TypeSend } from '@/types';

export const useConnection = (init?: (send: TypeSend) => void, messages?: (message: TypeMessage) => void) => {
  const webSocketStore = useWebSocketStore();
  const { ...socket } = storeToRefs(webSocketStore);

  const appStore = useAppStore();
  const { ...app } = storeToRefs(appStore);

  const isConnected = ref(false);

  const queue = new Queue<{ key: TypeMessage['key']; object?: TypeMessage['object'] }>(100, ({ key, object }) => {
    webSocketStore.onSend(key, object);
  });

  const onSend = (key: TypeMessage['key'], object?: TypeMessage['object']) => {
    queue.add({ key, object });
  };

  event.on('init', () => {
    if (init) init(onSend);
  });

  if (messages) {
    event.on('messages', (message: TypeMessage) => {
      messages(message);
    });
  }

  event.on('connected', (value) => {
    isConnected.value = value;
  });

  const onDialog = appStore.setDialog;

  onMounted(() => {
    if (init) init(onSend);
  });

  return { ...socket, ...app, isConnected, onSend, onDialog };
};
