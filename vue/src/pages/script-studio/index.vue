<script setup lang="ts">
import { computed, nextTick, onMounted, ref } from 'vue';

import { useLocale } from '@/composables/useLocale';
import { BYTECODE_EXAMPLES, compile, decompile, disassemble, DEFAULT_MANIFEST } from '@/lib/script';
import type { BytecodeExample, CapabilityManifest, CompileResult, EventSpec, FunctionSpec } from '@/lib/script';

const { $t } = useLocale();

const name = ref('boot');
const examples = BYTECODE_EXAMPLES;
const source = ref(`$u0 = 5;
log('hello', $u0);
$display = 'BC-OK';
$p13 = 128;
wait(100ms);
$p13 = 0;
`);

const result = ref<CompileResult | null>(null);
const manifest = ref<CapabilityManifest>(DEFAULT_MANIFEST);
const message = ref('');
const busy = ref(false);
const status = ref<Record<string, unknown> | null>(null);
const scripts = ref<Array<{ name: string; size: number; boot: number }>>([]);
const sourceEl = ref<HTMLTextAreaElement | null>(null);

function toArrayBuffer(bytes: Uint8Array): ArrayBuffer {
  const copy = new Uint8Array(bytes.length);
  copy.set(bytes);
  return copy.buffer;
}

const diagnostics = computed(() => result.value?.errors ?? []);
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

async function loadCapabilities() {
  try {
    const response = await fetch('/script/capabilities');
    if (response.ok) manifest.value = (await response.json()) as CapabilityManifest;
  } catch {
    manifest.value = DEFAULT_MANIFEST;
  }
}

async function refresh() {
  try {
    const [statusResponse, listResponse] = await Promise.all([
      fetch('/script/status'),
      fetch('/script/list'),
    ]);
    if (statusResponse.ok) status.value = (await statusResponse.json()) as Record<string, unknown>;
    if (listResponse.ok) scripts.value = (await listResponse.json()) as Array<{ name: string; size: number; boot: number }>;
  } catch {
    /* keep the previous view */
  }
}

async function compileSource() {
  busy.value = true;
  try {
    const compiled = compile(source.value, { manifest: manifest.value });
    result.value = compiled;
    if (compiled.ok) {
      message.value = `${$t('scriptStudio.compiled')}: ${compiled.bytecode?.length ?? 0} ${$t(
        'scriptStudio.bytes',
      )}, ${$t('scriptStudio.stack')} ${compiled.stats?.stackMax ?? '?'}`;
    } else {
      message.value = compiled.errors
        .map((error) => `${error.pos.line}:${error.pos.column} ${error.message}`)
        .join('\n');
    }
  } finally {
    busy.value = false;
  }
}

