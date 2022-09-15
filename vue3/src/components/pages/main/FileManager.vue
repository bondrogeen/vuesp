<template>
  <div class="files">
    <div class="d-flex align-center mb-6">
      <div class="text-h4 v-spacer">File system</div>
      <v-input-file :info="info" @result="onResult" @message="onMessage">
        <v-icons icon="plus" class="mr-4"></v-icons>
      </v-input-file>
      <v-dropdown>
        <template #activator="{ on }">
          <v-icons icon="menu" @click="on.click"></v-icons>
        </template>
        <v-list :list="mainMenu"></v-list>
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
        <li
          v-for="{ name, size, isDir, isFile } of sortFiles"
          :key="`file_${name}`"
          class="list__item"
          @click="onNext(isDir, name, $event)"
        >
          <div class="mr-4">
            <v-icons :icon="isDir ? `folder` : 'file'"></v-icons>
          </div>
          <div class="v-spacer">
            <div class="text-body-1 fw-600">{{ isDir ? `${name}` : name }}</div>
            <div v-if="isFile" class="text-body-2 grey-base">{{ size }} ({{ toByte(size) }})</div>
          </div>
          <div>
            <v-dropdown>
              <template #activator="{ on }">
                <v-icons icon="menu" @click="on.click"></v-icons>
              </template>
              <v-list :list="getListMenu(isDir)" @click="onDownload(name)"></v-list>
            </v-dropdown>
          </div>
        </li>
      </ul>
    </div>
  </div>
</template>

<script setup>
import { defineProps, watchEffect, defineEmits, ref, onMounted, computed } from 'vue';
import { toByte, debounce } from '@/utils/func/';

const emit = defineEmits(['send', 'clear', 'message']);

const props = defineProps({
  files: { type: Array, default: () => [] },
  info: { type: Object, default: () => ({}) },
  progress: { type: Object, default: () => ({}) },
});

const getListMenu = isDir =>
  [{ name: 'Download' }, { name: 'Remove' }].filter(i => (isDir ? i.name !== 'Download' : true));

const mainMenu = [{ name: 'Format' }];

const path = ref([]);
const filesTemp = ref([]);
const isLoading = ref(false);

const onUpdate = e => {
  isLoading.value = true;
  emit('clear', e);
  emit('send', { comm: 'FILES', data: { name: getFullPath.value } });
};

const onLoad = debounce(() => {
  filesTemp.value = props.files;
  isLoading.value = false;
}, 200);

const onPrev = index => {
  if (path.value.length > index) {
    path.value = path.value.filter((_, i) => i < index);
    onUpdate();
  }
};

const onNext = (isDir, value, e) => {
  if (isLoading.value || e?.target?.nodeName === 'svg') return;
  if (isDir && path) {
    path.value.push(value);
    onUpdate();
  }
};

const onDownload = name => {
  const url = path.value.length ? `/${path.value.join('/')}/${name}` : `/${name}`;
  const link = document.createElement('a');
  console.log(url);
  link.setAttribute('download', name);
  link.href = url;
  document.body.appendChild(link);
  link.click();
  link.remove();
};

const onResult = e => {
  if (e.ok) {
    emit('clear', e);
    emit('send', { comm: 'FILES', data: { name: '/' } });
    emit('send', { comm: 'INFO' });
  }
};

const sortFiles = computed(() => {
  const arr = JSON.parse(JSON.stringify(filesTemp.value));
  return arr.sort((a, b) => {
    return a.isFile > b.isFile ? 1 : -1;
  });
});

const onMessage = e => emit('message', e);
const getFullPath = computed(() => `/${path.value.join('/')}`);

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
