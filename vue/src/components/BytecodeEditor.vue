<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref } from 'vue';

import { useLocale } from '@/composables/useLocale';
import type { CapabilityManifest, CompileError } from '@/lib/script';
import { computeErrorRanges, getCompletions, highlightBytecode } from '@/lib/script/editor';
import type { CompletionItem, ErrorRange } from '@/lib/script/editor';

const { $t } = useLocale();

const MIN_HEIGHT = 240;
const HEIGHT_STORAGE_KEY = 'vuesp:bytecode-editor:height';

const props = withDefaults(
  defineProps<{
    modelValue: string;
    manifest: CapabilityManifest;
    scriptNames?: readonly string[];
    errors?: readonly CompileError[];
  }>(),
  {
    scriptNames: () => [],
    errors: () => [],
  },
);

const emit = defineEmits<{
  'update:modelValue': [value: string];
}>();

const root = ref<HTMLDivElement | null>(null);
const content = ref<HTMLDivElement | null>(null);
const textarea = ref<HTMLTextAreaElement | null>(null);
const highlight = ref<HTMLPreElement | null>(null);
const gutter = ref<HTMLPreElement | null>(null);
const measure = ref<HTMLSpanElement | null>(null);
const height = ref(MIN_HEIGHT);
const resizing = ref(false);

const suggestions = ref<readonly CompletionItem[]>([]);
const suggestOpen = ref(false);
const suggestStart = ref(0);
const selected = ref(0);
const menuLeft = ref(0);
const menuTop = ref(0);
const hoverError = ref<{
  message: string;
  line: number;
  column: number;
  x: number;
  y: number;
} | null>(null);

let charWidth = 7.2;
let blurTimer: ReturnType<typeof setTimeout> | null = null;
let resizeStartY = 0;
let resizeStartHeight = 0;
let editorLineHeight = 24;
let editorPadLeft = 12;
let editorPadTop = 12;

const errorRanges = computed<ErrorRange[]>(() =>
  computeErrorRanges(props.modelValue, props.errors),
);
const highlighted = computed(() =>
  highlightBytecode(props.modelValue, props.manifest, errorRanges.value),
);
const lineStarts = computed(() => {
  const starts = [0];
  for (let i = 0; i < props.modelValue.length; i++) {
    if (props.modelValue[i] === '\n') starts.push(i + 1);
  }
  return starts;
});
const lineCount = computed(() => props.modelValue.split('\n').length);
const gutterHtml = computed(() => {
  const errors = new Set(props.errors.map((error) => error.pos.line));
  let html = '';
  for (let line = 1; line <= lineCount.value; line++) {
    const cls = errors.has(line)
      ? 'text-red-500 font-semibold'
      : 'text-gray-dark-25 dark:text-gray-main';
    html += `<span class="${cls}">${line}</span>\n`;
  }
  return html;
});

function kindClass(kind: CompletionItem['kind']): string {
  switch (kind) {
    case 'function':
      return 'text-blue-600 dark:text-blue-400';
    case 'builtin':
      return 'text-yellow-600 dark:text-yellow-400';
    case 'event':
      return 'text-amber-dark-75 dark:text-amber-light-45';
    case 'slot':
    case 'data':
      return 'text-cyan-dark-30 dark:text-cyan-light-40';
    case 'script':
      return 'text-green-dark-55 dark:text-green-light-25';
    case 'keyword':
      return 'text-purple-main dark:text-purple-light-30';
    default:
      return 'text-gray-dark-30 dark:text-white/70';
  }
}

function measureChar() {
  const element = measure.value;
  if (!element) return;
  const width = element.getBoundingClientRect().width / 10;
  if (width > 0) charWidth = width;

  const input = textarea.value;
  if (input) {
    const style = getComputedStyle(input);
    editorLineHeight = parseFloat(style.lineHeight) || 24;
    editorPadLeft = parseFloat(style.paddingLeft) || 12;
    editorPadTop = parseFloat(style.paddingTop) || 12;
  }
}

function hideErrorTooltip() {
  hoverError.value = null;
}

