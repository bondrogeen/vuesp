<template>
  <div class="container mx-auto">
    <div class="flex flex-col gap-3 sm:gap-4 mx-auto">
      <div class="flex justify-between">
        <h1>{{ $t('menu.script') }}</h1>

        <v-select :items="listMenu" @change="onMenu">
          <template #activator="{ on }">
            <v-button color="" type="icon" @click="on.click">
              <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
            </v-button>
          </template>
        </v-select>
      </div>

      <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-[400px_1fr] 2xl:grid-cols-3 gap-3 sm:gap-4">
        <card-main :title="$t('list')" class="order-2 md:order-1">
          <template #header>
            <div class="flex gap-3">
              <v-button color="transparent" class="size-6 text-gray-500" :title="$t('add')" :disabled="selectedScript?.content === content" @click="onAddScriptDialog()">
                <icon-ri-sticky-note-add-line class="rotate-90"></icon-ri-sticky-note-add-line>
              </v-button>
            </div>
          </template>

          <ul class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-2 max-h-50 scrollbar">
            <li v-for="script of scripts" :key="script.id" class="group text-sm cursor-pointer transition-all flex items-center justify-between hover:opacity-80 px-1" @click="onSelect(script)">
              <div class="flex items-center gap-2 min-w-0">
                <span>#{{ script.id }}</span>
                <span class="truncate">{{ script.name }}</span>
              </div>

              <div class="group flex items-center gap-1">
                <v-button
                  color="transparent"
                  class="size-6"
                  :class="isScriptSave(script) ? 'text-gray-500' : 'text-blue-500 animate-shake'"
                  :title="$t('save')"
                  :disabled="isScriptSave(script)"
                  @click="onSaveScript"
                >
                  <icon-ri-save-line class=""></icon-ri-save-line>
                </v-button>
                <v-button color="transparent" class="size-6 text-gray-500 hover:text-red-400" :title="$t('remove')" @click.stop="onRemove(script)">
                  <icon-ri-delete-bin-line class="transition-all"></icon-ri-delete-bin-line>
                </v-button>
              </div>
            </li>

            <li v-if="scripts.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noScripts') }}</li>
          </ul>

          <div class="px-2 border-t border-gray-200 dark:border-gray-700 text-slate-400 flex justify-between mt-4 text-xs">
            <span>{{ scripts.length }} {{ $t('script', { count: `${scripts.length}` }) }}</span>
          </div>

          <div class="mt-4">
            <div class="flex items-center justify-between">
              <h3 class="text-lg font-semibold text-gray-800 dark:text-white/90 mb-2">{{ $t('slots') }}</h3>

              <div class="flex gap-2">
                <v-button color="transparent" class="size-5 px-0! text-gray-500 not-disabled:hover:text-blue-600" :disabled="!currentSlot?.active" @click="onStopScript">
                  <icon-ri-stop-circle-line class="transition-all size-5"></icon-ri-stop-circle-line>
                </v-button>

                <v-button color="transparent" class="size-5 px-0! text-gray-500 not-disabled:hover:text-red-600" :disabled="!currentSlot?.text" @click="onRemoveScript">
                  <icon-ri-close-circle-line class="transition-all size-5"></icon-ri-close-circle-line>
                </v-button>

                <v-button color="transparent" class="size-5" :title="$t('update')" @click="onUpdateScript">
                  <icon-ri-refresh-line class="text-gray-500"></icon-ri-refresh-line>
                </v-button>

                <v-button color="transparent" class="size-5" :title="$t('stop')" :disabled="!slotInfo.used" @click="onStopScriptAll">
                  <icon-ri-stop-circle-fill class="text-gray-500"></icon-ri-stop-circle-fill>
                </v-button>
              </div>
            </div>

            <ul class="overflow-y-auto flex flex-wrap gap-1 mb-2">
              <li v-for="slot in main.slots" :key="slot.index" class="group text-sm transition-all flex items-center rounded">
                <v-button class="px-0! rounded-none" :class="getColorSlot(slot)" color="" size="none" @click="onSlot(slot)">
                  <div class="flex-[0_0_8px] h-4 w-2"></div>
                </v-button>
              </li>
            </ul>
          </div>

          <div class="px-2 border-t border-gray-200 dark:border-gray-700 text-slate-400 flex text-xs">
            <span>{{ `${$t('total')}: ${slotInfo.total} ${$t('used')}: ${slotInfo.used}` }}</span>
          </div>

          <div v-if="currentSlot?.handler || currentSlot?.active" class="text-slate-200 text-sm line-clamp-1 mt-4">
            <span class="me-2 text-slate-400">#{{ currentSlot?.id }}</span>
            <span>{{ currentSlot?.text }}</span>
          </div>
        </card-main>

        <card-main :title="`${$t('editor')}  ${selectedScript?.name ? selectedScript?.name : ''}`" class="order-1">
          <template #header>
            <div class="flex gap-3">
              <v-button
                color="transparent"
                class="size-6"
                :class="!Boolean(content.trim()) ? 'text-gray-500' : 'text-green-500'"
                :title="$t('run')"
                :disabled="!Boolean(content.trim())"
                @click="onRunScript()"
              >
                <icon-ri-play-circle-line />
              </v-button>

              <v-select :items="examples" @change="onExample">
                <template #activator="{ on }">
                  <v-button color="transparent" class="size-6" :disabled="!selectedScript" @click="on.click">
                    <icon-ri-file-list-3-line class="text-gray-500"></icon-ri-file-list-3-line>
                  </v-button>
                </template>
              </v-select>
            </div>
          </template>

          <div class="flex-auto bg-gray-50 dark:bg-gray-900 dark:border-gray-700 left-0 w-full sticky top-0">
            <textarea
              v-model="content"
              class="w-full h-full min-h-60 p-4 text-sm outline-0 relative rounded-md border border-gray-200 dark:border-gray-800 flex flex-col field-sizing-content"
              spellcheck="false"
            ></textarea>
          </div>

          <div class="text-sm border-t border-gray-200 dark:border-gray-700 text-slate-400 flex flex-wrap justify-between mt-4">
            <span>
              <i class="far fa-file-alt mr-1"></i>
              {{ content ? `${$t('length')}: ${content?.length || 0}` : '' }}
            </span>

            <!-- <ul>
                <li v-for="error of errors" :key="error.position">{{ $t(`error.${error.key}`) }}</li>
              </ul> -->
          </div>
        </card-main>

        <card-main :title="$t('logs')" class="order-1 md:col-span-2 2xl:col-span-1">
          <template #header>
            <v-button color="transparent" class="size-6 opacity-60 hover:opacity-100" size="icon" @click="logs = []">
              <icon-ri-eraser-line class="text-gray-500"></icon-ri-eraser-line>
            </v-button>
          </template>

          <ul
            ref="container"
            class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-4 max-h-50 min-h-60 scrollbar"
            @mouseenter="onHover(true)"
            @mouseleave="onHover(false)"
          >
            <li v-for="(log, idx) in logs" :key="idx" class="py-0.5 flex items-start gap-2 border-b border-slate-600/50 last:border-0 text-sm">
              <span class="text-slate-400 whitespace-nowrap">{{ log.time }}</span>
              <span>
                {{ log.text }}
              </span>
            </li>
            <li v-if="logs.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noLogs') }}</li>
          </ul>
        </card-main>
      </div>
    </div>

    <v-dialog v-if="dialogAdd" size="sm" :title="`${$t('add')} #${idScript}`" @close="dialogAdd = false">
      <div class="my-4">
        <v-text-field v-model="name" v-bind="nameProps" :label="$t('name')" />

        <v-button class="w-full" color="blue" :disabled="!idScript || !name" @click="onSubmit">
          {{ $t('add') }}
        </v-button>
      </div>
    </v-dialog>
  </div>
