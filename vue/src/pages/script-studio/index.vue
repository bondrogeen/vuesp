<script setup lang="ts">
import { useDebounceFn } from '@vueuse/core';
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue';
import { VIcon } from 'vuesp-components/components';

import BytecodeEditor from '@/components/BytecodeEditor.vue';
import { useLocale } from '@/composables/useLocale';
import { useStore } from '@/composables/useStore';
import { useLogs } from '@/composables/script/useLogs';
import { BYTECODE_EXAMPLES, compile, decompile, disassemble, DEFAULT_MANIFEST } from '@/lib/script';
import type { BytecodeExample, CapabilityManifest, CompileResult, EventSpec, FunctionSpec } from '@/lib/script';

const { $t } = useLocale();
const { message: wsMessage } = useStore();
const { logs, onHover } = useLogs(wsMessage);

const logsOpen = ref(true);
const logsPanel = ref<HTMLElement | null>(null);

function clearLogs() {
  logs.value = [];
}

function showLogs() {
  logsOpen.value = true;
  void nextTick(() => logsPanel.value?.scrollIntoView({ behavior: 'smooth', block: 'nearest' }));
}

interface ScriptListItem {
  name: string;
  id: number;
  size: number;
  boot: number;
  loaded: number;
  active: number;
  error: number;
  flags: number;
}

interface EditorApi {
  insertAtCursor: (text: string, caretOffset?: number) => void;
  focus: () => void;
}

type Tone = 'amber' | 'red' | 'green' | 'blue' | 'gray';

const defaultSource = `$u0 = 5;
log('hello', $u0);
$display = 'BC-OK';
$p13 = 128;
wait(100ms);
$p13 = 0;
`;

const name = ref('boot');
const source = ref(defaultSource);
const savedSource = ref('');
const savedName = ref('');
const result = ref<CompileResult | null>(null);
const manifest = ref<CapabilityManifest>(DEFAULT_MANIFEST);
const message = ref('');
const busy = ref(false);
const scripts = ref<ScriptListItem[]>([]);
const editor = ref<EditorApi | null>(null);
const tabsScroll = ref<HTMLDivElement | null>(null);
const nameInput = ref<HTMLInputElement | null>(null);
const editingName = ref(false);
let pollTimer: ReturnType<typeof setInterval> | null = null;

const snippetsOpen = ref(false);
const examplesOpen = ref(false);
const advancedOpen = ref(false);
const advancedTab = ref<'disasm' | 'hex'>('disasm');
const moreOpen = ref(false);
const deleteTarget = ref<string | null>(null);

const examples = BYTECODE_EXAMPLES;

function toArrayBuffer(bytes: Uint8Array): ArrayBuffer {
  const copy = new Uint8Array(bytes.length);
  copy.set(bytes);
  return copy.buffer;
}

const diagnostics = computed(() => result.value?.errors ?? []);
const sourceLines = computed(() => source.value.split('\n').length);
const hex = computed(() => {
  const bytes = result.value?.bytecode;
  if (!bytes) return '';
  const lines: string[] = [];
  for (let offset = 0; offset < bytes.length; offset += 16) {
    lines.push(
      `${offset.toString(16).padStart(4, '0')}  ${Array.from(bytes.slice(offset, offset + 16))
        .map((byte) => byte.toString(16).padStart(2, '0'))
        .join(' ')}`,
    );
  }
  return lines.join('\n');
});

const currentEntry = computed(() => scripts.value.find((script) => script.name === name.value) ?? null);
const draftTab = computed(() => !scripts.value.some((script) => script.name === name.value));
const dirty = computed(() => source.value !== savedSource.value || name.value !== savedName.value);

const currentState = computed<{ label: string; tone: Tone }>(() => {
  const entry = currentEntry.value;
  if (!entry) return { label: $t('scriptStudio.stateNotSaved'), tone: 'amber' };
  if (entry.error) return { label: $t('scriptStudio.stateError'), tone: 'red' };
  if (entry.active) return { label: $t('scriptStudio.stateRunning'), tone: 'green' };
  if (entry.loaded) return { label: $t('scriptStudio.stateLoaded'), tone: 'blue' };
  return { label: $t('scriptStudio.stateStopped'), tone: 'gray' };
});

const buildText = computed(() => {
  const compiled = result.value;
  if (!compiled) return $t('scriptStudio.notCompiled');
  if (!compiled.ok) return `${compiled.errors.length} ${$t('scriptStudio.diagnostics')}`;
  return `${compiled.bytecode?.length ?? 0} ${$t('scriptStudio.bytes')} · ${$t('scriptStudio.stack')} ${
    compiled.stats?.stackMax ?? '—'
  }`;
});