function onMouseMove(event: MouseEvent) {
  if (errorRanges.value.length === 0) {
    hideErrorTooltip();
    return;
  }
  const input = textarea.value;
  if (!input) return;

  const rect = input.getBoundingClientRect();
  const x = event.clientX - rect.left - editorPadLeft + input.scrollLeft;
  const y = event.clientY - rect.top - editorPadTop + input.scrollTop;
  if (x < 0 || y < 0) {
    hideErrorTooltip();
    return;
  }

  const line = Math.floor(y / editorLineHeight);
  const starts = lineStarts.value;
  if (line < 0 || line >= starts.length) {
    hideErrorTooltip();
    return;
  }

  const lineStart = starts[line];
  const lineEnd = starts[line + 1] ?? props.modelValue.length;
  const column = Math.floor(x / charWidth);
  const offset = Math.min(
    lineStart + Math.max(0, column),
    Math.max(lineStart, lineEnd - (lineEnd > lineStart ? 1 : 0)),
  );
  const range = errorRanges.value.find((item) => offset >= item.start && offset < item.end);
  if (!range) {
    hideErrorTooltip();
    return;
  }

  hoverError.value = {
    message: range.message,
    line: range.line,
    column: range.column,
    x: Math.min(event.clientX + 12, window.innerWidth - 300),
    y: Math.min(event.clientY + 16, window.innerHeight - 80),
  };
}

function syncScroll() {
  const input = textarea.value;
  if (!input) return;
  if (gutter.value) gutter.value.scrollTop = input.scrollTop;
  if (highlight.value) {
    highlight.value.scrollTop = input.scrollTop;
    highlight.value.scrollLeft = input.scrollLeft;
  }
}

function closeSuggestions() {
  suggestOpen.value = false;
  suggestions.value = [];
  selected.value = 0;
}

function updateMenuPosition() {
  const input = textarea.value;
  const contentEl = content.value;
  const rootEl = root.value;
  if (!input || !contentEl || !rootEl) return;

  const caret = input.selectionStart;
  const before = props.modelValue.slice(0, caret);
  const line = (before.match(/\n/g) ?? []).length;
  const lineStart = before.lastIndexOf('\n') + 1;
  const column = caret - lineStart;

  const style = getComputedStyle(input);
  const lineHeight = parseFloat(style.lineHeight) || 20;
  const paddingLeft = parseFloat(style.paddingLeft) || 12;
  const paddingTop = parseFloat(style.paddingTop) || 12;
  const contentRect = contentEl.getBoundingClientRect();
  const rootRect = rootEl.getBoundingClientRect();

  const x = contentRect.left - rootRect.left + paddingLeft + column * charWidth - input.scrollLeft;
  const y =
    contentRect.top - rootRect.top + paddingTop + line * lineHeight - input.scrollTop + lineHeight;

  menuLeft.value = Math.max(4, Math.min(x, Math.max(4, rootEl.clientWidth - 324)));
  menuTop.value = y + 4;
}

function updateSuggestions(force = false) {
  const input = textarea.value;
  if (!input) {
    closeSuggestions();
    return;
  }
  const result = getCompletions(
    props.modelValue,
    input.selectionStart,
    props.manifest,
    props.scriptNames,
    force,
  );
  if (!result) {
    closeSuggestions();
    return;
  }
  suggestions.value = result.items;
  suggestStart.value = result.start;
  selected.value = 0;
  suggestOpen.value = true;
  void nextTick(updateMenuPosition);
}

function onInput(event: Event) {
  const target = event.target as HTMLTextAreaElement;
  emit('update:modelValue', target.value);
  hideErrorTooltip();
  void nextTick(() => {
    syncScroll();
    updateSuggestions();
  });
}

function insertRange(start: number, end: number, text: string, caretOffset?: number) {
  const input = textarea.value;
  if (!input) return;
  const next = `${props.modelValue.slice(0, start)}${text}${props.modelValue.slice(end)}`;
  emit('update:modelValue', next);
  void nextTick(() => {
    input.focus();
    const position = start + (caretOffset ?? text.length);
    input.setSelectionRange(position, position);
    syncScroll();
    updateSuggestions();
  });
}

function insertAtCursor(text: string, caretOffset?: number) {
  const input = textarea.value;
  if (!input) return;
  insertRange(input.selectionStart, input.selectionEnd, text, caretOffset);
}

function acceptSuggestion(index: number) {
  const item = suggestions.value[index];
  const input = textarea.value;
  if (!item || !input) return;
  insertRange(suggestStart.value, input.selectionStart, item.insert, item.caret);
  closeSuggestions();
}

