import { computed, ref, shallowRef } from 'vue';
import type { CompileResult, CapabilityManifest } from '@/lib/script';
import { compile, DEFAULT_MANIFEST } from '@/lib/script';
import { useBytecodeApi } from './useBytecodeApi';
import { hexDump } from '@/utils/scriptStudio';

export const useBytecodeCompile = () => {
  const api = useBytecodeApi();
  const manifest = ref<CapabilityManifest>(DEFAULT_MANIFEST);
  const result = shallowRef<CompileResult | null>(null);
  const isVmAvailable = ref(true);

  const diagnostics = computed(() => result.value?.errors ?? []);
  const hex = computed(() => (result.value?.bytecode ? hexDump(result.value.bytecode) : ''));
  const stackMax = computed(() => result.value?.stats?.stackMax ?? null);
  const byteCount = computed(() => result.value?.bytecode?.length ?? 0);

  const loadCapabilities = async (): Promise<boolean> => {
    try {
      const capabilities = await api.getCapabilities();
      isVmAvailable.value = capabilities !== null;
      manifest.value = capabilities ?? DEFAULT_MANIFEST;
    } catch {
      isVmAvailable.value = false;
      manifest.value = DEFAULT_MANIFEST;
    }
    return isVmAvailable.value;
  };

  const compileSource = (source: string): CompileResult => {
    result.value = compile(source, { manifest: manifest.value });
    return result.value;
  };

  const resetResult = () => {
    result.value = null;
  };

  const getSourceLines = (source: string): number => source.split('\n').length;

  return {
    manifest,
    result,
    isVmAvailable,
    diagnostics,
    hex,
    stackMax,
    byteCount,
    loadCapabilities,
    compileSource,
    resetResult,
    getSourceLines,
  };
};
