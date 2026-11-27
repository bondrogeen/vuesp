import type { CapabilityManifest } from '@/lib/script';
import type { IScriptListItem, IScriptStatus } from '@/types';
import { toArrayBuffer } from '@/utils/scriptStudio';

const REQUEST_TIMEOUT_MS = 8000;
const JSON_TYPE = 'application/json';
const BINARY_TYPE = 'application/octet-stream';

export class BytecodeApiError extends Error {
  readonly status: number;

  constructor(message: string, status = 0) {
    super(message);
    this.name = 'BytecodeApiError';
    this.status = status;
  }
}

const withTimeout = (init?: RequestInit): RequestInit => ({
  ...init,
  signal: AbortSignal.timeout(REQUEST_TIMEOUT_MS),
});

const isJsonResponse = (response: Response): boolean =>
  (response.headers.get('content-type') ?? '').includes(JSON_TYPE);

const readFailure = async (response: Response): Promise<string> => {
  if (isJsonResponse(response)) {
    try {
      const payload = (await response.json()) as { state?: boolean; error?: string };
      if (payload.error) return payload.error;
    } catch {
      /* fall through to the status text */
    }
  }
  return `HTTP ${response.status}`;
};

const requestJson = async <T>(url: string, init?: RequestInit): Promise<T> => {
  const response = await fetch(url, withTimeout(init));
  if (!response.ok || !isJsonResponse(response)) {
    throw new BytecodeApiError(await readFailure(response), response.status);
  }
  return (await response.json()) as T;
};

const requestBinary = async (url: string): Promise<Uint8Array> => {
  const response = await fetch(url, withTimeout());
  const contentType = response.headers.get('content-type') ?? '';
  if (!response.ok || !contentType.includes(BINARY_TYPE)) {
    throw new BytecodeApiError(await readFailure(response), response.status);
  }
  return new Uint8Array(await response.arrayBuffer());
};

export const useBytecodeApi = () => {
  const getCapabilities = async (): Promise<CapabilityManifest | null> => {
    const response = await fetch('/script/capabilities', withTimeout());
    if (!response.ok || !isJsonResponse(response)) return null;
    return (await response.json()) as CapabilityManifest;
  };

  const getList = (): Promise<IScriptListItem[]> => requestJson<IScriptListItem[]>('/script/list');

  const getStatus = (): Promise<IScriptStatus> => requestJson<IScriptStatus>('/script/status');

  const getBytecode = (name: string): Promise<Uint8Array> =>
    requestBinary(`/script/bytecode?name=${encodeURIComponent(name)}`);

  const saveScript = (name: string, bytes: Uint8Array, isRunAfterSave: boolean): Promise<unknown> =>
    requestJson(`/script/bytecode?name=${encodeURIComponent(name)}&run=${isRunAfterSave ? 1 : 0}`, {
      method: 'POST',
      headers: { 'Content-Type': BINARY_TYPE },
      body: toArrayBuffer(bytes),
    });

  const removeScript = (name: string): Promise<unknown> =>
    requestJson(`/script/bytecode?name=${encodeURIComponent(name)}`, { method: 'DELETE' });

  const runScript = (name: string): Promise<unknown> =>
    requestJson(`/script/run?name=${encodeURIComponent(name)}`, { method: 'POST' });

  const stopScript = (name: string): Promise<unknown> =>
    requestJson(`/script/stop?name=${encodeURIComponent(name)}`, { method: 'POST' });

  const stopAll = (): Promise<unknown> => requestJson('/script/stop-all', { method: 'POST' });

  return {
    getCapabilities,
    getList,
    getStatus,
    getBytecode,
    saveScript,
    removeScript,
    runScript,
    stopScript,
    stopAll,
  };
};