</template>

<script setup lang="ts">
import { KEYS } from '@/types';
import type { IScript, ILog } from './types';
import { computed, nextTick, onMounted, ref, useTemplateRef, watch } from 'vue';
import { timeUtcToString } from 'vuesp-components/helpers';
import { ScriptType, examples, formatScript, removeLBScript } from '@/assets/js/script';

import { useConnection } from '@/composables/useConnection';
import { required, maxLen } from '@/utils/validate';

import { useForm } from 'vuesp-components/composables';
import type { IListItem, IMessageMessage, ValidationSchema } from 'vuesp-components/types';

import { useLocale } from '@/composables/useLocale';
import { useFetch } from '@vueuse/core';

const { $t } = useLocale();

const { message, main, onSend } = useConnection((send) => {
  send(KEYS.MESSAGE, { ...message.value, id: 0, type: ScriptType.SCRIPT_GET_ALL_SLOT });
});

const dialogAdd = ref(false);
const logs = ref<ILog[]>([]);

const listMenu: IListItem[] = [
  { name: $t('save'), value: 1 },
  { name: $t('restore'), value: 2 },
];

const onMenu = (item: IListItem) => {
  if (item.value === 1) onSave();
  if (item.value === 2) onLoad();
};

const { defineField, handleSubmit } = useForm({
  validationSchema: () =>
    ({
      name: [required, maxLen(8)],
    }) as ValidationSchema,
});
const idScript = ref('0');
const [name, nameProps] = defineField<string>('name');

const addScript = () => {
  scripts.value.push({ id: +idScript.value, name: name.value, content: '' });
};

