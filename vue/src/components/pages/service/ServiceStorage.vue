<template>
  <CardGray title="File system">
    <div class="px-4 pb-2 flex items-center border-b border-gray-200 dark:border-gray-600">
      <div class="flex gap-2 items-center font-bold flex-auto">
        <div
          v-for="(value, i) of path"
          :key="value"
          class="flex items-center gap-2 cursor-pointer text-gray-600 last:cursor-default last:text-gray-900 dark:text-gray-200 dark:last:text-white"
          @click="onPrev(i)"
        >
          <div>{{ value }}</div>

          <IconNext v-if="isLast(path, i)" class="h-4 w-4"></IconNext>
        </div>
      </div>

      <div>
        <VTextFieldFile @change="onUpload"></VTextFieldFile>

        <VDropdown right="0" left="unset" top="0">
          <template #activator="{ on }">
            <IconMenu @click="on.click"></IconMenu>
          </template>

          <VList :list="mainMenu" @click="onEventService"></VList>
        </VDropdown>
      </div>
    </div>

    <div class="relative min-h-[260px]">
      <VLoader v-if="isLoading" class="absolute top-1/2 left-1/2 text-primary" />

      <VList v-slot="{ item: { name, size, isDir, isFile } }" :list="sortFiles">
        <div class="flex items-center flex-auto" @click="onNext(isDir, name)">
          <div class="mr-4">
            <IconFolder v-if="isDir"></IconFolder>
            <IconFile v-else></IconFile>
          </div>

          <div>
            <div class="text-sm">{{ isDir ? `${name}` : name }}</div>

            <div v-if="isFile" class="text-sm text-[10px] text-gray-400">{{ toByte(size) }} ({{ size }})</div>
          </div>
        </div>

        <VDropdown right="0" left="unset" top="0">
          <template #activator="{ on }">
            <button @click="on.click">
              <IconMenu></IconMenu>
            </button>
          </template>

          <VList :list="getListMenu(isDir)" @click="onEventList(name, $event)" />
        </VDropdown>
      </VList>
    </div>
  </CardGray>
</template>

<script setup lang="ts">
import { defineProps, watchEffect, defineEmits, ref, onMounted, computed, inject, nextTick } from 'vue';
import { toByte, debounce } from '@/utils/func/';

import VTextFieldFile from '@/components/general/VTextFieldFile.vue';
import VDropdown from '@/components/general/VDropdown.vue';
import VLoader from '@/components/general/VLoader.vue';
import VList from '@/components/general/VList.vue';
import CardGray from '@/components/cards/CardGray.vue';

import IconNext from '@/components/icons/IconNext.vue';
import IconMenu from '@/components/icons/IconMenu.vue';
import IconFolder from '@/components/icons/IconFolder.vue';
import IconFile from '@/components/icons/IconFile.vue';

import { DialogKey } from '@/simbol/index.ts';

import type { TypeMenu, TypeTextFieldEvent, TypeStateInfo, TypeStateFile } from '@/types/types.ts';

interface Props {
  files: TypeStateFile[];
  modelValue?: string[];
  info?: TypeStateInfo;
  progress?: [];
  url?: string;
}

const { url = '/fs', modelValue = [], info = {}, files = [] } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'update:modelValue', value: string[]): void;
  (e: 'send', value: { comm: string; data?: any }): void;
}>();

const dialog = inject(DialogKey, ({}) => {});

const mainMenu: TypeMenu[] = [
  { id: 2, name: 'Upload' },
  { id: 3, name: 'Reload' },
  { id: 4, name: 'Format' },
];
const listMenu: TypeMenu[] = [
  { id: 1, name: 'Download' },
  { id: 2, name: 'Remove' },
];

const isLoading = ref(false);

const path = computed({
  set: (value: string[]) => emit('update:modelValue', value),
  get: () => modelValue,
});

const getListMenu = (isDir: boolean) => listMenu.filter(i => (isDir ? i.id !== 1 : true));
const sortFiles = computed(() => JSON.parse(JSON.stringify(files)).sort((a: TypeStateFile, b: TypeStateFile) => (a.isFile > b.isFile ? 1 : -1)));
const getFullPath = computed(() => `${path.value.join('/').replace('root', '')}/`);
const fileName = (name: string) => `${getFullPath.value}${name}`;

const onUpdate = () => {
  isLoading.value = true;
  emit('send', { comm: 'FILES', data: { name: getFullPath.value } });
  emit('send', { comm: 'INFO' });
};

const onPrev = (index: number) => {
  if (path.value.length > index + 1) {
    path.value = path.value.filter((_, i) => i < index + 1);
    nextTick(() => onUpdate());
  }
};

const onNext = (isDir: boolean, value: string) => {
  if (isDir && path) {
    path.value.push(value);
    onUpdate();
  }
};

const onEventService = ({ id }: TypeMenu) => {
  if (id === 2) {
    const input: HTMLInputElement | null = document.querySelector('input[type="file"]');
    if (input) {
      input.click();
    }
  }
  if (id === 3) onUpdate();
  if (id === 4) onSureFormat();
};

const onEventList = (name: string, { id }: TypeMenu) => {
  if (id === 1) onDownload(name);
  if (id === 2) onSureDelete(name);
};

const onFormat = async () => {
  const res = await (await fetch(`${url}?format=true`, { method: 'POST' })).json();
  if (res?.state) onUpdate();
};

const onSureFormat = () => dialog({ value: true, message: 'All files will be deleted. Are you sure?', callback: onFormat });

const onUpload = async (data: TypeTextFieldEvent) => {
  const totalSize = data?.info?.totalSize || 0;
  const files = data?.files || [];

  const date = new FormData();
  for (let i = 0; i < files.length; i++) {
    const file: File | null = files.item(i);
    if (file) {
      const fileName = `${getFullPath.value}${file.name}`;
      date.append(`file[${i}]`, file, fileName);
    }
  }
  const { totalBytes = 0, usedBytes = 0 }: TypeStateInfo = info;
  if (totalSize < totalBytes - usedBytes) {
    const res = await (await fetch(url, { method: 'POST', body: date })).json();
    if (res?.state) onUpdate();
  } else {
    dialog({ value: true, message: 'No free space' });
  }
};

const onDelete = async (name: string) => {
  const res = await (await fetch(`${url}?file=${fileName(name)}`, { method: 'DELETE' })).json();
  if (res?.state) onUpdate();
  else dialog({ value: true, message: 'Directory is not empty' });
};

const onSureDelete = (name: string) => {
  if (fileName(name).includes('www')) {
    dialog({ value: true, message: 'The file belongs to the "www" directory. <br/> Are you sure you want to delete it?', callback: onDelete.bind(this, name) });
  } else {
    onDelete(name);
  }
};

const onDownload = (name: string) => {
  const link = document.createElement('a');
  link.setAttribute('download', name);
  link.href = `${url}?file=${fileName(name)}`;
  document.body.appendChild(link);
  link.click();
  link.remove();
};

const isLast = (path: string[], i: number) => path.length > i + 1;

const onLoad = debounce(() => {
  isLoading.value = false;
}, 300);

watchEffect(() => {
  onLoad(files);
});

onMounted(() => {
  if (!files.length) onUpdate();
});
</script>