function stateClass(tone: Tone): string {
  switch (tone) {
    case 'red':
      return 'bg-red-main/10 text-red-main';
    case 'green':
      return 'bg-green-main/10 text-green-main';
    case 'blue':
      return 'bg-blue-main/10 text-blue-main';
    case 'amber':
      return 'bg-amber-main/10 text-amber-main';
    default:
      return 'bg-gray-main/10 text-gray-main';
  }
}

function toneDot(tone: Tone): string {
  switch (tone) {
    case 'red':
      return 'bg-red-main';
    case 'green':
      return 'bg-green-main';
    case 'blue':
      return 'bg-blue-main';
    case 'amber':
      return 'bg-amber-main';
    default:
      return 'bg-gray-main';
  }
}

function scriptState(script: ScriptListItem): { label: string; tone: Tone } {
  if (script.error) return { label: $t('scriptStudio.stateError'), tone: 'red' };
  if (script.active) return { label: $t('scriptStudio.stateRunning'), tone: 'green' };
  if (script.loaded) return { label: $t('scriptStudio.stateLoaded'), tone: 'blue' };
  return { label: $t('scriptStudio.stateStopped'), tone: 'gray' };
}

async function loadCapabilities() {
  try {
    const response = await fetch('/script/capabilities');
    if (response.ok) manifest.value = (await response.json()) as CapabilityManifest;
  } catch {
    manifest.value = DEFAULT_MANIFEST;
  }
}

function scrollActiveTab() {
  void nextTick(() => {
    const container = tabsScroll.value;
    if (!container) return;
    const active = container.querySelector<HTMLElement>(`#tab-${name.value}`);
    if (!active) return;
    const left = active.offsetLeft;
    const right = left + active.offsetWidth;
    if (left < container.scrollLeft) {
      container.scrollLeft = Math.max(0, left - 8);
    } else if (right > container.scrollLeft + container.clientWidth) {
      container.scrollLeft = right - container.clientWidth + 8;
    }
  });
}

async function refresh() {
  try {
    const response = await fetch('/script/list');
    if (response.ok) scripts.value = (await response.json()) as ScriptListItem[];
  } catch {
    /* keep the previous view */
  }
}

function compileSource(silent = false) {
  const compiled = compile(source.value, { manifest: manifest.value });
  result.value = compiled;
  if (silent) return;
  message.value = compiled.ok
    ? `${$t('scriptStudio.compiled')}: ${compiled.bytecode?.length ?? 0} ${$t(
        'scriptStudio.bytes',
      )}, ${$t('scriptStudio.stack')} ${compiled.stats?.stackMax ?? '?'}`
    : '';
}

const debouncedCompile = useDebounceFn(() => compileSource(true), 350);

watch(source, () => {
  void debouncedCompile();
});

async function post(url: string): Promise<boolean> {
  const response = await fetch(url, { method: 'POST' });
  const text = await response.text();
  if (!response.ok) {
    message.value = text;
    return false;
  }
  try {
    const data = JSON.parse(text) as { state?: boolean };
    if (data.state === false) {
      message.value = $t('scriptStudio.stateError');
      return false;
    }
  } catch {
    /* non-JSON success response */
  }
  return true;
}

async function save(runAfter: boolean) {
  if (busy.value) return;
  compileSource(true);
  const compiled = result.value;
  if (!compiled?.ok || !compiled.bytecode) {
    message.value = '';
    return;
  }

  busy.value = true;
  try {
    const response = await fetch(
      `/script/bytecode?name=${encodeURIComponent(name.value)}&run=${runAfter ? 1 : 0}`,
      {
        method: 'POST',
        headers: { 'Content-Type': 'application/octet-stream' },
        body: toArrayBuffer(compiled.bytecode),
      },
    );
    if (!response.ok) {
      message.value = await response.text();
      return;
    }
    savedSource.value = source.value;
    savedName.value = name.value;
    message.value = runAfter ? $t('scriptStudio.started') : $t('scriptStudio.saved');
    await refresh();
  } finally {
    busy.value = false;
  }
}

async function runNamed(scriptName: string) {
  if (busy.value) return;
  busy.value = true;
  try {
    if (await post(`/script/run?name=${encodeURIComponent(scriptName)}`)) {
      message.value = '';
      await refresh();
    }
  } finally {
    busy.value = false;
  }
}

