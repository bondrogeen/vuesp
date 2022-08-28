<template>
  <div>
    <h1 class="greeting">{{ $route.name }}</h1>
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
            <span v-if="isFile" class="size">{{ size | toByte }} ({{ size }})</span>
          </li>
          <li class="list__item list__item--total">
            <span class="name">Total</span>
            <span class="size">{{ total | toByte }} ({{ total }})</span>
          </li>
        </ul>
      </div>
    </div>
  </div>
</template>

<script>
import { mapActions, mapGetters } from 'vuex';
export default {
  data: () => ({
    path: [],
    history: {},
  }),
  computed: {
    ...mapGetters({
      files: 'app/getFileList',
    }),
    sortFiles() {
      const arr = JSON.parse(JSON.stringify(this.files));
      return arr.sort((a, b) => {
        return a.isFile > b.isFile ? 1 : -1;
      });
    },
    total() {
      return this.files.reduce((acc, el) => acc + el.size, 0);
    },
    getFullPath() {
      return `/${this.path.join('/')}`;
    },
  },
  mounted() {
    this.onUpdate();
  },
  methods: {
    ...mapActions({
      onSend: 'socket/onSend',
      clearFileList: 'app/clearFileList',
    }),
    onPrev() {
      this.path = this.path.filter((_, i) => i < this.path.length - 1);
      this.onUpdate();
    },
    onNext(isDir, path) {
      if (isDir && path) {
        this.path.push(path);
        this.onUpdate();
      }
    },
    onUpdate() {
      this.clearFileList();
      this.onSend({ comm: 'FILES', data: { name: this.getFullPath } });
    },
  },
};
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
