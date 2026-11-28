<script setup lang="ts">
import { useDebounceFn } from '@vueuse/core';
import { computed, nextTick, onBeforeUnmount, onMounted, ref, useTemplateRef, watch } from 'vue';
import { VCard } from 'vuesp-components/components';

import BytecodeEditor from '@/components/BytecodeEditor.vue';
import ScriptStudioToolbar from '@/components/script-studio/ScriptStudioToolbar.vue';
import ScriptStudioMetrics from '@/components/script-studio/ScriptStudioMetrics.vue';
import ScriptStudioActionBar from '@/components/script-studio/ScriptStudioActionBar.vue';
import ScriptStudioTabs from '@/components/script-studio/ScriptStudioTabs.vue';
import ScriptStudioLogs from '@/components/script-studio/ScriptStudioLogs.vue';
import ScriptDetailsDialog from '@/components/script-studio/ScriptDetailsDialog.vue';
import ScriptDeleteDialog from '@/components/script-studio/ScriptDeleteDialog.vue';
import ScriptRenameDialog from '@/components/script-studio/ScriptRenameDialog.vue';
import ScriptDiscardDialog from '@/components/script-studio/ScriptDiscardDialog.vue';

import { useLocale } from '@/composables/useLocale';
import { useStore } from '@/composables/useStore';
import { useBytecodeCompile } from '@/composables/script/useBytecodeCompile';
import { useBytecodeScripts } from '@/composables/script/useBytecodeScripts';
import { useProjectConfig } from '@/composables/useProjectConfig';
import type { IRuntimeMetric, IScriptListItem } from '@/types';
import {
  SCRIPT_STATE,
  downloadBlob,
  getMessageText,
  getPackErrorName,
  getVmErrorName,
  isScriptActive,
  isScriptError,
  isValidScriptName,
  SCRIPT_NAME_MAX_LENGTH,
} from '@/utils/scriptStudio';

const DEFAULT_SOURCE = `$u0 = 5;
log('hello', $u0);
$display = 'BC-OK';
$p13 = 128;
wait(100ms);
$p13 = 0;
`;

const { $t } = useLocale();
const { suggestions } = useStore();
const projectConfig = useProjectConfig();

const { manifest, result, isVmAvailable, diagnostics, hex, stackMax, byteCount, loadCapabilities, compileSource, resetResult, getSourceLines } = useBytecodeCompile();

const { scripts, runtime, isBusy, onRefresh, onSave, onRemove, onRun, onStop, onStopAll, onLoad } = useBytecodeScripts(manifest);

const source = ref(DEFAULT_SOURCE);
const name = ref('boot');
const savedSource = ref('');
const savedName = ref('');
const message = ref('');
const logsCount = ref(0);
const isSettingsOpen = ref(false);
const isRenameOpen = ref(false);
const isDetailsOpen = ref(false);
const deleteTarget = ref<string | null>(null);
const discardAction = ref<TDiscardAction | null>(null);

type TDiscardAction =
  | { type: 'select'; name: string }
  | { type: 'run'; name: string }
  | { type: 'reload' }
  | { type: 'new' }
  | { type: 'import'; fileName: string; source: string };

const logsRef = useTemplateRef<InstanceType<typeof ScriptStudioLogs>>('logsRef');
const importInput = useTemplateRef<HTMLInputElement>('importInput');
const editorRef = useTemplateRef<InstanceType<typeof BytecodeEditor>>('editor');

const currentEntry = computed(() => scripts.value.find((script) => script.name === name.value) ?? null);
const isDirty = computed(() => source.value !== savedSource.value || name.value !== savedName.value);
const draftName = computed(() => (currentEntry.value ? null : name.value));
const scriptNames = computed(() => scripts.value.map((script) => script.name));
const isDownloadAvailable = computed(() => Boolean(source.value.trim()));
const lineCount = computed(() => getSourceLines(source.value));
const maxScripts = computed(() => manifest.value.maxScripts ?? projectConfig.maxSavedScripts);

const ranName = ref<string | null>(null);
let ranTimer: ReturnType<typeof setTimeout> | null = null;

