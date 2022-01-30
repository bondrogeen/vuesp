<template>
  <div>
    <h1 class="greeting">{{ $route.name }}</h1>
    <div class="files">
      <div class="files__title">FS</div>
      <div class="files__list">
        <ul class="list">
          <li class="list__item" v-for="{ name, size } of files" :key="`file_${name}`">
            <span class="name">{{ name }}</span>
            <span class="size">{{ size | toByte }} ({{ size }})</span>
          </li>
          <li class="list__item">
            <span class="name">Total</span>
            <span class="size">{{ total | toByte }} ({{ total }})</span>
          </li>
        </ul>
      </div>
      <div class="files__btn col-24 flex flex-end">
        <at-button type="primary" @click="onTest">Update FS</at-button>
      </div>
    </div>
  </div>
</template>

<script>
import { mapActions, mapGetters } from 'vuex';
export default {
  data: () => ({}),
  computed: {
    ...mapGetters({
      files: 'app/getFileList',
    }),
    total() {
      return this.files.reduce((acc, el) => (acc + el.size), 0)
    }
  },
  mounted() {
    console.log(this.$route.name);
  },
  methods: {
    ...mapActions({
      onSend: 'socket/onSend',
      clearFileList: 'app/clearFileList',
    }),
    onTest() {
      console.log('onTest');
      this.clearFileList();
      this.onSend({ comm: 'FILES' });
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
  }
}
</style>
