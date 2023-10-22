<template>
  <div ref="root" class="v-expansion" :class="{ 'v-expansion--active': isActive }">
    <div class="v-expansion__header" @click="onActive">
      <div class="v-expansion__label text-title-1 fw-700">{{ label }}</div>
      <div class="v-expansion__icon grey-text">
        <v-icons icon="select"></v-icons>
      </div>
    </div>
    <div class="v-expansion__content">
      <div class="v-expansion__inner">
        <slot></slot>
      </div>
    </div>
  </div>
</template>
<script setup>
import { ref, defineProps, onMounted } from 'vue';
const props = defineProps({
  label: { type: String, default: '' },
  value: { type: Boolean, default: false },
});

const root = ref(null);
const isActive = ref(props.value);

const onChange = () => {
  const content = root.value.querySelector('.v-expansion__content');
  content.style.maxHeight = isActive.value ? `${content.scrollHeight}px` : '0';
  if (isActive.value) {
    setTimeout(() => {
      content.style.overflow = isActive.value ? `` : 'hidden';
    }, 500);
  } else {
    content.style.overflow = isActive.value ? `` : 'hidden';
  }
};

const onActive = () => {
  isActive.value = !isActive.value;
  onChange();
};

onMounted(() => {
  onChange();
});
</script>

<style lang="scss">
.v-expansion {
  border-bottom: 1px solid var(--border-1);
  &__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    min-height: 60px;
    padding: 0 15px;
    cursor: pointer;
  }
  &__content {
    max-height: 0;
    // overflow: hidden;
    transition: all 0.3s ease-out;
  }
  &__icon {
    transition: all 0.3s ease-out;
  }
  &__inner {
    padding: 15px;
  }
  &--active {
    border-bottom: none;
    .v-expansion__icon {
      transform: rotateX(180deg);
    }
  }
}
</style>
