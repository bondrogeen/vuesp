<template>
  <div class="files">
    <div class="d-flex align-center mb-6">
      <div class="text-h4 v-spacer">File system</div>
      <v-input-file :info="info" :path="getFullPath" @submit="onUpload" @error="onMessage">
        <v-icons icon="plus" class="mr-4"></v-icons>
      </v-input-file>
      <v-dropdown>
        <template #activator="{ on }">
          <v-icons icon="menu" @click="on.click"></v-icons>
        </template>
        <v-list :list="mainMenu" @click="onEventServise"></v-list>
      </v-dropdown>
    </div>
    <div class="files__route d-flex gap-4 align-center text-h5 fw-600 grey-base">
      <div class="files__route-item" @click="onPrev(0)">
        <div class="mr-2">/</div>
        <v-icons icon="next"></v-icons>
      </div>
      <div v-for="(value, i) of path" :key="value" class="files__route-item" @click="onPrev(i + 1)">
        <div class="mr-2">{{ value }}</div>
        <v-icons icon="next"></v-icons>
      </div>
    </div>
    <div class="files__list">
      <v-loader v-if="isLoading" />
      <ul class="list">
        <li v-for="{ name, size, isDir, isFile } of sortFiles" :key="`file_${name}`" class="list__item">
          <div class="list__inner" @click="onNext(isDir, name)">
            <div class="mr-4">
              <v-icons :icon="isDir ? `folder` : 'file'"></v-icons>
            </div>
            <div class="v-spacer">
              <div class="text-body-1 fw-600">{{ isDir ? `${name}` : name }}</div>
              <div v-if="isFile" class="text-body-2 grey-base">{{ toByte(size) }} ({{ size }})</div>
            </div>
          </div>
          <v-dropdown>
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
import { defineProps, watchEffect, defineEmits, ref, onMounted, computed } from 'vue';
import { toByte, debounce } from '@/utils/func/';

const props = defineProps({
  files: { type: Array, default: () => [] },
  info: { type: Object, default: () => ({}) },
  progress: { type: Object, default: () => ({}) },
  url: { type: String, default: '/fs' },
});

const emit = defineEmits(['send', 'clear', 'message']);

const mainMenu = [
  { id: 1, name: 'Create directory' },
  { id: 2, name: 'Upload files' },
  { id: 3, name: 'Format' },
];
const listMenu = [
  { id: 1, name: 'Download' },
  { id: 2, name: 'Remove' },
];

const path = ref([]);
const filesTemp = ref([]);
const isLoading = ref(false);

const getListMenu = isDir => listMenu.filter(i => (isDir ? i.id !== 1 : true));

const onUpdate = e => {
  isLoading.value = true;
  emit('clear', e);
  emit('send', { comm: 'FILES', data: { name: getFullPath.value } });
  emit('send', { comm: 'INFO' });
};

const onLoad = debounce(() => {
  filesTemp.value = props.files;
  isLoading.value = false;
}, 300);

const onPrev = index => {
  if (path.value.length > index) {
    path.value = path.value.filter((_, i) => i < index);
    onUpdate();
  }
};

const onNext = (isDir, value) => {
  if (isDir && path) {
    path.value.push(value);
    onUpdate();
  }
};

const fileName = name => `${getFullPath.value}${name}`;

const onEventServise = ({ id }) => {
  if (id === 3) onFormat();
};

const onEventList = (name, { id }) => {
  if (id === 1) onDownload(name);
  if (id === 2) onDelete(name);
};

const onFormat = async () => {
  const res = await (await fetch(`${props.url}?format=true`, { method: 'POST' })).json();
  if (res?.state) onUpdate();
  else onMessage({ message: 'Directory is not empty' });
};

const onUpload = async formData => {
  const res = await (await fetch(props.url, { method: 'POST', body: formData })).json();
  if (res?.state) onUpdate();
};

const onDelete = async name => {
  const res = await (await fetch(`${props.url}?file=${fileName(name)}`, { method: 'DELETE' })).json();
  if (res?.state) onUpdate();
  else onMessage({ message: 'Directory is not empty' });
};

const onDownload = name => {
  const link = document.createElement('a');
  link.setAttribute('download', name);
  link.href = `${props.url}?file=${fileName(name)}`;
  document.body.appendChild(link);
  link.click();
  link.remove();
};

const sortFiles = computed(() => {
  const arr = JSON.parse(JSON.stringify(filesTemp.value));
  return arr.sort((a, b) => {
    return a.isFile > b.isFile ? 1 : -1;
  });
});

const onMessage = e => emit('message', e);
const getFullPath = computed(() => (path.value.length ? `/${path.value.join('/')}/` : '/'));

watchEffect(() => {
  onLoad(props.files);
});

onMounted(() => {
  setTimeout(onUpdate, 300);
});
</script>

<style lang="scss">
.files {
  &__route {
    user-select: none;
    height: 40px;
    border-top: 1px solid color('grey', 'lighten-1');
    border-bottom: 1px solid color('grey', 'lighten-1');
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
.list {
  &__inner {
    flex: 1 1;
    display: flex;
    align-items: center;
    height: 100%;
  }
  &__item {
    display: flex;
    align-items: center;
    height: 60px;
    &:not(:last-child) {
      border-bottom: 1px solid color('grey', 'lighten-1');
    }
    .name {
      font-weight: bold;
    }
  }
}
</style>
