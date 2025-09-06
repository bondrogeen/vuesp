import { onMounted, ref } from 'vue';
import event from '@/assets/js/event';

export const useConnection = (initData: () => void) => {
  const isInit = ref(false);
  const isConnected = ref(false);

  event.on('init', () => {
    isInit.value = true;
    initData();
  });

  event.on('connected', (value) => {
    isConnected.value = value;
  });

  onMounted(() => {
    initData();
  });

  return { isInit, isConnected };
};
