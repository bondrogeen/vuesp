<template>
  <label class="v-input-file">
    <input type="file" @change="onChange" />
    <slot></slot>
  </label>
</template>

<script setup>
import { ref, defineProps, defineEmits } from 'vue';
const emit = defineEmits(['result']);
const props = defineProps({
  value: { type: String, default: '' },
  path: { type: String, default: '/upload' },
  progress: { type: Object, default: () => ({}) },
});

const fileUpload = ref();

const onUpload = async () => {
  const formData = new FormData();
  formData.append('update', fileUpload.value);
  const res = await fetch(props.path, {
    method: 'POST',
    body: formData,
  });
  setTimeout(() => emit('result', res), 500);
};

const onChange = e => {
  const file = e?.target?.files?.[0];
  if (file) {
    fileUpload.value = e.target.files[0];
    onUpload();
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
