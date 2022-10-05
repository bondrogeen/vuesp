<template>
  <div class="v-card">
    <div class="v-card__content">
      <slot></slot>
    </div>
  </div>
</template>

<style lang="scss">
.v-card {
  position: relative;
  background: var(--bg-2);
  border-radius: 10px;
  &__content {
    padding: 20px;
  }
}
</style>
