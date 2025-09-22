import { onMounted, ref } from 'vue';
import { storeToRefs } from 'pinia';
import { event } from '@/assets/js/';

import { useWebSocketStore } from '@/stores/WebSocketStore.ts';
import type { TypeMessage } from '@/types';

export const useConnection = (init: (send: (comm: string, data?: any) => void) => void, messages?: (data: TypeMessage) => void) => {
  console.log('useConnection');

  const webSocketStore = useWebSocketStore();
  const { ...arg } = storeToRefs(webSocketStore);


  const isInit = ref(false);
  const isConnected = ref(false);

  event.on('init', () => {
    isInit.value = true;
    init(webSocketStore.onSend);
  });

  if (messages) {
    event.on('messages', (data: TypeMessage) => {
      messages(data);
    });
  }

  event.on('init', () => {
    isInit.value = true;
    init(webSocketStore.onSend);
  });

  event.on('connected', (value) => {
    isConnected.value = value;
  });

  const onSend = webSocketStore.onSend;

  onMounted(() => {
    init(webSocketStore.onSend);
  });

  return { ...arg, isInit, isConnected, onSend };
};