async function stopNamed(scriptName: string) {
  if (busy.value) return;
  busy.value = true;
  try {
    if (await post(`/script/stop?name=${encodeURIComponent(scriptName)}`)) {
      message.value = $t('scriptStudio.stopped');
      await refresh();
    }
  } finally {
    busy.value = false;
  }
}

async function runCurrent() {
  if (dirty.value || !currentEntry.value) {
    await save(true);
    return;
  }
  await runNamed(name.value);
}

async function stopCurrent() {
  await stopNamed(name.value);
}

async function stopAll() {
  if (busy.value) return;
  busy.value = true;
  try {
    if (await post('/script/stop-all')) {
      message.value = $t('scriptStudio.stopped');
      await refresh();
    }
  } finally {
    busy.value = false;
  }
}

async function toggleRun(script: ScriptListItem) {
  if (script.active) await stopNamed(script.name);
  else await runNamed(script.name);
}

async function loadScript(scriptName: string) {
  busy.value = true;
  try {
    const response = await fetch(`/script/bytecode?name=${encodeURIComponent(scriptName)}`);
    if (!response.ok) {
      message.value = `HTTP ${response.status}`;
      return;
    }
    const bytes = new Uint8Array(await response.arrayBuffer());
    const decoded = decompile(bytes, { manifest: manifest.value });
    if (!decoded.ok || !decoded.source) {
      message.value = decoded.errors.map((error) => error.message).join('\n');
      return;
    }
    name.value = scriptName;
    source.value = decoded.source;
    savedSource.value = decoded.source;
    savedName.value = scriptName;
    result.value = {
      ok: true,
      errors: [],
      warnings: [],
      bytecode: bytes,
      disassembly: disassemble(bytes),
      source: decoded.source,
    };
    message.value = $t('scriptStudio.decompiled');
    scrollActiveTab();
  } finally {
    busy.value = false;
  }
}

async function selectScript(script: ScriptListItem) {
  if (script.name === name.value && !dirty.value) return;
  if (
    dirty.value &&
    script.name !== name.value &&
    !window.confirm($t('scriptStudio.discardChanges'))
  ) {
    return;
  }
  await loadScript(script.name);
}

async function reloadCurrent() {
  if (!currentEntry.value) return;
  await loadScript(name.value);
}

function startEditName() {
  editingName.value = true;
  void nextTick(() => {
    nameInput.value?.focus();
    nameInput.value?.select();
  });
}

function finishName() {
  editingName.value = false;
  scrollActiveTab();
}

function newScript() {
  if (dirty.value && !window.confirm($t('scriptStudio.discardChanges'))) return;
  let index = 1;
  while (scripts.value.some((script) => script.name === `script${index}`)) index++;
  name.value = `script${index}`;
  source.value = defaultSource;
  savedSource.value = '';
  savedName.value = '';
  result.value = null;
  message.value = $t('scriptStudio.newScript');
  compileSource(true);
  scrollActiveTab();
  void nextTick(() => editor.value?.focus());
}

function askDelete(scriptName: string) {
  deleteTarget.value = scriptName;
  moreOpen.value = false;
}

async function deleteScript() {
  const target = deleteTarget.value;
  if (!target || busy.value) return;
  busy.value = true;
  try {
    const response = await fetch(`/script/bytecode?name=${encodeURIComponent(target)}`, {
      method: 'DELETE',
    });
    if (!response.ok) {
      message.value = await response.text();
      return;
    }
    deleteTarget.value = null;
    message.value = $t('scriptStudio.deleted');
    if (name.value === target) {
      const next = scripts.value.find((script) => script.name !== target);
      if (next) await loadScript(next.name);
      else newScript();
    }
    await refresh();
  } finally {
    busy.value = false;
  }
}

function loadExample(example: BytecodeExample) {
  if (dirty.value && !window.confirm($t('scriptStudio.discardChanges'))) return;
  name.value = example.name;
  source.value = example.source;
  savedSource.value = '';
  savedName.value = '';
  result.value = null;
  message.value = '';
  examplesOpen.value = false;
  void nextTick(() => editor.value?.focus());
}

function insertFunction(fn: FunctionSpec) {
  editor.value?.insertAtCursor(`${fn.name}()`, fn.name.length + 1);
  snippetsOpen.value = false;
}

function insertEvent(event: EventSpec) {
  const snippet = `on('${event.name}') {\n  \n}\n`;
  editor.value?.insertAtCursor(snippet, snippet.indexOf('\n') + 3);
  snippetsOpen.value = false;
}