const flashRan = (scriptName: string) => {
  ranName.value = scriptName;
  if (ranTimer) clearTimeout(ranTimer);
  ranTimer = setTimeout(() => {
    ranName.value = null;
  }, 2500);
};

const loadedCount = computed(
  () => scripts.value.filter((script) => (script.state & SCRIPT_STATE.LOADED) !== 0).length,
);

const metrics = computed<IRuntimeMetric[]>(() => {
  const value = runtime.value;
  const packName = getPackErrorName(value?.packError ?? 0);
  const errorName = value?.error ? getVmErrorName(value.error) : $t('scriptStudio.noError');
  const errorScript = scripts.value.find((script) => isScriptError(script.state))?.name ?? '';
  const errorText = [errorName, errorScript].filter(Boolean).join(' · ');
  return [
    { key: 'scripts', label: $t('scriptStudio.savedCount'), value: `${scripts.value.length}/${maxScripts.value}` },
    { key: 'cacheSlots', label: $t('scriptStudio.cacheSlots'), value: String(manifest.value.cacheSlots ?? projectConfig.cacheSlots) },
    { key: 'vm', label: $t('scriptStudio.vm'), value: `v${manifest.value.vm}` },
    { key: 'pack', label: $t('scriptStudio.pack'), value: packName, isError: Boolean(value?.packError) },
    {
      key: 'lastError',
      label: $t('scriptStudio.lastError'),
      value: errorText,
      isError: Boolean(value?.error),
      title: errorText,
    },
    { key: 'errorPc', label: $t('scriptStudio.errorPc'), value: String(value?.errorPc ?? 0) },
    { key: 'vmScripts', label: $t('scriptStudio.vmScripts'), value: String(loadedCount.value) },
  ];
});

const onCompile = (isSilent = false) => {
  const compiled = compileSource(source.value);
  if (isSilent) return;
  message.value = compiled.ok ? `${$t('scriptStudio.compiled')}: ${compiled.bytecode?.length ?? 0} ${$t('scriptStudio.bytes')}, ${$t('scriptStudio.stack')} ${compiled.stats?.stackMax ?? '?'}` : '';
};

const debouncedCompile = useDebounceFn(() => onCompile(true), 350);

