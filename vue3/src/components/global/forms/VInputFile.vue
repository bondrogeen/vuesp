<template>
  <label class="v-input-file">
    <input type="file" v-bind="$attrs" @change="onChange" />
    <slot v-bind="selectFiles"></slot>
  </label>
</template>

<script setup>
import { ref, defineProps, defineEmits } from 'vue';
const props = defineProps({
  value: { type: String, default: '' },
  path: { type: String, default: '/' },
});

const emit = defineEmits(['change']);

const selectFiles = ref({ files: [], totalSize: 0 });

const onChange = async e => {
  const date = new FormData();
  const files = e.target.files;
  const info = { files: [] };
  info.totalSize = 0;

  for (let i = 0; i < files.length; i++) {
    const file = files.item(i);
    info.totalSize += file.size;
    info.files.push({ name: file.name, size: file.size });
    const fileName = `${props.path}${file.name}`;
    date.append(`file[${i}]`, file, fileName);
  }
  if (!files.length) return;
  selectFiles.value = info;
  emit('change', { date, info });
};
</script>

<style lang="scss">
.v-input-file {
  position: relative;
  display: flex;
  align-items: center;
  cursor: pointer;
  input[type='file'] {
    display: none;
  }
}
</style>