async function upload(run: boolean) {
  if (!result.value?.ok || !result.value.bytecode) await compileSource();
  const bytes = result.value?.bytecode;
  if (!bytes) return;
  busy.value = true;
  try {
    const response = await fetch(`/script/bytecode?name=${encodeURIComponent(name.value)}&run=${run ? 1 : 0}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/octet-stream' },
      body: toArrayBuffer(bytes),
    });
    message.value = await response.text();
    await refresh();
  } finally {
    busy.value = false;
  }
}

async function loadFromDevice() {
  busy.value = true;
  try {
    const response = await fetch(`/script/bytecode?name=${encodeURIComponent(name.value)}`);
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
    source.value = decoded.source;
    result.value = {
      ok: true,
      errors: [],
      warnings: [],
      bytecode: bytes,
      disassembly: disassemble(bytes),
      source: decoded.source,
    };
    message.value = $t('scriptStudio.decompiled');
  } finally {
    busy.value = false;
  }
}

async function run() {
  const response = await fetch(`/script/run?name=${encodeURIComponent(name.value)}`, { method: 'POST' });
  message.value = await response.text();
  await refresh();
}

async function stop() {
  const response = await fetch(`/script/stop?name=${encodeURIComponent(name.value)}`, { method: 'POST' });
  message.value = await response.text();
  await refresh();
}

async function stopAll() {
  const response = await fetch('/script/stop-all', { method: 'POST' });
  message.value = await response.text();
  await refresh();
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

function selectScript(scriptName: string) {
  name.value = scriptName;
}

function loadExample(example: BytecodeExample) {
  name.value = example.name;
  source.value = example.source;
  result.value = null;
  message.value = '';
}

async function insertAtCursor(text: string) {
  const element = sourceEl.value;
  if (!element) {
    source.value += text;
    return;
  }
  const start = element.selectionStart ?? source.value.length;
  const end = element.selectionEnd ?? start;
  source.value = source.value.slice(0, start) + text + source.value.slice(end);
  await nextTick();
  const position = start + text.length;
  element.focus();
  element.setSelectionRange(position, position);
}

function insertFunction(fn: FunctionSpec) {
  insertAtCursor(`${fn.name}()`);
}

function insertEvent(event: EventSpec) {
  insertAtCursor(`\non('${event.name}') {\n  \n}\n`);
}

onMounted(async () => {
  await loadCapabilities();
  await refresh();
});
</script>

<template>
  <div class="flex flex-col gap-6">
    <div class="flex flex-wrap items-center justify-between gap-3">
      <h1>{{ $t('scriptStudio.title') }}</h1>

      <div class="flex items-center gap-2">
        <label class="text-sm text-gray-dark-30 dark:text-white/70">{{ $t('scriptStudio.id') }}</label>
        <input
          v-model="name"
          type="text"
          maxlength="16"
          class="w-40 rounded-md border border-gray-light-10 dark:border-gray-dark-30 bg-white dark:bg-gray-dark-40 px-3 py-2 text-sm"
        />
        <button
          class="rounded-md border border-gray-light-10 dark:border-gray-dark-30 px-3 py-2 text-sm hover:opacity-80"
          :disabled="busy"
          @click="refresh"
        >
          {{ $t('scriptStudio.refresh') }}
        </button>
      </div>
    </div>

    <div class="grid grid-cols-1 xl:grid-cols-[2fr_1fr] gap-4">
      <section class="rounded-lg border border-gray-light-10 dark:border-gray-dark-30 bg-white dark:bg-gray-dark-40 p-4">
        <div class="flex flex-wrap items-center justify-between gap-2 mb-3">
          <h2 class="text-lg font-semibold">{{ $t('scriptStudio.source') }}</h2>
          <div class="flex flex-wrap gap-2">
            <button class="rounded-md bg-blue-600 px-3 py-2 text-sm text-white hover:bg-blue-700 disabled:opacity-50" :disabled="busy" @click="compileSource">
              {{ $t('scriptStudio.compile') }}
            </button>
            <button class="rounded-md bg-green-600 px-3 py-2 text-sm text-white hover:bg-green-700 disabled:opacity-50" :disabled="busy" @click="upload(true)">
              {{ $t('scriptStudio.uploadRun') }}
            </button>
            <button class="rounded-md border border-gray-light-10 dark:border-gray-dark-30 px-3 py-2 text-sm hover:opacity-80 disabled:opacity-50" :disabled="busy" @click="upload(false)">
              {{ $t('scriptStudio.upload') }}
            </button>
            <button class="rounded-md border border-gray-light-10 dark:border-gray-dark-30 px-3 py-2 text-sm hover:opacity-80 disabled:opacity-50" :disabled="busy" @click="loadFromDevice">
              {{ $t('scriptStudio.load') }}
            </button>
            <button class="rounded-md border border-gray-light-10 dark:border-gray-dark-30 px-3 py-2 text-sm hover:opacity-80 disabled:opacity-50" :disabled="busy || !result?.bytecode" @click="download">
              {{ $t('scriptStudio.download') }}
            </button>
            <button class="rounded-md border border-red-400 px-3 py-2 text-sm text-red-500 hover:opacity-80" :disabled="busy" @click="stop">
              {{ $t('scriptStudio.stop') }}
            </button>
            <button class="rounded-md border border-red-400 px-3 py-2 text-sm text-red-500 hover:opacity-80" :disabled="busy" @click="stopAll">
              {{ $t('scriptStudio.stopAll') }}
            </button>
            <button class="rounded-md border border-gray-light-10 dark:border-gray-dark-30 px-3 py-2 text-sm hover:opacity-80" :disabled="busy" @click="run">
              {{ $t('scriptStudio.run') }}
            </button>
          </div>
        </div>

        <textarea
          ref="sourceEl"
          v-model="source"
          spellcheck="false"
          class="h-[420px] w-full resize-y rounded-md border border-gray-light-10 dark:border-gray-dark-30 bg-gray-light-30 dark:bg-gray-dark-30 p-3 font-mono text-xs leading-5 outline-none focus:border-blue-500"
        ></textarea>

        <pre v-if="message" class="mt-3 whitespace-pre-wrap rounded-md bg-gray-light-30 dark:bg-gray-dark-30 p-3 text-xs text-gray-dark-30 dark:text-white/70">{{ message }}</pre>

        <div v-if="diagnostics.length" class="mt-3">
          <h3 class="text-sm font-semibold mb-1">{{ $t('scriptStudio.diagnostics') }}</h3>
          <ul class="space-y-1">
            <li v-for="(error, index) in diagnostics" :key="index" class="text-xs text-red-500">
              {{ error.pos.line }}:{{ error.pos.column }} — {{ error.message }}
            </li>
          </ul>
        </div>
      </section>

      <aside class="flex flex-col gap-4">
        <section class="rounded-lg border border-gray-light-10 dark:border-gray-dark-30 bg-white dark:bg-gray-dark-40 p-4">
          <h2 class="text-lg font-semibold mb-2">{{ $t('examples') }}</h2>
          <div class="flex flex-wrap gap-1">
            <button
              v-for="example in examples"
              :key="`example-${example.name}`"
              class="rounded-md border border-gray-light-10 dark:border-gray-dark-30 px-2 py-1 text-xs hover:bg-gray-light-30 dark:hover:bg-gray-dark-30"
              @click="loadExample(example)"
            >
              {{ example.name }}
            </button>
          </div>
        </section>

        <section class="rounded-lg border border-gray-light-10 dark:border-gray-dark-30 bg-white dark:bg-gray-dark-40 p-4">
          <div class="flex items-center justify-between mb-2">
            <h2 class="text-lg font-semibold">{{ $t('scriptStudio.status') }}</h2>
            <button class="text-xs text-blue-500 hover:underline" @click="refresh">{{ $t('scriptStudio.refresh') }}</button>
          </div>
          <pre class="whitespace-pre-wrap text-xs text-gray-dark-30 dark:text-white/70">{{ status ? JSON.stringify(status, null, 2) : '—' }}</pre>
        </section>

        <section class="rounded-lg border border-gray-light-10 dark:border-gray-dark-30 bg-white dark:bg-gray-dark-40 p-4">
          <h2 class="text-lg font-semibold mb-2">{{ $t('scriptStudio.suggestions') }}</h2>

          <div class="mb-3">
            <div class="text-[11px] uppercase tracking-wide text-gray-dark-30 dark:text-white/50">{{ $t('scriptStudio.functions') }}</div>
            <div class="mt-1 flex flex-wrap gap-1">
              <button
                v-for="fn in manifest.functions"
                :key="`fn-${fn.id}`"
                class="rounded-md border border-gray-light-10 dark:border-gray-dark-30 px-2 py-1 text-xs hover:bg-gray-light-30 dark:hover:bg-gray-dark-30"
                :title="`${fn.name}(${fn.min}..${fn.max}) => ${fn.ret}`"
                @click="insertFunction(fn)"
              >
                {{ fn.name }}
              </button>
            </div>
          </div>

          <div>
            <div class="text-[11px] uppercase tracking-wide text-gray-dark-30 dark:text-white/50">{{ $t('scriptStudio.events') }}</div>
            <div class="mt-1 flex flex-wrap gap-1">
              <button
                v-for="event in manifest.events"
                :key="`ev-${event.id}`"
                class="rounded-md border border-gray-light-10 dark:border-gray-dark-30 px-2 py-1 text-xs hover:bg-gray-light-30 dark:hover:bg-gray-dark-30"
                :title="`${event.name}: ${event.params} params`"
                @click="insertEvent(event)"
              >
                {{ event.name }}
              </button>
            </div>
          </div>
        </section>

        <section class="rounded-lg border border-gray-light-10 dark:border-gray-dark-30 bg-white dark:bg-gray-dark-40 p-4">
          <h2 class="text-lg font-semibold mb-2">{{ $t('scriptStudio.scripts') }}</h2>
          <ul class="space-y-1">
            <li v-for="script in scripts" :key="script.name">
              <button
                class="flex w-full items-center justify-between rounded-md px-2 py-1 text-left text-sm hover:bg-gray-light-30 dark:hover:bg-gray-dark-30"
                @click="selectScript(script.name)"
              >
                <span>{{ script.name }}</span>
                <span class="text-xs text-gray-dark-30 dark:text-white/60">{{ script.size }} {{ $t('scriptStudio.bytes') }}</span>
              </button>
            </li>
            <li v-if="scripts.length === 0" class="text-xs text-gray-dark-30 dark:text-white/60">{{ $t('noScripts') }}</li>
          </ul>
        </section>
      </aside>
    </div>

    <div class="grid grid-cols-1 xl:grid-cols-2 gap-4">
      <section class="rounded-lg border border-gray-light-10 dark:border-gray-dark-30 bg-white dark:bg-gray-dark-40 p-4">
        <div class="flex items-center justify-between mb-2">
          <h2 class="text-lg font-semibold">{{ $t('scriptStudio.disassembly') }}</h2>
        </div>
        <pre class="max-h-[360px] overflow-auto whitespace-pre rounded-md bg-gray-light-30 dark:bg-gray-dark-30 p-3 font-mono text-xs">{{ result?.disassembly || '—' }}</pre>
      </section>

      <section class="rounded-lg border border-gray-light-10 dark:border-gray-dark-30 bg-white dark:bg-gray-dark-40 p-4">
        <div class="flex items-center justify-between mb-2">
          <h2 class="text-lg font-semibold">{{ $t('scriptStudio.hex') }}</h2>
          <button class="text-xs text-blue-500 hover:underline" :disabled="!hex" @click="copyHex">{{ $t('scriptStudio.copy') }}</button>
        </div>
        <pre class="max-h-[360px] overflow-auto whitespace-pre rounded-md bg-gray-light-30 dark:bg-gray-dark-30 p-3 font-mono text-xs">{{ hex || '—' }}</pre>
      </section>
    </div>
  </div>
</template>
