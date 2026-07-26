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

      <div class="flex flex-col sm:flex-row gap-3 sm:gap-4">
        <aside class="w-full sm:w-56 md:w-80 lg:w-100 flex-shrink-0 flex flex-col">
          <card-main>
            <VTabs class="flex-auto min-h-75" :value="tabValue" :items="tabs" @click="onTab">
              <template #header="{ tab, i }">
                <div class="flex items-center justify-center gap-2 px-2">
                  <icon-ri-file-text-line v-if="i === 0" class="lg:hidden" />
                  <icon-ri-presentation-line v-if="i === 1" class="lg:hidden" />
                  <icon-ri-stack-line v-if="i === 2" class="lg:hidden" />
                  <span class="hidden lg:flex">
                    {{ tab.title }}
                  </span>
                </div>
              </template>

              <template #tab-0>
                <ul class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-2">
                  <li v-for="script of scripts" :key="script.id" class="group text-sm cursor-pointer transition-all flex items-center justify-between py-2 hover:opacity-80" @click="onSelect(script)">
                    <div class="flex items-center gap-2 min-w-0">
                      <span>#{{ script.id }}</span>
                      <span class="truncate">{{ script.name }}</span>
                    </div>

                    <div class="group flex items-center gap-1">
                      <v-button color="transparent" class="size-6" @click.stop="onRemove(script)">
                        <icon-ri-delete-bin-line class="text-gray-500 hover:text-red-400 group-hover:opacity-100 opacity-40 transition-all"></icon-ri-delete-bin-line>
                      </v-button>
                    </div>
                  </li>

                  <li class="text-sm text-slate-400 italic text-center">
                    <v-button class="w-full" size="sm" @click="onAddScriptDialog">+ {{ $t('add') }}</v-button>
                  </li>

                  <li v-if="scripts.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noScripts') }}</li>
                </ul>

                <div class="px-2 border-t border-gray-200 dark:border-gray-700 text-slate-400 flex justify-between mt-4 text-xs">
                  <span>{{ scripts.length }} {{ $t('script', { count: `${scripts.length}` }) }}</span>
                </div>
              </template>

              <template #tab-1>
                <ul class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-2">
                  <li
                    v-for="(example, i) of examples"
                    :key="example.name"
                    class="group text-sm cursor-pointer transition-all flex items-center justify-between py-2 hover:opacity-80"
                    @click="onExample(example)"
                  >
                    <div class="flex items-center gap-2 min-w-0">
                      <span>#{{ i + 1 }}</span>
                      <span class="truncate">{{ example.name }}</span>
                    </div>
                  </li>
                  <li v-if="examples.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noScripts') }}</li>
                </ul>
              </template>

              <template #tab-2>
                <ul class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-2 max-h-60 scrollbar">
                  <li v-for="(slot) in main.slots" :key="slot.index" class="group text-sm transition-all flex items-center justify-between py-1">
                    <div class="flex items-center justify-between gap-2 flex-auto">
                      <div class="flex gap-1 flex-auto text-gray-500" :title="slot.text">
                        <div class="flex-[0_0_6px] h-5 w-1 rounded" :class="getColorSlot(slot)"></div>

                        <div>{{ `${slot.index})` }} {{ slot.id !== 255 ? `id:${slot.id}` : '' }}</div>
                        <span class="block truncate w-50 overflow-hidden">{{ slot.text }}</span>
                      </div>

                      <div v-if="slot.id !== 255" class="group flex items-center gap-1">
                        <v-button color="transparent" class="size-6 px-0!" size="icon" :disabled="!slot.active" @click="onStopScript(slot)">
                          <icon-ri-stop-circle-line class="text-gray-500 hover:text-blue-500 group-hover:opacity-100 opacity-20 transition-all size-6"></icon-ri-stop-circle-line>
                        </v-button>

                        <v-button color="transparent" class="size-6 px-0!" size="icon" :disabled="!slot.text" @click="onRemoveScript(slot)">
                          <icon-ri-close-circle-line class="text-gray-500 hover:text-red-500 group-hover:opacity-100 opacity-20 transition-all size-6"></icon-ri-close-circle-line>
                        </v-button>
                      </div>
                    </div>
                  </li>
                </ul>

                <div class="px-2 border-t border-gray-200 dark:border-gray-700 text-slate-400 flex justify-between mt-4 text-xs">
                  <span>{{ slotInfo }}</span>
                </div>
              </template>
            </VTabs>
          </card-main>
        </aside>

        <div class="flex-1 flex flex-col gap-3 sm:gap-4 min-h-0">
          <card-main :title="`${$t('editor')}  ${selectedScript?.name ? selectedScript?.name : ''}`">
            <template #header>
              <div class="flex gap-3">
                <v-button color="transparent" class="size-6" size="icon" :disabled="selectedScript?.content === content" @click="onSaveScript()">
                  <icon-ri-save-line class="text-gray-500"></icon-ri-save-line>
                </v-button>
                <v-button color="transparent" class="size-6" size="icon" :disabled="!content" @click="onRunScript()">
                  <icon-ri-play-circle-line class="text-gray-500"></icon-ri-play-circle-line>
                </v-button>
                <v-button color="transparent" class="size-6" size="icon" :disabled="!content" @click="onStopScriptAll()">
                  <icon-ri-stop-circle-fill class="text-gray-500"></icon-ri-stop-circle-fill>
                </v-button>
              </div>
            </template>

            <div class="flex-auto bg-gray-50 dark:bg-gray-900 dark:border-gray-700 left-0 w-full sticky top-0">
              <textarea
                v-model="content"
                class="w-full h-full min-h-50 p-4 text-sm outline-0 relative rounded-md border border-gray-200 dark:border-gray-800 flex flex-col field-sizing-content"
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

          <card-main :title="$t('logs')">
            <template #header>
              <v-button color="transparent" class="size-6 opacity-60 hover:opacity-100" size="icon" @click="logs = []">
                <icon-ri-eraser-line class="text-gray-500"></icon-ri-eraser-line>
              </v-button>
            </template>

            <ul ref="container" class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-4 max-h-50 min-h-32 scrollbar">
              <li v-for="(log, idx) in logs" :key="idx" class="py-0.5 flex items-start gap-2 border-b border-slate-100/50 last:border-0 text-sm">
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
import { computed, onMounted, ref, watch } from 'vue';
import { timeUtcToString } from 'vuesp-components/helpers';
import { ScriptType, examples, formatScript, removeLBScript } from '@/assets/js/script';