const onRefreshList = async () => {
  try {
    await onRefresh();
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onLoadScript = async (scriptName: string) => {
  try {
    const payload = await onLoad(scriptName);
    if (!payload) return;
    name.value = payload.name;
    source.value = payload.source;
    savedSource.value = payload.source;
    savedName.value = payload.name;
    result.value = {
      ok: true,
      errors: [],
      warnings: [],
      bytecode: payload.bytes,
      disassembly: payload.disassembly,
      source: payload.source,
    };
    message.value = $t('scriptStudio.decompiled');
    ranName.value = null;
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onSelectScript = async (script: IScriptListItem) => {
  if (script.name === name.value) return;
  if (isDirty.value) {
    discardAction.value = { type: 'select', name: script.name };
    return;
  }
  await onLoadScript(script.name);
};

const onReloadCurrent = async () => {
  if (!currentEntry.value) return;
  if (isDirty.value) {
    discardAction.value = { type: 'reload' };
    return;
  }
  await onLoadScript(name.value);
};

const onSaveScript = async (isRunAfterSave: boolean): Promise<boolean> => {
  onCompile(true);
  const compiled = result.value;
  if (!compiled?.ok || !compiled.bytecode) return false;
  const targetName = name.value;
  const targetSource = source.value;
  try {
    const isSaved = await onSave(targetName, compiled.bytecode, isRunAfterSave);
    if (!isSaved) return false;
    if (name.value === targetName) {
      savedName.value = targetName;
      savedSource.value = targetSource;
    }
    message.value = isRunAfterSave ? $t('scriptStudio.started') : $t('scriptStudio.saved');
    await onRefreshList();
    if (isRunAfterSave) {
      const entry = scripts.value.find((script) => script.name === targetName);
      if (!entry || !isScriptActive(entry.state)) flashRan(targetName);
    }
    return true;
  } catch (error) {
    message.value = getMessageText(error);
    return false;
  }
};

const onRunNamed = async (scriptName: string) => {
  try {
    if (!(await onRun(scriptName))) return;
    message.value = $t('scriptStudio.started');
    await onRefreshList();
    const entry = scripts.value.find((script) => script.name === scriptName);
    if (!entry || !isScriptActive(entry.state)) flashRan(scriptName);
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onStopNamed = async (scriptName: string) => {
  try {
    if (!(await onStop(scriptName))) return;
    message.value = $t('scriptStudio.stopped');
    if (ranName.value === scriptName) ranName.value = null;
    await onRefreshList();
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onToggleRun = async (script: IScriptListItem) => {
  if (isScriptActive(script.state)) {
    await onStopNamed(script.name);
    return;
  }
  if (script.name === name.value && isDirty.value) {
    await onSaveScript(true);
    return;
  }
  if (script.name !== name.value) {
    if (isDirty.value) {
      discardAction.value = { type: 'run', name: script.name };
      return;
    }
    await onLoadScript(script.name);
    if (name.value !== script.name) return;
  }
  await onRunNamed(script.name);
};

const onStopAllScripts = async () => {
  try {
    if (!(await onStopAll())) return;
    message.value = $t('scriptStudio.stopped');
    ranName.value = null;
    await onRefreshList();
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const createNewScript = () => {
  let index = 1;
  while (scripts.value.some((script) => script.name === `script${index}`)) index += 1;
  name.value = `script${index}`;
  source.value = DEFAULT_SOURCE;
  savedSource.value = '';
  savedName.value = '';
  resetResult();
  ranName.value = null;
  message.value = $t('scriptStudio.newScript');
  onCompile(true);
  void nextTick(() => editorRef.value?.focus());
};

const onNewScript = () => {
  if (isDirty.value) {
    discardAction.value = { type: 'new' };
    return;
  }
  createNewScript();
};

const onAskDelete = (scriptName: string) => {
  deleteTarget.value = scriptName;
};

const onConfirmDelete = async () => {
  const target = deleteTarget.value;
  if (!target || isBusy.value) return;
  try {
    if (!(await onRemove(target))) return;
    deleteTarget.value = null;
    message.value = $t('scriptStudio.deleted');
    if (name.value === target) {
      const next = scripts.value.find((script) => script.name !== target);
      if (next) await onLoadScript(next.name);
      else createNewScript();
    }
    await onRefreshList();
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const scriptNameFromFile = (fileName: string): string => {
  const base = fileName.replace(/\.[^.]+$/, '').toLowerCase();
  let candidate = base
    .replace(/[^a-z0-9_-]/g, '_')
    .replace(/^[^a-z_]/, '_')
    .slice(0, SCRIPT_NAME_MAX_LENGTH);
  if (!isValidScriptName(candidate)) candidate = 'imported';
  if (scripts.value.some((script) => script.name === candidate)) {
    let index = 2;
    let next = candidate;
    while (scripts.value.some((script) => script.name === next)) {
      const suffix = String(index);
      next = `${candidate.slice(0, SCRIPT_NAME_MAX_LENGTH - suffix.length)}${suffix}`;
      index += 1;
    }
    candidate = next;
  }
  return candidate;
};

const applyImported = (importedName: string, text: string) => {
  name.value = importedName;
  source.value = text;
  savedSource.value = '';
  savedName.value = '';
  resetResult();
  ranName.value = null;
  message.value = $t('scriptStudio.imported');
  onCompile(true);
  void nextTick(() => editorRef.value?.focus());
};

const onImportClick = () => {
  importInput.value?.click();
};

const onImportFile = async (event: Event) => {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  input.value = '';
  if (!file) return;
  const text = await file.text();
  if (!text.trim()) {
    message.value = $t('scriptStudio.importEmpty');
    return;
  }
  if (isDirty.value) {
    discardAction.value = { type: 'import', fileName: file.name, source: text };
    return;
  }
  applyImported(scriptNameFromFile(file.name), text);
};

const runDiscardAction = async () => {
  const action = discardAction.value;
  discardAction.value = null;
  if (!action) return;
  if (action.type === 'select') await onLoadScript(action.name);
  else if (action.type === 'run') {
    await onLoadScript(action.name);
    if (name.value === action.name) await onRunNamed(action.name);
  } else if (action.type === 'reload') await onLoadScript(name.value);
  else if (action.type === 'new') createNewScript();
  else if (action.type === 'import') applyImported(scriptNameFromFile(action.fileName), action.source);
};

const onDiscardSave = async () => {
  if (!(await onSaveScript(false))) return;
  await runDiscardAction();
};

const onDiscardDiscard = async () => {
  await runDiscardAction();
};

const onDownload = () => {
  if (!source.value.trim()) return;
  downloadBlob(source.value, `${name.value || 'script'}.js`, 'text/javascript');
};

const onRenameSave = (nextName: string) => {
  if (scripts.value.some((script) => script.name === nextName)) {
    message.value = $t('scriptStudio.nameTaken');
    return;
  }
  isRenameOpen.value = false;
  if (nextName === name.value) return;
  name.value = nextName;
  message.value = $t('scriptStudio.newScript');
};

const onShowLogs = () => {
  logsRef.value?.open();
};

const onUpdateLogsCount = (count: number) => {
  logsCount.value = count;
};

watch(source, () => {
  ranName.value = null;
  void debouncedCompile();
});

onBeforeUnmount(() => {
  if (ranTimer) clearTimeout(ranTimer);
});

onMounted(async () => {
  const isAvailable = await loadCapabilities();
  if (!isAvailable) {
    message.value = $t('scriptStudio.vmUnavailableHint');
    onCompile(true);
    return;
  }
  await onRefreshList();
  const initial = scripts.value.find((script) => script.name === name.value) ?? scripts.value[0];
  if (initial) await onLoadScript(initial.name);
  else onCompile(true);
});
</script>

<template>
  <VCard variant="glass" padding="none" class="flex min-w-0 flex-col">
    <ScriptStudioToolbar
      :name="name"
      :is-busy="isBusy"
      :is-vm-available="isVmAvailable"
      :is-settings-open="isSettingsOpen"
      :is-download-available="isDownloadAvailable"
      :is-dirty="isDirty"
      @save="onSaveScript(false)"
      @rename="isRenameOpen = true"
      @reload="onReloadCurrent"
      @import="onImportClick"
      @download="onDownload"
      @stop-all="onStopAllScripts"
      @toggle-settings="isSettingsOpen = !isSettingsOpen"
    />

    <ScriptStudioMetrics
      v-if="isSettingsOpen"
      :metrics="metrics"
    >
      <ScriptStudioActionBar
        :logs-count="logsCount"
        :line-count="lineCount"
        :byte-count="byteCount"
        :stack-max="stackMax"
        @details="isDetailsOpen = true"
        @show-logs="onShowLogs"
      />
    </ScriptStudioMetrics>

    <ScriptStudioTabs
      :scripts="scripts"
      :active-name="name"
      :draft-name="draftName"
      :is-busy="isBusy"
      :is-dirty="isDirty"
      :max-scripts="maxScripts"
      :ran-name="ranName"
      @select="onSelectScript"
      @toggle-run="onToggleRun"
      @remove="onAskDelete"
      @create="onNewScript"
    />

    <BytecodeEditor ref="editor" v-model="source" :manifest="manifest" :script-names="scriptNames" :errors="diagnostics" :device-suggestions="suggestions" />

    <ScriptStudioLogs ref="logsRef" :name="name" @update:count="onUpdateLogsCount" />
  </VCard>

  <ScriptDetailsDialog v-if="isDetailsOpen" :result="result" :hex="hex" @close="isDetailsOpen = false" />
  <ScriptDeleteDialog v-if="deleteTarget" :name="deleteTarget" :is-busy="isBusy" @confirm="onConfirmDelete" @close="deleteTarget = null" />
  <ScriptRenameDialog v-if="isRenameOpen" :name="name" :is-busy="isBusy" @save="onRenameSave" @close="isRenameOpen = false" />
  <ScriptDiscardDialog v-if="discardAction" @save="onDiscardSave" @discard="onDiscardDiscard" @cancel="discardAction = null" />
  <input ref="importInput" type="file" accept=".js,.txt,.vjs" class="hidden" @change="onImportFile" />
</template>
