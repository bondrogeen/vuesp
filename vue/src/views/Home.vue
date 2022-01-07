<template>
  <h1 class="greeting">{{$route.name}}</h1>
</template>

<script>
export default {
  mounted() {
    console.log(this.$route.name)
  }
}
</script>

<style>

</style>
