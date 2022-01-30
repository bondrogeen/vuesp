<template>
  <div class="v-header flex container">
    <div class="v-header__nav">
      <at-menu mode="horizontal" :activeName="getPath" @on-select="onRoute">
        <template v-for="{ title, path, icon, children } of menus">
          <at-menu-item v-if="!children" :key="title" :name="path">
            <i :class="`icon ${icon}`"></i>
            {{ title }}
          </at-menu-item>
          <at-submenu v-else :key="title">
            <template slot="title">
              <i :class="`icon ${icon}`"></i>
              {{ title }}
            </template>
            <template v-for="{ name, path: subPath } of children">
              <at-menu-item :key="name" :name="path + subPath">{{ name }}</at-menu-item>
            </template>
          </at-submenu>
        </template>
      </at-menu>
    </div>
  </div>
</template>

<script>
export default {
  computed: {
    routes() {
      return this.$router?.options?.routes || [];
    },
    menus() {
      return this.routes
        .filter(item => item?.meta?.title)
        .map(item => {
          return {
            title: item.meta.title,
            path: item.path,
            access: item.meta.access,
            icon: item.meta.icon,
            children: item.children,
          };
        });
    },
    getPath() {
      return this.$route.path;
    },
  },
  methods: {
    onRoute(value) {
      this.$router.push(value);
      console.log(value);
    },
  },
};
</script>
<style lang="scss">
.v-header {
  background-color: #fff;
  &__end {
    padding: 0 20px;
  }
}
</style>
