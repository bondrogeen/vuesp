import { onMounted, ref } from 'vue';
import { storeToRefs } from 'pinia';
import { event } from '@/assets/js/';

import { useAppStore } from '@/stores/AppStore.js';
import { useWebSocketStore } from '@/stores/WebSocketStore.ts';

import type { IDashboardItem, TypeSend } from '@/types';

import { getParamsData, getFunction } from 'vuesp-components/dashboard';

export const useConnection = () => {


  return {  };
};
