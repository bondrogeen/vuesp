import { onMounted, onUnmounted } from 'vue';

import { storeToRefs } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';

export const useSocket = () => {
  const webSocket = useWebSocket();
  const { socket, isConnect } = storeToRefs(webSocket);

  let ping: ReturnType<typeof setInterval> | null = null;

  const connect = () => {
    const instance: WebSocket = new WebSocket(`/esp`);
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

  onMounted(async () => {
    ping = setInterval(webSocket.onPing, 1000);
    setTimeout(connect, 100);
  });

  onUnmounted(() => {
    if (ping) clearInterval(ping);
    if (socket.value) socket.value.close(1000);
  });
  return { isConnect };
};
