<template>
  <div>
    <h1 class="greeting">{{ $route.name }}</h1>
  </div>
</template>

<script>
export default {
  computed: {
  },
  mounted() {
    console.log(this.$route.name);
  },
};
</script>

<style>
</style>