function onKeydown(event: KeyboardEvent) {
  if (event.ctrlKey && event.key === ' ') {
    event.preventDefault();
    updateSuggestions(true);
    return;
  }

  if (suggestOpen.value && suggestions.value.length > 0) {
    if (event.key === 'ArrowDown') {
      event.preventDefault();
      selected.value = (selected.value + 1) % suggestions.value.length;
      return;
    }
    if (event.key === 'ArrowUp') {
      event.preventDefault();
      selected.value =
        (selected.value - 1 + suggestions.value.length) % suggestions.value.length;
      return;
    }
    if (event.key === 'Enter' || event.key === 'Tab') {
      event.preventDefault();
      acceptSuggestion(selected.value);
      return;
    }
    if (event.key === 'Escape') {
      event.preventDefault();
      closeSuggestions();
      return;
    }
  }

  if (event.key === 'Tab' && !event.shiftKey) {
    event.preventDefault();
    insertAtCursor('  ');
  }
}

function onKeyup(event: KeyboardEvent) {
  if (event.key === 'Escape') {
    closeSuggestions();
    return;
  }
  if (['ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(event.key)) {
    updateSuggestions();
  }
}

function onScroll() {
  syncScroll();
  closeSuggestions();
  hideErrorTooltip();
}

function onBlur() {
  hideErrorTooltip();
  if (blurTimer) clearTimeout(blurTimer);
  blurTimer = setTimeout(closeSuggestions, 120);
}

function onFocus() {
  void nextTick(measureChar);
}

function maxEditorHeight(): number {
  return Math.max(MIN_HEIGHT, window.innerHeight - 220);
}

function defaultEditorHeight(): number {
  return Math.max(MIN_HEIGHT, Math.min(Math.round(window.innerHeight * 0.68), 736));
}

function loadEditorHeight(): number {
  const stored = Number(localStorage.getItem(HEIGHT_STORAGE_KEY));
  if (!Number.isFinite(stored) || stored < MIN_HEIGHT) return defaultEditorHeight();
  return Math.min(stored, maxEditorHeight());
}

function onResizeMove(event: PointerEvent) {
  if (!resizing.value) return;
  height.value = Math.max(
    MIN_HEIGHT,
    Math.min(resizeStartHeight + event.clientY - resizeStartY, maxEditorHeight()),
  );
}

function stopResize() {
  if (!resizing.value) return;
  resizing.value = false;
  localStorage.setItem(HEIGHT_STORAGE_KEY, String(height.value));
  window.removeEventListener('pointermove', onResizeMove);
  window.removeEventListener('pointerup', stopResize);
  window.removeEventListener('pointercancel', stopResize);
  document.body.style.userSelect = '';
  document.body.style.cursor = '';
}

function startResize(event: PointerEvent) {
  if (event.button !== 0) return;
  event.preventDefault();
  resizing.value = true;
  resizeStartY = event.clientY;
  resizeStartHeight = height.value;
  document.body.style.userSelect = 'none';
  document.body.style.cursor = 'ns-resize';
  window.addEventListener('pointermove', onResizeMove);
  window.addEventListener('pointerup', stopResize);
  window.addEventListener('pointercancel', stopResize);
}

onMounted(() => {
  height.value = loadEditorHeight();
  measureChar();
  document.fonts?.ready.then(measureChar).catch(() => undefined);
});

onBeforeUnmount(() => {
  if (blurTimer) clearTimeout(blurTimer);
  stopResize();
});

defineExpose({
  focus: () => textarea.value?.focus(),
  insertAtCursor,
});
</script>

<template>
  <div ref="root" class="relative w-full" :class="{ 'select-none': resizing }">
    <div
      class="flex overflow-hidden bg-gray-dark-40/[0.05] dark:bg-black/35"
      :style="{ height: `${height}px` }"
    >
      <div
        class="relative w-12 shrink-0 border-r border-gray-light-10/70 bg-black/5 dark:border-white/10 dark:bg-white/5"
      >
        <pre
          ref="gutter"
          class="m-0 h-full overflow-hidden py-4 pl-2 pr-2 text-right font-mono text-xs leading-6"
          v-html="gutterHtml"
        ></pre>
      </div>

      <div ref="content" class="relative min-w-0 flex-1">
        <pre
          ref="highlight"
          aria-hidden="true"
          class="pointer-events-none absolute inset-0 m-0 overflow-hidden whitespace-pre p-4 font-mono text-xs leading-6 text-gray-dark-30 dark:text-white/70"
          style="tab-size: 2"
          v-html="highlighted"
        ></pre>
        <textarea
          ref="textarea"
          :value="modelValue"
          wrap="off"
          spellcheck="false"
          autocapitalize="off"
          autocomplete="off"
          autocorrect="off"
          class="editor-input scrollbar absolute inset-0 h-full w-full resize-none overflow-auto whitespace-pre bg-transparent p-4 font-mono text-xs leading-6 outline-none"
          style="tab-size: 2"
          @input="onInput"
          @keydown="onKeydown"
          @keyup="onKeyup"
          @click="updateSuggestions()"
          @mousemove="onMouseMove"
          @mouseleave="hideErrorTooltip"
          @scroll="onScroll"
          @blur="onBlur"
          @focus="onFocus"
        ></textarea>
      </div>
    </div>

    <div
      class="group flex h-4 cursor-ns-resize touch-none select-none items-center justify-center border-t border-gray-light-10/70 bg-black/[0.02] transition hover:bg-black/5 dark:border-white/10 dark:bg-white/[0.02] dark:hover:bg-white/5"
      role="separator"
      aria-orientation="horizontal"
      :aria-label="$t('scriptStudio.resize')"
      :title="$t('scriptStudio.resize')"
      @pointerdown="startResize"
    >
      <span class="flex gap-1 opacity-50 transition group-hover:opacity-100">
        <span class="size-1 rounded-full bg-gray-dark-25 dark:bg-white/40" />
        <span class="size-1 rounded-full bg-gray-dark-25 dark:bg-white/40" />
        <span class="size-1 rounded-full bg-gray-dark-25 dark:bg-white/40" />
      </span>
    </div>

    <span
      ref="measure"
      aria-hidden="true"
      class="invisible absolute left-0 top-0 font-mono text-xs"
    >MMMMMMMMMM</span>

    <div
      v-if="hoverError"
      class="pointer-events-none fixed z-[120] max-w-xs rounded-md border border-red-main/40 bg-white px-2.5 py-1.5 text-xs shadow-lg dark:border-red-main/30 dark:bg-gray-dark-40 dark:text-white"
      :style="{ left: `${hoverError.x}px`, top: `${hoverError.y}px` }"
    >
      <div class="font-mono text-[10px] text-red-main">
        {{ hoverError.line }}:{{ hoverError.column }}
      </div>
      <div class="mt-0.5 whitespace-pre-wrap">{{ hoverError.message }}</div>
    </div>

    <div
      v-if="suggestOpen && suggestions.length > 0"
      class="editor-menu glass-popover scrollbar absolute z-50 max-h-60 overflow-y-auto rounded-lg p-1 text-xs"
      :style="{ left: `${menuLeft}px`, top: `${menuTop}px` }"
    >
      <button
        v-for="(item, index) in suggestions"
        :key="`${item.kind}-${item.label}`"
        type="button"
        class="flex w-full items-center justify-between gap-2 rounded px-2 py-1 text-left"
        :class="
          index === selected
            ? 'bg-blue-50 dark:bg-blue-900/30'
            : 'hover:bg-gray-light-30 dark:hover:bg-gray-dark-30'
        "
        @mousedown.prevent="acceptSuggestion(index)"
        @mouseenter="selected = index"
      >
        <span class="min-w-0 truncate" :class="kindClass(item.kind)">{{ item.label }}</span>
        <span class="max-w-[45%] shrink-0 truncate text-[10px] text-gray-dark-25 dark:text-gray-main">{{ item.detail }}</span>
      </button>
    </div>
  </div>
</template>

<style scoped>
.editor-input {
  color: transparent;
  caret-color: #374151;
  -webkit-text-fill-color: transparent;
}

:global(.dark) .editor-input {
  caret-color: #f3f4f6;
}

.editor-input::selection {
  background: rgb(59 130 246 / 0.3);
}

:deep(.editor-error) {
  border-radius: 2px;
  background: rgb(239 68 68 / 0.08);
  text-decoration: underline wavy #ef4444;
  text-decoration-thickness: 1.5px;
  text-underline-offset: 3px;
}

.editor-menu {
  width: max-content;
  min-width: 12rem;
  max-width: min(20rem, 100%);
  scrollbar-width: thin;
  scrollbar-color: var(--color-slate-main) transparent;
}
</style>
