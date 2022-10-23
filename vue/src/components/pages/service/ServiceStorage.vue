<template>
  <div class="files">
    <div class="files__path d-flex a-center">
      <div class="files__route d-flex gap-4 a-center text-h5 fw-600 grey-base">
        <div v-for="(value, i) of path" :key="value" class="files__route-item" @click="onPrev(i)">
          <div class="mr-2">{{ value }}</div>
          <v-icons icon="next"></v-icons>
        </div>
      </div>
      <div>
        <v-input-file @change="onUpload"></v-input-file>
        <v-dropdown right="0" left="unset" top="0">
          <template #activator="{ on }">
            <v-icons icon="menu" @click="on.click"></v-icons>
          </template>
          <v-list :list="mainMenu" @click="onEventServise"></v-list>
        </v-dropdown>
      </div>
    </div>
    <div class="files__list">
      <v-loader v-if="isLoading" />
      <ul class="v-list">
        <li v-for="{ name, size, isDir, isFile } of sortFiles" :key="`file_${name}`" class="v-list__item d-flex a-center pa-0">
          <div class="d-flex a-center v-spacer my-2" @click="onNext(isDir, name)">
            <div class="mr-4">
              <v-icons :icon="isDir ? `folder` : 'file'"></v-icons>
            </div>
            <div class="v-spacer">
              <div class="text-body-1 fw-600">{{ isDir ? `${name}` : name }}</div>
              <div v-if="isFile" class="text-caption grey-base">{{ toByte(size) }} ({{ size }})</div>
            </div>
          </div>
          <v-dropdown right="0" left="unset" top="0">
            <template #activator="{ on }">
              <v-icons icon="menu" @click="on.click"></v-icons>
            </template>
            <v-list :list="getListMenu(isDir)" @click="onEventList(name, $event)" />
          </v-dropdown>
        </li>
      </ul>
    </div>
  </div>
</template>

<script setup>
import { defineProps, watchEffect, defineEmits, ref, onMounted, computed, inject, nextTick } from 'vue';
import { toByte, debounce } from '@/utils/func/';

const props = defineProps({
  modelValue: { type: Array, default: () => [] },
  files: { type: Array, default: () => [] },
  info: { type: Object, default: () => ({}) },
  progress: { type: Object, default: () => ({}) },
  url: { type: String, default: '/fs' },
});

const emit = defineEmits(['update:modelValue', 'send']);
const dialog = inject('dialog');

const mainMenu = [
  { id: 2, name: 'Upload' },
  { id: 3, name: 'Reload' },
  { id: 4, name: 'Format' },
];
const listMenu = [
  { id: 1, name: 'Download' },
  { id: 2, name: 'Remove' },
];

const filesTemp = ref([]);
const isLoading = ref(false);

const path = computed({
  set: value => emit('update:modelValue', value),
  get: () => props.modelValue,
});

const getListMenu = isDir => listMenu.filter(i => (isDir ? i.id !== 1 : true));
const sortFiles = computed(() => JSON.parse(JSON.stringify(filesTemp.value)).sort((a, b) => (a.isFile > b.isFile ? 1 : -1)));
const getFullPath = computed(() => `${path.value.join('/').replace('root', '')}/`);
const fileName = name => `${getFullPath.value}${name}`;

const onUpdate = () => {
  isLoading.value = true;
  emit('send', { comm: 'FILES', data: { name: getFullPath.value } });
  emit('send', { comm: 'INFO' });
};

const onPrev = index => {
  if (path.value.length > index + 1) {
    path.value = path.value.filter((_, i) => i < index + 1);
    nextTick(() => onUpdate());
  }
};

const onNext = (isDir, value) => {
  if (isDir && path) {
    path.value.push(value);
    onUpdate();
  }
};

const onEventServise = ({ id }) => {
  if (id === 2) document.querySelector('input[type="file"]').click();
  if (id === 3) onUpdate();
  if (id === 4) onSureFormat();
};

const onEventList = (name, { id }) => {
  if (id === 1) onDownload(name);
  if (id === 2) onSureDelete(name);
};

const onFormat = async () => {
  const res = await (await fetch(`${props.url}?format=true`, { method: 'POST' })).json();
  if (res?.state) onUpdate();
};

const onSureFormat = () => dialog({ message: 'All files will be deleted. Are you sure?', callback: onFormat });

const onUpload = async ({ files, info }) => {
  const totalSize = info?.totalSize || 0;
  const date = new FormData();
  for (let i = 0; i < files.length; i++) {
    const file = files.item(i);
    const fileName = `${getFullPath.value}${file.name}`;
    date.append(`file[${i}]`, file, fileName);
  }
  const { totalBytes, usedBytes } = props.info;
  if (totalSize < totalBytes - usedBytes) {
    const res = await (await fetch(props.url, { method: 'POST', body: date })).json();
    if (res?.state) onUpdate();
  } else {
    dialog({ message: 'No free space' });
  }
};

const onDelete = async name => {
  const res = await (await fetch(`${props.url}?file=${fileName(name)}`, { method: 'DELETE' })).json();
  if (res?.state) onUpdate();
  else dialog({ message: 'Directory is not empty' });
};

const onSureDelete = name => {
  if (fileName(name).includes('www')) {
    dialog({ message: 'The file belongs to the "www" directory. <br/> Are you sure you want to delete it?', callback: onDelete.bind(this, name) });
  } else {
    onDelete(name);
  }
};

const onDownload = name => {
  const link = document.createElement('a');
  link.setAttribute('download', name);
  link.href = `${props.url}?file=${fileName(name)}`;
  document.body.appendChild(link);
  link.click();
  link.remove();
};

const onLoad = debounce(() => {
  filesTemp.value = props.files;
  isLoading.value = false;
}, 300);

watchEffect(() => {
  onLoad(props.files);
});

onMounted(() => {
  if (!props.files.length) onUpdate();
});
</script>

<style lang="scss">
.files {
  min-height: 400px;
  &__path {
    width: 100%;
    border-top: 1px solid var(--border-1);
    border-bottom: 1px solid var(--border-1);
  }
  &__icons {
    svg {
      height: 24px;
    }
    cursor: pointer;
  }
  &__route {
    width: 100%;
    user-select: none;
    height: 60px;

    &-item {
      display: flex;
      &:not(:last-child) {
        cursor: pointer;
      }
    }
    svg {
      height: 16px;
    }
  }
  &__list {
    position: relative;
    margin: 0 0 20px 0;
  }
}
</style>
