<script setup lang="ts">
import { useDebounceFn } from '@vueuse/core';
import { computed, nextTick, onMounted, ref, useTemplateRef, watch } from 'vue';
import { VCard } from 'vuesp-components/components';

import BytecodeEditor from '@/components/BytecodeEditor.vue';
import ScriptStudioToolbar from '@/components/script-studio/ScriptStudioToolbar.vue';
import ScriptStudioMetrics from '@/components/script-studio/ScriptStudioMetrics.vue';
import ScriptStudioActionBar from '@/components/script-studio/ScriptStudioActionBar.vue';
import ScriptStudioTabs from '@/components/script-studio/ScriptStudioTabs.vue';
import ScriptStudioLogs from '@/components/script-studio/ScriptStudioLogs.vue';
import ScriptSnippetsDialog from '@/components/script-studio/ScriptSnippetsDialog.vue';
import ScriptExamplesDialog from '@/components/script-studio/ScriptExamplesDialog.vue';
import ScriptDetailsDialog from '@/components/script-studio/ScriptDetailsDialog.vue';
import ScriptDeleteDialog from '@/components/script-studio/ScriptDeleteDialog.vue';
import ScriptRenameDialog from '@/components/script-studio/ScriptRenameDialog.vue';

import { useLocale } from '@/composables/useLocale';
import { useStore } from '@/composables/useStore';
import { useBytecodeCompile } from '@/composables/script/useBytecodeCompile';
import { useBytecodeScripts } from '@/composables/script/useBytecodeScripts';
import { SCRIPT_LIMITS } from '@/generated/scriptManifest';
import { BYTECODE_EXAMPLES, getEventArgNames, normalizeSuggestionInsert } from '@/lib/script';
import type { BytecodeExample, EventSpec, FunctionSpec } from '@/lib/script';
import type { ISuggestion } from 'vuesp-components/types';
import type { IRuntimeMetric, IScriptListItem } from '@/types';
import {
  SCRIPT_STATE,
  downloadBlob,
  getMessageText,
  getPackErrorName,
  getScriptStatusMeta,
  getVmErrorName,
  isScriptActive,
  isScriptError,
  toArrayBuffer,
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
const isSnippetsOpen = ref(false);
const isExamplesOpen = ref(false);
const isDetailsOpen = ref(false);
const deleteTarget = ref<string | null>(null);

const logsRef = useTemplateRef<InstanceType<typeof ScriptStudioLogs>>('logsRef');
const editorRef = useTemplateRef<InstanceType<typeof BytecodeEditor>>('editor');

const currentEntry = computed(() => scripts.value.find((script) => script.name === name.value) ?? null);
const isDirty = computed(() => source.value !== savedSource.value || name.value !== savedName.value);
const currentStatus = computed(() => getScriptStatusMeta(currentEntry.value, $t));
const draftName = computed(() => (currentEntry.value ? null : name.value));
const scriptNames = computed(() => scripts.value.map((script) => script.name));
const isDownloadAvailable = computed(() => Boolean(result.value?.bytecode));
const lineCount = computed(() => getSourceLines(source.value));

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
    { key: 'cacheSlots', label: $t('scriptStudio.cacheSlots'), value: String(manifest.value.cacheSlots ?? SCRIPT_LIMITS.cacheSlots) },
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
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onSelectScript = async (script: IScriptListItem) => {
  if (script.name === name.value && !isDirty.value) return;
  if (isDirty.value && script.name !== name.value && !window.confirm($t('scriptStudio.discardChanges'))) {
    return;
  }
  await onLoadScript(script.name);
};

const onReloadCurrent = async () => {
  if (!currentEntry.value) return;
  await onLoadScript(name.value);
};

const onSaveScript = async (isRunAfterSave: boolean) => {
  onCompile(true);
  const compiled = result.value;
  if (!compiled?.ok || !compiled.bytecode) return;
  const targetName = name.value;
  const targetSource = source.value;
  try {
    const isSaved = await onSave(targetName, compiled.bytecode, isRunAfterSave);
    if (!isSaved) return;
    if (name.value === targetName) {
      savedName.value = targetName;
      savedSource.value = targetSource;
    }
    message.value = isRunAfterSave ? $t('scriptStudio.started') : $t('scriptStudio.saved');
    await onRefreshList();
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onRunNamed = async (scriptName: string) => {
  try {
    if (!(await onRun(scriptName))) return;
    message.value = $t('scriptStudio.started');
    await onRefreshList();
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onStopNamed = async (scriptName: string) => {
  try {
    if (!(await onStop(scriptName))) return;
    message.value = $t('scriptStudio.stopped');
    await onRefreshList();
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onToggleRun = async (script: IScriptListItem) => {
  if (isScriptActive(script.state)) await onStopNamed(script.name);
  else await onRunNamed(script.name);
};

const onStopAllScripts = async () => {
  try {
    if (!(await onStopAll())) return;
    message.value = $t('scriptStudio.stopped');
    await onRefreshList();
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onNewScript = () => {
  if (isDirty.value && !window.confirm($t('scriptStudio.discardChanges'))) return;
  let index = 1;
  while (scripts.value.some((script) => script.name === `script${index}`)) index += 1;
  name.value = `script${index}`;
  source.value = DEFAULT_SOURCE;
  savedSource.value = '';
  savedName.value = '';
  resetResult();
  message.value = $t('scriptStudio.newScript');
  onCompile(true);
  void nextTick(() => editorRef.value?.focus());
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
      else onNewScript();
    }
    await onRefreshList();
  } catch (error) {
    message.value = getMessageText(error);
  }
};

const onLoadExample = (example: BytecodeExample) => {
  if (isDirty.value && !window.confirm($t('scriptStudio.discardChanges'))) return;
  name.value = example.name;
  source.value = example.source;
  savedSource.value = '';
  savedName.value = '';
  resetResult();
  message.value = '';
  isExamplesOpen.value = false;
  void nextTick(() => editorRef.value?.focus());
};

const onInsertFunction = (fn: FunctionSpec) => {
  editorRef.value?.insertAtCursor(`${fn.name}()`, fn.name.length + 1);
  isSnippetsOpen.value = false;
};

const onInsertEvent = (event: EventSpec) => {
  const params = getEventArgNames(event.name);
  const snippet = params.length > 0
    ? `on('${event.name}', (${params.join(', ')}) => {\n  \n});\n`
    : `on('${event.name}') {\n  \n}\n`;
  editorRef.value?.insertAtCursor(snippet, snippet.indexOf('\n') + 3);
  isSnippetsOpen.value = false;
};

const onInsertSuggestion = (suggestion: ISuggestion) => {
  const { insert, caret } = normalizeSuggestionInsert(suggestion);
  editorRef.value?.insertAtCursor(insert, caret);
  isSnippetsOpen.value = false;
};

const onDownload = () => {
  const bytes = result.value?.bytecode;
  if (!bytes) return;
  downloadBlob(toArrayBuffer(bytes), `script-${name.value || 'script'}.vbc`, 'application/octet-stream');
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
  void debouncedCompile();
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
      :status="currentStatus"
      :is-busy="isBusy"
      :is-vm-available="isVmAvailable"
      :is-settings-open="isSettingsOpen"
      :is-download-available="isDownloadAvailable"
      :is-current-selected="Boolean(currentEntry)"
      @save="onSaveScript(false)"
      @rename="isRenameOpen = true"
      @reload="onReloadCurrent"
      @download="onDownload"
      @stop-all="onStopAllScripts"
      @remove="onAskDelete(name)"
      @toggle-settings="isSettingsOpen = !isSettingsOpen"
    />

    <ScriptStudioMetrics
      v-if="isSettingsOpen"
      :metrics="metrics"
      :is-busy="isBusy"
      :is-download-available="isDownloadAvailable"
      :is-current-selected="Boolean(currentEntry)"
      :instr-per-tick="manifest.limits.instrPerTick"
      :us-per-tick="manifest.limits.usPerTick"
      @reload="onReloadCurrent"
      @download="onDownload"
      @stop-all="onStopAllScripts"
    >
      <ScriptStudioActionBar
        :logs-count="logsCount"
        :line-count="lineCount"
        :byte-count="byteCount"
        :stack-max="stackMax"
        @insert="isSnippetsOpen = true"
        @examples="isExamplesOpen = true"
        @details="isDetailsOpen = true"
        @show-logs="onShowLogs"
      />
    </ScriptStudioMetrics>

    <ScriptStudioTabs :scripts="scripts" :active-name="name" :draft-name="draftName" :is-busy="isBusy" @select="onSelectScript" @toggle-run="onToggleRun" @remove="onAskDelete" @create="onNewScript" />

    <BytecodeEditor ref="editor" v-model="source" :manifest="manifest" :script-names="scriptNames" :errors="diagnostics" :device-suggestions="suggestions" />

    <ScriptStudioLogs ref="logsRef" :name="name" @update:count="onUpdateLogsCount" />
  </VCard>

  <ScriptSnippetsDialog v-if="isSnippetsOpen" :manifest="manifest" :suggestions="suggestions" @insert-function="onInsertFunction" @insert-event="onInsertEvent" @insert-suggestion="onInsertSuggestion" @close="isSnippetsOpen = false" />
  <ScriptExamplesDialog v-if="isExamplesOpen" :examples="BYTECODE_EXAMPLES" @load="onLoadExample" @close="isExamplesOpen = false" />
  <ScriptDetailsDialog v-if="isDetailsOpen" :result="result" :hex="hex" @close="isDetailsOpen = false" />
  <ScriptDeleteDialog v-if="deleteTarget" :name="deleteTarget" :is-busy="isBusy" @confirm="onConfirmDelete" @close="deleteTarget = null" />
  <ScriptRenameDialog v-if="isRenameOpen" :name="name" :is-busy="isBusy" @save="onRenameSave" @close="isRenameOpen = false" />
</template>
