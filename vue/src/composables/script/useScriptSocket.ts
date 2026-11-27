import { onBeforeUnmount, onMounted } from 'vue';

import { event } from '@/assets/js/';
import { useStore } from '@/composables/useStore';
import { KEY } from '@/generated/protocol';
import type { IScriptStatus, TMessage } from '@/types';

const REQUEST_TIMEOUT_MS = 5000;

interface IPending {
  resolve: (status: IScriptStatus) => void;
  reject: (error: Error) => void;
  timer: ReturnType<typeof setTimeout>;
}

export const useScriptSocket = () => {
  const { wsSend } = useStore();
  const pending = new Map<number, IPending>();
  const listeners = new Set<(status: IScriptStatus) => void>();
  let nextId = Math.floor(Math.random() * 0xffff) + 1;

  const onMessage = (message: TMessage) => {
    if (message.key !== KEY.SCRIPTSTATUS || !message.object) return;
    const status = message.object as unknown as IScriptStatus;
    for (const listener of listeners) listener(status);
    if (!status.requestId) return;
    const item = pending.get(status.requestId);
    if (!item) return;
    pending.delete(status.requestId);
    clearTimeout(item.timer);
    item.resolve(status);
  };

  onMounted(() => {
    event.on('messages', onMessage as (value: unknown) => void);
  });

  onBeforeUnmount(() => {
    event.off('messages', onMessage as (value: unknown) => void);
    for (const item of pending.values()) {
      clearTimeout(item.timer);
      item.reject(new Error('script socket closed'));
    }
    pending.clear();
  });

  const nextRequestId = (): number => {
    const id = nextId;
    nextId = nextId >= 0xffff ? 1 : nextId + 1;
    return id;
  };

  const waitFor = (requestId: number): Promise<IScriptStatus> =>
    new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        pending.delete(requestId);
        reject(new Error('script request timeout'));
      }, REQUEST_TIMEOUT_MS);
      pending.set(requestId, { resolve, reject, timer });
    });

  const send = (action: number, name = '', flags = 0): Promise<IScriptStatus> => {
    const requestId = nextRequestId();
    const result = waitFor(requestId);
    wsSend(KEY.SCRIPTCMD, { action, flags, requestId, name });
    return result;
  };

  const cancel = (requestId: number) => {
    const item = pending.get(requestId);
    if (!item) return;
    clearTimeout(item.timer);
    pending.delete(requestId);
  };

  const onStatus = (listener: (status: IScriptStatus) => void): (() => void) => {
    listeners.add(listener);
    return () => listeners.delete(listener);
  };

  return { send, nextRequestId, waitFor, cancel, onStatus };
};
