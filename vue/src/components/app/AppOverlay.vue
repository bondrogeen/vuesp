<template>
  <div class="app-overlay">
    <div class="app-overlay__inner">
      <div class="app-overlay__text">{{ text }}</div>
      <div class="lds-roller">
        <div v-for="item of 8" :key="item"></div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  props: {
    text: {
      type: String,
      default: 'Loading...',
    },
  },
};
</script>