import { useConnection } from '@/composables/useConnection';
import { required, maxLen } from '@/utils/validate';

import { useForm } from 'vuesp-components/composables';
import type { IListItem, IMessageMessage, ValidationSchema } from 'vuesp-components/types';
import { VTabs } from 'vuesp-components';

import { useLocale } from '@/composables/useLocale';
import { useFetch } from '@vueuse/core';
import { useRoute, useRouter } from 'vue-router';
const router = useRouter();
const route = useRoute();

const { $t } = useLocale();

const { message, main, onSend } = useConnection((send) => {
  send(KEYS.MESSAGE, { ...message.value, id: 0, type: ScriptType.SCRIPT_GET_ALL_SLOT });
});

const hash = (route.hash || '').replace('#', '');

const tabValue = ref(+hash || 0);
const tabs = [{ title: $t('list') }, { title: $t('examples') }, { title: $t('slot') }];

const onTab = (i: number) => {
  router.push({ hash: `#${i}` });
};

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

watch(
  () => message.value,
  (v) => {
    if (v.type === 0) {
      const date = new Date();
      logs.value.unshift({ time: `${timeUtcToString(date, { minute: '2-digit', hour: '2-digit', second: '2-digit' })}`, type: 0, text: v?.text || '' });
    }
  }
);

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

const getColorSlot = (slot: IMessageMessage) => {
  if (slot.active) return 'bg-violet-500';
  if (slot.handler) return 'bg-blue-500';
  return 'bg-green-500';
};

