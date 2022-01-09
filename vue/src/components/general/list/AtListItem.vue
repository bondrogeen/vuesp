<template>
  <li class="at-list-item" @click="$emit('click', $event)">
    <slot></slot>
  </li>
</template>

<style lang="scss">
.at-list-item {
  padding: 10px 15px;
  font-size: 1.2em;
  color: black;
  cursor: pointer;
  &:hover {
    background-color: #f4f7fc;
  }
}
</style>