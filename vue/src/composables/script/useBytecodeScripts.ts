import { onBeforeUnmount, onMounted, ref } from 'vue';
import type { Ref } from 'vue';
import { event } from '@/assets/js/';
import { decompile, disassemble } from '@/lib/script';
import type { CapabilityManifest } from '@/lib/script';
import { SCRIPTACT, SCRIPTRES } from '@/generated/protocol';
import type { IScriptListItem, IScriptPayload, IScriptStatus } from '@/types';
import { getScriptResultName } from '@/utils/scriptStudio';
import { useBytecodeApi } from './useBytecodeApi';
import { useScriptSocket } from './useScriptSocket';

export const useBytecodeScripts = (manifest: Ref<CapabilityManifest>) => {
  const api = useBytecodeApi();
  const socket = useScriptSocket();
  const scripts = ref<IScriptListItem[]>([]);
  const runtime = ref<IScriptStatus | null>(null);
  const isBusy = ref(false);
  let refreshSeq = 0;

  socket.onStatus((status) => {
    runtime.value = status;
    if (!status.name) return;
    const script = scripts.value.find((item) => item.name === status.name);
    if (script) script.state = status.state;
  });

  const onConnected = (value: unknown) => {
    if (value === true) {
      void socket.send(SCRIPTACT.STATUS).catch(() => {
        /* device may not have answered yet */
      });
    }
  };

  onMounted(() => {
    event.on('connected', onConnected);
    void socket.send(SCRIPTACT.STATUS).catch(() => {
      /* device may not have answered yet */
    });
  });

  onBeforeUnmount(() => event.off('connected', onConnected));

  const onRefresh = async (): Promise<void> => {
    const seq = ++refreshSeq;
    const list = await api.getList();
    if (seq === refreshSeq) scripts.value = list;
  };

  const assertOk = (status: IScriptStatus) => {
    if (status.result !== SCRIPTRES.OK) {
      throw new Error(getScriptResultName(status.result));
    }
  };

  const onSave = async (
    name: string,
    bytes: Uint8Array,
    isRunAfterSave: boolean,
  ): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      const requestId = socket.nextRequestId();
      const pending = socket.waitFor(requestId);
      pending.catch(() => {
        /* the HTTP call failed before the status arrived */
      });
      try {
        await api.uploadScript(name, bytes, requestId, isRunAfterSave);
      } catch (error) {
        socket.cancel(requestId);
        throw error;
      }
      assertOk(await pending);
      return true;
    } finally {
      isBusy.value = false;
    }
  };

  const onRemove = async (name: string): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      assertOk(await socket.send(SCRIPTACT.REMOVE, name));
      return true;
    } finally {
      isBusy.value = false;
    }
  };

  const onRun = async (name: string): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      assertOk(await socket.send(SCRIPTACT.RUN, name));
      return true;
    } finally {
      isBusy.value = false;
    }
  };

  const onStop = async (name: string): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      assertOk(await socket.send(SCRIPTACT.STOP, name));
      return true;
    } finally {
      isBusy.value = false;
    }
  };

  const onStopAll = async (): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      assertOk(await socket.send(SCRIPTACT.STOP_ALL));
      return true;
    } finally {
      isBusy.value = false;
    }
  };

  const onLoad = async (name: string): Promise<IScriptPayload | null> => {
    if (isBusy.value) return null;
    isBusy.value = true;
    try {
      const bytes = await api.getBytecode(name);
      const decoded = decompile(bytes, { manifest: manifest.value });
      if (!decoded.ok || !decoded.source) {
        const message = decoded.errors.map((error) => error.message).join('\n');
        throw new Error(message || 'decompile failed');
      }
      return { name, bytes, source: decoded.source, disassembly: disassemble(bytes) };
    } finally {
      isBusy.value = false;
    }
  };

  return {
    scripts,
    runtime,
    isBusy,
    onRefresh,
    onSave,
    onRemove,
    onRun,
    onStop,
    onStopAll,
    onLoad,
  };
};
