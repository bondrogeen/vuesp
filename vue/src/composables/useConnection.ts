import { onMounted } from 'vue';
import { event } from '@/assets/js/';
import Queue from '@/assets/js/queue';

import { useStore } from '@/composables/useStore';

import type { TypeMessage, TypeSend } from '@/types';

export const useConnection = (init?: (send: TypeSend) => void, messages?: (message: TypeMessage) => void) => {
  const store = useStore();

  const queue = new Queue<{ key: TypeMessage['key']; object?: TypeMessage['object'] }>(100, ({ key, object }) => {
    store.wsSend(key, object);
  });

  const onSend = (key: TypeMessage['key'], object?: TypeMessage['object']) => {
    queue.add({ key, object });
  };

  event.on('init', () => {
    if (!store.isInit.value && init) init(onSend);
  });

  if (messages) {
    event.on('messages', (message: TypeMessage) => {
      messages(message);
    });
  }

  onMounted(() => {
    if (store.isInit.value && init) init(onSend);
  });

  return { ...store, onSend };
};