const onRunScript = () => {
  const id = selectedScript.value?.id || 0;
  onSend('MESSAGE', { ...message.value, id, type: ScriptType.SCRIPT_START, text: removeLBScript(content.value) });
};
const onStopScript = (slot: IMessageMessage) => {
  onSend('MESSAGE', { ...slot, type: ScriptType.SCRIPT_STOP, text: '' });
};
const onStopScriptAll = () => {
  const id = selectedScript.value?.id || 0;
  onSend('MESSAGE', { ...message.value, id, type: ScriptType.SCRIPT_STOP_ALL, text: content.value });
};
const onRemoveScript = (slot: IMessageMessage) => {
  onSend('MESSAGE', { ...slot, type: ScriptType.SCRIPT_REMOVE, text: '' });
};

const scripts = ref<IScript[]>([]);
const selectedScript = ref<IScript | null>(null);
const content = ref('');

function onSelect(script: IScript) {
  selectedScript.value = script;
  content.value = formatScript(script.content);
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
  selectedScript.value = null;
  content.value = formatScript(item.value as string);
};
</script>

<!-- 
if:$v0=5;$v0=0;end;
if:$v0<10;$v0=$v0+1;end;
if:$v0>=0;$display='POS';end;
if:$v0<=0;$display='NEG';end;
if:$v0!=0;$display='NOT ZERO';end;if:$s0!='OFF';$p13=255;end;
if:$v0>5&&$v0<10;$display='BETWEEN';end;
if:$v0==0||$v1==0;$display='ZERO';end;


$v0=1;$v1=1;if:$v0==1;if:$v1==1;$display='BOTH';end;end;
$v0=5;if:$v0>0;if:$v0<10;$display='BETWEEN';end;end;
$v0=$p13;if:$v0==0;$display='ZERO';else;if:$v0<128;$display='LOW';else;$display='HIGH';end;end;
$v0=0;while:$v0<5;$v0=$v0+1;end;
$v0=10;while:$v0>0;$v0=$v0-1;end;
$v0=0;while:$v0<10;$v0=$v0+1;if:$v0==5;$v0=10;end;end;
$v0=0;while:$v0<10;$p13=$v0;$v0=$v0+1;end;
$v0=0;while:$v0<256;$p13=$v0;$v0=$v0+10;end;
$v0=0;while:$v0<100;$v0=$v0+1;if:$v0>50;$v0=100;end;end;
$v0=get($a0,0);
$v0=get($a0,1);
$v0=len($a0);
set($a0,$v0);
$s0=chr(65);
$v0=ord($s0);
$v0=len($s0);
$s0=$s0+'World';
$s0=$s0+' ';
if:$v0>128;$p13=0;else;$p13=255;end;
while:$v0<10;$p13=$v0;$v0=$v0+1;end;
while:$v0<256;$p13=$v0;$v0=$v0+1;end;
$p13=255;wait(1000);$p13=0;wait(500);
$p13=255;wait(5s);$p13=0;wait(1m);
wait(100u);
wait(100);
wait(1s);
wait(5s);
wait(1m);
wait(1h);
on('EVENT',if:$p13==0;call(1);else;call(2);end;end);
$display=$s0+'World';
$v0=0;while:$v0<5;$display=$v0;$v0=$v0+1;end;
on('CLICK',$display='CLICKED';end);
on('START',$p13=255;wait(1000);$p13=0;end);
on('BUTTON',if:$p13==0;$display='LOW';end;end);
on('STOP',$p13=0;$display='STOPPED';end);
on('ALARM',$v0=0;while:$v0<5;$p13=255;wait(100);$p13=0;wait(100);$v0=$v0+1;end;end);
on('RESET',$v0=0;$p13=0;$display='RESET';end);
on('BUTTON',$p13=255;wait(1000);$p13=0;end);
if:$p0==0;$v0=$v0+1;$display=$v0;wait(200);end;
if:$temperature>25;$p13=255;else;$p13=0;end;
if:$s0=='START';while:$v0<10;$p13=$v0;$v0=$v0+1;end;end;
$v0=0;while:$v0<10;if:$v0%2==0;$display=$v0;end;$v0=$v0+1;end;
$v0=10;while:$v0>0;$display=$v0;$v0=$v0-1;wait(1000);end;$display='DONE';
$v0=0;while:$v0<5;$p13=255;wait(100s);$p13=0;wait(100s);$v0=$v0+1;end;
 -->