const onSubmit = handleSubmit(() => {
  addScript();
  name.value = ``;
  dialogAdd.value = false;
});

const ids = computed(() => scripts.value.map(({ id }) => id));

const onAddScriptDialog = () => {
  const getId = Array.from({ length: 256 }, (_, i) => i + 1).find((i) => !ids.value.includes(i));
  idScript.value = `${getId}`;
  dialogAdd.value = true;
};
const onRemove = ({ id }: IScript) => {
  scripts.value = scripts.value.filter((i) => i.id !== id);
};
const onSaveScript = () => {
  if (!selectedScript.value) return;
  const { id } = selectedScript.value;
  scripts.value = scripts.value.map((i) => (i.id === id ? { ...i, content: content.value } : i));
};

const isHover = ref(false);
const container = useTemplateRef('container');
const scrollLastLog = () => {
  if (!container.value?.lastElementChild) return;
  if (isHover.value) return;
  container.value.scrollTop = container.value.scrollHeight;
};

const onHover = (value: boolean) => {
  isHover.value = value;
};

watch(
  () => message.value,
  (v) => {
    if (v.type === 0) {
      const date = new Date();
      logs.value.push({ time: `${timeUtcToString(date, { minute: '2-digit', hour: '2-digit', second: '2-digit' })}`, type: 0, text: v?.text || '' });
      nextTick(() => {
        scrollLastLog();
      });
    }
  }
);

const currentSlot = ref<IMessageMessage | null>(null);
const slotInfo = computed(() => {
  return Object.values(main.value.slots).reduce(
    (acc, i: IMessageMessage) => {
      if (i.active || i.handler) {
        acc.used = acc.used + 1;
      }
      acc.total = acc.total + 1;
      return acc;
    },
    { total: 0, used: 0 }
  );
});

const onSlot = (slot: IMessageMessage) => {
  currentSlot.value = slot;
};
const getColorSlot = (slot: IMessageMessage) => {
  if (slot.active) return 'bg-violet-500';
  if (slot.handler) return 'bg-blue-500';
  return 'bg-gray-200 dark:bg-gray-500 ';
};

const onRunScript = () => {
  const id = selectedScript.value?.id || 0;
  onSend(KEYS.MESSAGE, { ...message.value, id, type: ScriptType.SCRIPT_START, text: removeLBScript(content.value) });
};
const onStopScript = () => {
  onSend(KEYS.MESSAGE, { ...currentSlot.value, type: ScriptType.SCRIPT_STOP, text: '' });
};
const onStopScriptAll = () => {
  onSend(KEYS.MESSAGE, { ...message.value, type: ScriptType.SCRIPT_STOP_ALL, text: '' });
  setTimeout(() => {
    onUpdateScript();
  }, 500);
};
const onRemoveScript = () => {
  onSend(KEYS.MESSAGE, { ...currentSlot.value, type: ScriptType.SCRIPT_REMOVE, text: '' });
};
const onUpdateScript = () => {
  onSend(KEYS.MESSAGE, { ...message.value, id: 0, type: ScriptType.SCRIPT_GET_ALL_SLOT });
};

const scripts = ref<IScript[]>([]);
const selectedScript = ref<IScript | null>(null);
const content = ref('');

const isScriptNotSave = computed(() => {
  if (!selectedScript.value?.content) return false;
  if (formatScript(selectedScript.value?.content) === content.value) return false;
  return true;
});

function onSelect(script: IScript) {
  // if (isScriptNotSave.value) return;
  selectedScript.value = script;
  content.value = formatScript(script.content);
}
function isScriptSave(script: IScript) {
  if (selectedScript.value?.id === script.id && formatScript(selectedScript.value?.content) !== content.value) return false;
  return true;
}

// const validate = computed(() => ScriptValidatorAPI.validate(selectedScriptContent.value || ''));
// const isValid = computed(() => validate.value.valid);
// const errors = computed(() => validate.value.errors);

const PATH = '/scripts.txt';

const onLoad = async () => {
  const { data } = await useFetch(`/fs?file=${PATH}`).text();
  if (!data.value) return;
  const lines = data.value.split('\n');
  scripts.value = lines.map((script): IScript => {
    const [id, name, content] = script.split(':', 3);
    return { id: +id, name, content };
  });
};

const onSave = async () => {
  const text = scripts.value.map(({ id, name, content }) => `${id}:${name}:${removeLBScript(content)}`).join('\n');
  const body = new FormData();
  body.append('file[0]', new Blob([text], { type: 'text/plain' }), PATH);
  return await useFetch('/fs', { body }).post();
};

onMounted(() => {
  onLoad();
});

const onExample = (item: IListItem) => {
  content.value = formatScript(item.value as string);
};
</script>
