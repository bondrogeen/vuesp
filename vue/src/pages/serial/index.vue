<script setup lang="ts">
import { computed, ref } from 'vue';

import type { ISerialConsoleTransport, TSerialConsoleLocale, TSerialConsoleStatus } from 'vuesp-components/components';
import { SerialConsole } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';

const { $t, i18n } = useLocale();

/**
 * Example page: the demo transport below echoes everything back.
 * Replace it with a real transport:
 *  - device WebSocket (KEY.RS232 + CMD.WRITE / CMD.READ), or
 *  - Web Serial (navigator.serial) for a local USB port.
 */
const status = ref<TSerialConsoleStatus>('disconnected');
const listeners = new Set<(bytes: Uint8Array) => void>();
let echoTimer: ReturnType<typeof setTimeout> | null = null;

const emit = (bytes: Uint8Array): void => {
  for (const listener of listeners) listener(bytes);
};

const transport: ISerialConsoleTransport = {
  start: () => {
    status.value = 'connected';
    echoTimer = setTimeout(() => emit(new TextEncoder().encode('Demo console ready\r\n')), 150);
  },
  stop: () => {
    status.value = 'disconnected';
    if (echoTimer) clearTimeout(echoTimer);
    echoTimer = null;
  },
  send: async (bytes) => {
    if (status.value !== 'connected') return false;
    setTimeout(() => emit(bytes), 80);
    return true;
  },
  subscribe: (listener) => {
    listeners.add(listener);
    return () => listeners.delete(listener);
  },
};

const locale = computed<TSerialConsoleLocale>(() => (i18n.state.locale === 'ru' ? 'ru' : 'en'));
</script>

<template>
  <div class="flex flex-col gap-4">
    <SerialConsole
      :status="status"
      :port-label="$t('serial.demoPort')"
      :labels="{ usb: '' }"
      :transport="transport"
      :locale="locale"
      :initial-quick-commands="['PING', 'INFO']"
      initial-line-ending="crlf"
      storage-key="vuesp:serial:demo"
    >
      <template #settings-extra>
        <p class="text-[11px] text-gray-dark-25 dark:text-gray-main">{{ $t('serial.demoHint') }}</p>
      </template>
    </SerialConsole>
  </div>
</template>
