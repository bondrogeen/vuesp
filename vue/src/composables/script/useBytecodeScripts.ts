import { ref } from 'vue';
import type { Ref } from 'vue';
import { decompile, disassemble } from '@/lib/script';
import type { CapabilityManifest } from '@/lib/script';
import type { IScriptListItem, IScriptPayload, IScriptStatus } from '@/types';
import { useBytecodeApi } from './useBytecodeApi';

const POLL_INTERVAL_MS = 1500;

export const useBytecodeScripts = (manifest: Ref<CapabilityManifest>) => {
  const api = useBytecodeApi();
  const scripts = ref<IScriptListItem[]>([]);
  const runtime = ref<IScriptStatus | null>(null);
  const isBusy = ref(false);
  const isRefreshing = ref(false);
  let refreshSeq = 0;
  let pollTimer: ReturnType<typeof setInterval> | null = null;

  const onRefresh = async (): Promise<void> => {
    const seq = ++refreshSeq;
    isRefreshing.value = true;
    try {
      const [list, status] = await Promise.all([api.getList(), api.getStatus()]);
      if (seq !== refreshSeq) return;
      scripts.value = list;
      runtime.value = status;
    } finally {
      if (seq === refreshSeq) isRefreshing.value = false;
    }
  };

  const onSave = async (name: string, bytes: Uint8Array, isRunAfterSave: boolean): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      await api.saveScript(name, bytes, isRunAfterSave);
      return true;
    } finally {
      isBusy.value = false;
    }
  };

  const onRemove = async (name: string): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      await api.removeScript(name);
      return true;
    } finally {
      isBusy.value = false;
    }
  };

  const onRun = async (name: string): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      await api.runScript(name);
      return true;
    } finally {
      isBusy.value = false;
    }
  };

  const onStop = async (name: string): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      await api.stopScript(name);
      return true;
    } finally {
      isBusy.value = false;
    }
  };

  const onStopAll = async (): Promise<boolean> => {
    if (isBusy.value) return false;
    isBusy.value = true;
    try {
      await api.stopAll();
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

  const onStartPolling = (): void => {
    if (pollTimer) return;
    pollTimer = setInterval(() => {
      const isPaused = document.hidden || isRefreshing.value || isBusy.value;
      if (isPaused) return;
      void onRefresh().catch(() => {
        /* keep the previous view on transient errors */
      });
    }, POLL_INTERVAL_MS);
  };

  const onStopPolling = (): void => {
    if (!pollTimer) return;
    clearInterval(pollTimer);
    pollTimer = null;
  };

  return {
    scripts,
    runtime,
    isBusy,
    isRefreshing,
    onRefresh,
    onSave,
    onRemove,
    onRun,
    onStop,
    onStopAll,
    onLoad,
    onStartPolling,
    onStopPolling,
  };
};
