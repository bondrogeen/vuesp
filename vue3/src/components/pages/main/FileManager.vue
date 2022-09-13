<template>
  <div class="files">
    <div class="files__title">FS {{ getFullPath }}</div>
    <div class="files__list">
      <ul class="list">
        <li class="list__item">
          <span class="name">Name</span>
          <span class="size">Size</span>
        </li>
        <li v-if="path.length" class="list__item" @click="onPrev">
          <span class="name">../</span>
        </li>
        <li
          class="list__item"
          v-for="{ name, size, isDir, isFile } of sortFiles"
          :key="`file_${name}`"
          @click="onNext(isDir, '/' + name)"
        >
          <span class="">{{ isDir ? `/${name}` : name }}</span>
          <span v-if="isFile" class="size">{{ size }} ({{ size }})</span>
        </li>
        <li class="list__item list__item--total">
          <span class="name">Total</span>
          <span class="size">{{ total }} ({{ total }})</span>
        </li>
      </ul>
    </div>
  </div>
</template>

<script setup>
import { defineProps, defineEmits, ref, onMounted, computed } from 'vue';
const emit = defineEmits(['send']);

const props = defineProps({
  files: { type: Array, default: () => [] },
});

const path = ref([]);
// const history = ref({});

const onUpdate = () => {
  emit('send', { comm: 'FILES', data: { name: getFullPath.value } });
};

const onPrev = () => {
  path.value = path.value.filter((_, i) => i < path.value.length - 1);
  onUpdate();
};
const onNext = (isDir, value) => {
  if (isDir && path) {
    path.value.push(value);
    onUpdate();
  }
};

const sortFiles = computed(() => {
  const arr = JSON.parse(JSON.stringify(props.files));
  return arr.sort((a, b) => {
    return a.isFile > b.isFile ? 1 : -1;
  });
});

const total = computed(() => props.files.reduce((acc, el) => acc + el.size, 0));

const getFullPath = computed(() => `/${path.value.join('/')}`);

onMounted(() => {
  onUpdate();
});
</script>

<style lang="scss">
.files {
  &__title {
    margin: 0 0 20px 0;
  }
  &__list {
    margin: 0 0 20px 0;
  }
}
.list {
  &__item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    height: 40px;
    padding: 0 20px;
    &:hover {
      background-color: #ebf0fa;
    }
    .name {
      font-weight: bold;
    }
    &--total {
      border-top: 1px solid grey;
    }
  }
}
</style>