function download() {
  const bytes = result.value?.bytecode;
  if (!bytes) return;
  const blob = new Blob([toArrayBuffer(bytes)], { type: 'application/octet-stream' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = `script-${name.value || 'script'}.vbc`;
  link.click();
  URL.revokeObjectURL(url);
}

async function copyHex() {
  if (hex.value) await navigator.clipboard.writeText(hex.value);
}

function closeTopLayer(): boolean {
  if (moreOpen.value) {
    moreOpen.value = false;
    return true;
  }
  if (deleteTarget.value) {
    deleteTarget.value = null;
    return true;
  }
  if (snippetsOpen.value) {
    snippetsOpen.value = false;
    return true;
  }
  if (examplesOpen.value) {
    examplesOpen.value = false;
    return true;
  }
  if (advancedOpen.value) {
    advancedOpen.value = false;
    return true;
  }
  return false;
}

function onWindowKeydown(event: KeyboardEvent) {
  if (event.key === 'Escape') closeTopLayer();
}

onMounted(async () => {
  window.addEventListener('keydown', onWindowKeydown);
  await loadCapabilities();
  await refresh();
  const initial = scripts.value.find((script) => script.name === name.value) ?? scripts.value[0];
  if (initial) await loadScript(initial.name);
  else compileSource(true);
  pollTimer = setInterval(() => {
    if (!busy.value) void refresh();
  }, 1500);
});

onBeforeUnmount(() => {
  window.removeEventListener('keydown', onWindowKeydown);
  if (pollTimer) {
    clearInterval(pollTimer);
    pollTimer = null;
  }
});
</script>

<template>
  <div class="flex flex-col gap-4">
      <section class="glass-inset flex min-w-0 flex-col rounded-xl">
        <div
          class="flex flex-wrap items-center gap-2 border-b border-gray-light-10/70 px-4 py-3 dark:border-white/10"
        >
          <span
            class="size-2.5 rounded-full"
            :class="
              currentState.tone === 'red'
                ? 'bg-red-main'
                : currentState.tone === 'green'
                  ? 'bg-green-main'
                  : currentState.tone === 'amber'
                    ? 'bg-amber-main'
                    : 'bg-blue-main'
            "
          />
          <template v-if="editingName">
            <input
              ref="nameInput"
              v-model="name"
              type="text"
              maxlength="16"
              class="w-44 min-w-0 rounded border border-gray-light-10 bg-transparent px-1 font-mono text-sm font-semibold outline-none focus:border-blue-main dark:border-white/10"
              :aria-label="$t('scriptStudio.id')"
              @keydown.enter="finishName"
              @keydown.esc="finishName"
              @blur="finishName"
            />
            <button
              class="rounded p-1 text-gray-dark-30 hover:bg-black/5 dark:text-white/60 dark:hover:bg-white/5"
              :title="$t('scriptStudio.editName')"
              :aria-label="$t('scriptStudio.editName')"
              @click="finishName"
            >
              <span class="text-sm leading-none">✓</span>
            </button>
          </template>
          <template v-else>
            <span class="max-w-44 truncate font-mono text-sm font-semibold">{{ name }}</span>
            <button
              class="rounded p-1 text-gray-dark-30 hover:bg-black/5 dark:text-white/60 dark:hover:bg-white/5"
              :title="$t('scriptStudio.editName')"
              :aria-label="$t('scriptStudio.editName')"
              @click="startEditName"
            >
              <span class="text-sm leading-none">✎</span>
            </button>
          </template>
          <span
            v-if="dirty"
            class="size-1.5 rounded-full bg-amber-main"
            :title="$t('scriptStudio.stateNotSaved')"
          />
          <span
            class="rounded px-2 py-0.5 text-[10px]"
            :class="stateClass(currentState.tone)"
          >
            {{ currentState.label }}
          </span>

          <div class="ml-auto flex flex-wrap items-center gap-2">
            <button
              class="inline-flex items-center gap-1 rounded-md bg-blue-main px-3 py-1.5 text-xs font-medium text-white transition hover:opacity-90 disabled:opacity-50"
              :disabled="busy"
              @click="save(false)"
            >
              <VIcon name="save" class="size-3.5" />
              {{ $t('save') }}
            </button>
            <button
              v-if="currentEntry?.active"
              class="inline-flex items-center gap-1 rounded-md bg-red-main px-3 py-1.5 text-xs font-medium text-white transition hover:opacity-90 disabled:opacity-50"
              :disabled="busy"
              @click="stopCurrent"
            >
              <VIcon name="stop-circle" class="size-3.5" />
              {{ $t('scriptStudio.stop') }}
            </button>
            <button
              v-else
              class="inline-flex items-center gap-1 rounded-md bg-green-main px-3 py-1.5 text-xs font-medium text-white transition hover:opacity-90 disabled:opacity-50"
              :disabled="busy"
              @click="runCurrent"
            >
              <VIcon name="play-circle" class="size-3.5" />
              {{ $t('scriptStudio.run') }}
            </button>

            <div class="relative">
              <button
                class="inline-flex items-center justify-center rounded-md border border-gray-light-10/70 px-2 py-1.5 text-xs transition hover:bg-black/5 disabled:opacity-50 dark:border-white/10 dark:hover:bg-white/5"
                :disabled="busy"
                :title="$t('scriptStudio.more')"
                :aria-label="$t('scriptStudio.more')"
                @click="moreOpen = !moreOpen"
              >
                <VIcon name="more" class="size-3.5" />
              </button>
              <div v-if="moreOpen" class="fixed inset-0 z-30" @click="moreOpen = false" />
              <div
                v-if="moreOpen"
                class="glass-popover absolute right-0 top-full z-40 mt-1 w-56 rounded-lg p-1 text-xs"
              >
                <button
                  class="flex w-full items-center gap-2 rounded px-2 py-1.5 text-left hover:bg-black/5 dark:hover:bg-white/5"
                  @click="moreOpen = false; reloadCurrent()"
                >
                  <VIcon name="refresh" class="size-3.5" />
                  {{ $t('scriptStudio.reloaded') }}
                </button>
                <button
                  class="flex w-full items-center gap-2 rounded px-2 py-1.5 text-left hover:bg-black/5 disabled:opacity-50 dark:hover:bg-white/5"
                  :disabled="!result?.bytecode"
                  @click="moreOpen = false; download()"
                >
                  <VIcon name="download" class="size-3.5" />
                  {{ $t('scriptStudio.download') }}
                </button>
                <button
                  class="flex w-full items-center gap-2 rounded px-2 py-1.5 text-left hover:bg-black/5 dark:hover:bg-white/5"
                  @click="moreOpen = false; stopAll()"
                >
                  <VIcon name="stop-circle" class="size-3.5" />
                  {{ $t('scriptStudio.stopAll') }}
                </button>
                <button
                  v-if="currentEntry"
                  class="flex w-full items-center gap-2 rounded px-2 py-1.5 text-left text-red-main hover:bg-red-main/10"
                  @click="askDelete(name)"
                >
                  <VIcon name="trash" class="size-3.5" />
                  {{ $t('scriptStudio.delete') }}
                </button>
              </div>
            </div>
          </div>
        </div>

        <div
          class="flex flex-wrap items-center gap-2 border-b border-gray-light-10/70 px-4 py-2 text-xs dark:border-white/10"
        >
          <button
            class="inline-flex items-center gap-1 rounded-md border border-gray-light-10/70 px-2.5 py-1.5 transition hover:bg-black/5 dark:border-white/10 dark:hover:bg-white/5"
            @click="snippetsOpen = true"
          >
            <VIcon name="code" class="size-3.5" />
            {{ $t('scriptStudio.insert') }}
          </button>
          <button
            class="inline-flex items-center gap-1 rounded-md border border-gray-light-10/70 px-2.5 py-1.5 transition hover:bg-black/5 dark:border-white/10 dark:hover:bg-white/5"
            @click="examplesOpen = true"
          >
            <VIcon name="document" class="size-3.5" />
            {{ $t('examples') }}
          </button>
          <button
            class="inline-flex items-center gap-1 rounded-md border border-gray-light-10/70 px-2.5 py-1.5 transition hover:bg-black/5 dark:border-white/10 dark:hover:bg-white/5"
            @click="advancedOpen = true"
          >
            <VIcon name="list-check" class="size-3.5" />
            {{ $t('scriptStudio.details') }}
          </button>
          <button
            class="inline-flex items-center gap-1 rounded-md border border-gray-light-10/70 px-2.5 py-1.5 transition hover:bg-black/5 dark:border-white/10 dark:hover:bg-white/5"
            @click="showLogs"
          >
            <VIcon name="code" class="size-3.5" />
            {{ $t('logs') }}
            <span class="rounded bg-black/5 px-1.5 text-[10px] dark:bg-white/10">
              {{ logs.length }}
            </span>
          </button>

          <span class="ml-auto font-mono text-[11px] text-gray-dark-25 dark:text-gray-main">
            {{ sourceLines }} {{ $t('scriptStudio.lines') }} · {{ result?.bytecode?.length ?? 0 }}
            {{ $t('scriptStudio.bytes') }} · {{ $t('scriptStudio.stack') }}
            {{ result?.stats?.stackMax ?? '—' }}
          </span>
        </div>

        <div
          class="flex items-end gap-1 bg-black/[0.02] dark:bg-white/[0.02]"
        >
          <button
            class="mb-0 grid h-[31px] w-[42px] shrink-0 translate-y-px place-items-center rounded-t-lg border border-b-0 border-gray-light-10/70 bg-black/[0.02] text-gray-dark-30 transition hover:bg-black/5 dark:border-white/10 dark:bg-transparent dark:text-white/60 dark:hover:bg-white/5"
            :title="$t('scriptStudio.newScript')"
            :aria-label="$t('scriptStudio.newScript')"
            @click="newScript"
          >
            <VIcon name="add" class="size-3.5" />
          </button>
          <div ref="tabsScroll" class="flex flex-1 gap-1 overflow-x-auto overflow-y-hidden pt-1.5 scrollbar">
            <div class="flex min-w-max items-end gap-1">
              <div
                v-for="script in scripts"
                :key="script.name"
                :id="`tab-${script.name}`"
                class="group flex items-center gap-1 rounded-t-lg border border-b-0 px-2 py-1"
                :class="
                  script.name === name
                    ? 'border-gray-light-10/70 bg-gray-dark-40/[0.05] text-gray-dark-40 dark:border-white/10 dark:bg-black/35 dark:text-white'
                    : 'translate-y-px border-gray-light-10/70 bg-black/[0.02] text-gray-dark-30 hover:bg-black/5 dark:border-white/10 dark:bg-transparent dark:text-white/60 dark:hover:bg-white/5'
                "
              >
                <button class="flex min-w-0 items-center gap-1 text-left" @click="selectScript(script)">
                  <span class="size-1.5 shrink-0 rounded-full" :class="toneDot(scriptState(script).tone)" />
                  <span class="max-w-40 truncate font-mono text-[11px]">{{ script.name }}</span>
                  <span
                    v-if="script.name === name && dirty"
                    class="size-1 shrink-0 rounded-full bg-amber-main"
                    :title="$t('scriptStudio.stateNotSaved')"
                  />
                </button>
                <button
                  class="rounded p-0.5 text-current opacity-70 transition hover:bg-black/5 hover:opacity-100 disabled:opacity-40 dark:hover:bg-white/5"
                  :disabled="busy"
                  :title="script.active ? $t('scriptStudio.stop') : $t('scriptStudio.run')"
                  :aria-label="script.active ? $t('scriptStudio.stop') : $t('scriptStudio.run')"
                  @click.stop="toggleRun(script)"
                >
                  <VIcon :name="script.active ? 'stop-circle' : 'play-circle'" class="size-3" />
                </button>
                <button
                  class="rounded p-0.5 text-current opacity-70 transition hover:bg-red-main/10 hover:text-red-main hover:opacity-100"
                  :title="$t('scriptStudio.delete')"
                  :aria-label="$t('scriptStudio.delete')"
                  @click.stop="askDelete(script.name)"
                >
                  <VIcon name="trash" class="size-3" />
                </button>
              </div>
              <div
                v-if="draftTab"
                :id="`tab-${name}`"
                class="-mb-px flex items-center gap-1 rounded-t-lg border border-b-0 border-amber-main/40 bg-white/70 px-2 py-1 text-gray-dark-40 dark:border-amber-main/40 dark:bg-white/10 dark:text-white"
              >
                <span class="size-1.5 shrink-0 rounded-full bg-amber-main" />
                <span class="max-w-40 truncate font-mono text-[11px]">{{ name }}</span>
                <span class="rounded bg-amber-main/10 px-1 text-[8px] uppercase text-amber-main">
                  {{ $t('scriptStudio.stateNotSaved') }}
                </span>
              </div>
              <span
                v-if="scripts.length === 0 && !draftTab"
                class="px-3 py-1 text-[11px] text-gray-dark-30 dark:text-white/60"
              >
                {{ $t('noScripts') }}
              </span>
            </div>
          </div>
        </div>
        <BytecodeEditor
          ref="editor"
          v-model="source"
          :manifest="manifest"
          :script-names="scripts.map((script) => script.name)"
          :errors="diagnostics"
        />

        <div
          class="flex items-center gap-2 border-t border-gray-light-10/70 px-4 py-2 dark:border-white/10"
        >
          <span
            class="size-1.5 rounded-full"
            :class="
              diagnostics.length ? 'bg-red-main' : result?.ok ? 'bg-green-main' : 'bg-gray-main'
            "
          />
          <span class="font-mono text-xs whitespace-pre-wrap text-gray-dark-30 dark:text-white/70">
            {{ dirty ? $t('scriptStudio.stateNotSaved') : message || buildText }}
          </span>
        </div>


        <div ref="logsPanel" class="border-t border-gray-light-10/70 dark:border-white/10">
          <div class="flex items-center gap-2 px-4 py-2 text-xs">
            <button class="flex items-center gap-2" @click="logsOpen = !logsOpen">
              <VIcon name="code" class="size-3.5" />
              <span>{{ $t('logs') }}</span>
              <span class="rounded bg-black/5 px-1.5 text-[10px] dark:bg-white/10">
                {{ logs.length }}
              </span>
              <span class="text-[10px] text-gray-dark-25 dark:text-gray-main">
                {{ logsOpen ? '−' : '+' }}
              </span>
            </button>
            <button
              class="ml-auto text-[11px] text-blue-500 hover:underline disabled:opacity-50"
              :disabled="logs.length === 0"
              @click="clearLogs"
            >
              {{ $t('scriptStudio.clear') }}
            </button>
          </div>
          <div
            v-if="logsOpen"
            ref="container"
            class="h-48 overflow-auto border-t border-gray-light-10/70 bg-gray-dark-40/[0.05] p-3 font-mono text-xs leading-5 scrollbar dark:border-white/10 dark:bg-black/35"
            @mouseenter="onHover(true)"
            @mouseleave="onHover(false)"
          >
            <div v-for="(log, index) in logs" :key="index" class="flex gap-2">
              <span class="shrink-0 text-gray-dark-25 dark:text-gray-main">{{ log.time }}</span>
              <span class="whitespace-pre-wrap break-all text-gray-dark-30 dark:text-white/70">
                {{ log.text }}
              </span>
            </div>
            <div v-if="logs.length === 0" class="text-gray-dark-25 dark:text-gray-main">
              {{ $t('noLogs') }}
            </div>
          </div>
        </div>
      </section>

    <Teleport to="body">
      <div
        v-if="snippetsOpen"
        class="fixed inset-0 z-[100] flex items-center justify-center bg-black/50 p-4 backdrop-blur-sm"
        @click.self="snippetsOpen = false"
      >
        <div class="glass-popover max-h-[80vh] w-full max-w-3xl overflow-auto rounded-xl p-5 scrollbar">
          <div class="mb-4 flex items-center justify-between">
            <h2 class="text-lg font-semibold">{{ $t('scriptStudio.suggestions') }}</h2>
            <button class="rounded p-1 hover:bg-black/5 dark:hover:bg-white/5" @click="snippetsOpen = false">
              <VIcon name="close" class="size-4" />
            </button>
          </div>
          <div class="grid gap-5 md:grid-cols-2">
            <section>
              <div class="mb-2 text-[11px] uppercase tracking-wide text-gray-dark-30 dark:text-white/50">
                {{ $t('scriptStudio.functions') }}
              </div>
              <div class="flex flex-wrap gap-1">
                <button
                  v-for="fn in manifest.functions"
                  :key="`fn-${fn.id}`"
                  class="rounded-md border border-gray-light-10 px-2 py-1 text-xs hover:bg-black/5 dark:border-white/10 dark:hover:bg-white/5"
                  :title="`${fn.name}(${fn.min}..${fn.max}) => ${fn.ret}`"
                  @click="insertFunction(fn)"
                >
                  {{ fn.name }}
                </button>
              </div>
            </section>
            <section>
              <div class="mb-2 text-[11px] uppercase tracking-wide text-gray-dark-30 dark:text-white/50">
                {{ $t('scriptStudio.events') }}
              </div>
              <div class="flex flex-wrap gap-1">
                <button
                  v-for="event in manifest.events"
                  :key="`ev-${event.id}`"
                  class="rounded-md border border-gray-light-10 px-2 py-1 text-xs hover:bg-black/5 dark:border-white/10 dark:hover:bg-white/5"
                  :title="`${event.name}: ${event.params} params`"
                  @click="insertEvent(event)"
                >
                  {{ event.name }}
                </button>
              </div>
            </section>
          </div>
        </div>
      </div>

      <div
        v-if="examplesOpen"
        class="fixed inset-0 z-[100] flex items-center justify-center bg-black/50 p-4 backdrop-blur-sm"
        @click.self="examplesOpen = false"
      >
        <div class="glass-popover max-h-[80vh] w-full max-w-3xl overflow-auto rounded-xl p-5 scrollbar">
          <div class="mb-4 flex items-center justify-between">
            <h2 class="text-lg font-semibold">{{ $t('examples') }}</h2>
            <button class="rounded p-1 hover:bg-black/5 dark:hover:bg-white/5" @click="examplesOpen = false">
              <VIcon name="close" class="size-4" />
            </button>
          </div>
          <div class="grid gap-2 sm:grid-cols-2">
            <button
              v-for="example in examples"
              :key="`example-${example.name}`"
              class="rounded-lg border border-gray-light-10 p-3 text-left transition hover:bg-black/5 dark:border-white/10 dark:hover:bg-white/5"
              @click="loadExample(example)"
            >
              <div class="font-mono text-sm">{{ example.name }}</div>
              <div class="mt-1 max-h-12 overflow-hidden whitespace-pre-wrap text-[11px] text-gray-dark-25 dark:text-gray-main">
                {{ example.source }}
              </div>
            </button>
          </div>
        </div>
      </div>

      <div
        v-if="advancedOpen"
        class="fixed inset-0 z-[100] flex items-center justify-center bg-black/50 p-4 backdrop-blur-sm"
        @click.self="advancedOpen = false"
      >
        <div class="glass-popover flex max-h-[80vh] w-full max-w-4xl flex-col overflow-hidden rounded-xl">
          <div class="flex items-center justify-between border-b border-gray-light-10/70 px-5 py-3 dark:border-white/10">
            <h2 class="text-lg font-semibold">{{ $t('scriptStudio.details') }}</h2>
            <button class="rounded p-1 hover:bg-black/5 dark:hover:bg-white/5" @click="advancedOpen = false">
              <VIcon name="close" class="size-4" />
            </button>
          </div>
          <div class="flex items-center gap-2 border-b border-gray-light-10/70 px-5 py-2 text-xs dark:border-white/10">
            <button
              class="rounded px-2 py-1"
              :class="advancedTab === 'disasm' ? 'bg-blue-main/10 text-blue-main' : 'hover:bg-black/5 dark:hover:bg-white/5'"
              @click="advancedTab = 'disasm'"
            >
              {{ $t('scriptStudio.disassembly') }}
            </button>
            <button
              class="rounded px-2 py-1"
              :class="advancedTab === 'hex' ? 'bg-blue-main/10 text-blue-main' : 'hover:bg-black/5 dark:hover:bg-white/5'"
              @click="advancedTab = 'hex'"
            >
              {{ $t('scriptStudio.hex') }}
            </button>
            <button
              v-if="advancedTab === 'hex'"
              class="ml-auto text-blue-500 hover:underline disabled:opacity-50"
              :disabled="!hex"
              @click="copyHex"
            >
              {{ $t('scriptStudio.copy') }}
            </button>
          </div>
          <pre
            v-if="advancedTab === 'disasm'"
            class="m-0 flex-1 overflow-auto whitespace-pre bg-gray-dark-40/[0.05] p-4 font-mono text-xs leading-6 scrollbar dark:bg-black/35"
          >{{ result?.disassembly || '—' }}</pre>
          <pre
            v-else
            class="m-0 flex-1 overflow-auto whitespace-pre bg-gray-dark-40/[0.05] p-4 font-mono text-xs leading-6 scrollbar dark:bg-black/35"
          >{{ hex || '—' }}</pre>
        </div>
      </div>

      <div
        v-if="deleteTarget"
        class="fixed inset-0 z-[100] flex items-center justify-center bg-black/50 p-4 backdrop-blur-sm"
        @click.self="deleteTarget = null"
      >
        <div class="glass-popover w-full max-w-md rounded-xl p-5">
          <h2 class="text-lg font-semibold">{{ $t('scriptStudio.deleteConfirm') }}</h2>
          <p class="mt-2 text-sm text-gray-dark-30 dark:text-white/70">
            <span class="font-mono">{{ deleteTarget }}</span> — {{ $t('scriptStudio.deleteHint') }}
          </p>
          <div class="mt-5 flex justify-end gap-2">
            <button
              class="rounded-md border border-gray-light-10 px-3 py-1.5 text-xs hover:bg-black/5 dark:border-white/10 dark:hover:bg-white/5"
              @click="deleteTarget = null"
            >
              {{ $t('scriptStudio.cancel') }}
            </button>
            <button
              class="rounded-md bg-red-main px-3 py-1.5 text-xs font-medium text-white hover:opacity-90"
              @click="deleteScript"
            >
              {{ $t('scriptStudio.delete') }}
            </button>
          </div>
        </div>
      </div>
    </Teleport>
  </div>
</template>
