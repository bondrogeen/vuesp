<template>
  <label class="v-input-file">
    <input type="file" multiple @change="onChange" />
    <slot></slot>
  </label>
</template>

<script setup>
import { computed, defineProps, defineEmits } from 'vue';
const emit = defineEmits(['result', 'message']);
const props = defineProps({
  value: { type: String, default: '' },
  path: { type: String, default: '/upload' },
  info: { type: Object, default: () => ({}) },
});

const availableByte = computed(() => props.info.totalBytes - props.info.usedBytes);

const onUpload = async formData => await fetch(props.path, { method: 'POST', body: formData });

const onChange = e => {
  const formData = new FormData();
  const files = e.target.files;
  let totalSize = 0;
  for (let i = 0; i < files.length; i++) {
    const file = files.item(i);
    totalSize += file.size;
    formData.append(`file[${i}]`, file);
  }
  if (!files.length) return;
  if (totalSize < availableByte.value) {
    const res = onUpload(formData);
    setTimeout(() => emit('result', res), 1000);
  } else {
    emit('message', { value: true, message: 'No free space on filesystem' });
  }
};
</script>

<style lang="scss">
.v-input-file {
  position: relative;
  input[type='file'] {
    display: none;
  }
}
</style>
