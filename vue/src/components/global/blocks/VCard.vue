<template>
  <div class="v-card">
    <div class="v-card__header flex justify-between">
      <slot name="header"></slot>
    </div>
    <div class="v-card__content">
      <slot></slot>
    </div>
  </div>
</template>

<style lang="scss">
.v-card {
  position: relative;
  box-shadow: 0 6px 12px var(--shadow-1);

  &__header {
    border-radius: 10px 10px 0 0;
    padding: 10px 20px;
    background: var(--bg-2);
  }
  &__content {
    background: color('app', 'white');

    // padding: 20px;
  }
}
</style>
