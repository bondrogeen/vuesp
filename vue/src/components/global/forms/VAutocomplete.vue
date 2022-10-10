<template>
  <v-dropdown class="v-autocomplete" v-bind="$attrs">
    <template #activator="{ show, on }">
      <v-input
        :value="show ? search : currentUser"
        :placeholder="show ? 'Поиск' : placeholder"
        @click="on.click"
        @input="onInput"
        @focus="search = ''"
      >
        <template #icon>
          <svg width="10" height="6" viewBox="0 0 10 6" fill="none" xmlns="http://www.w3.org/2000/svg">
            <path d="M1 1L5 5L9 1" stroke="#ABB5BE" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
          </svg>
        </template>
      </v-input>
    </template>
    <div>
      <v-list v-slot="{ item }" :list="getList" @click="$emit('change', $event)">
        <slot :item="item">{{ item[valueName] }}</slot>
      </v-list>
    </div>
    <div v-if="isEmpty" class="v-autocomplete__empty">Not found</div>
  </v-dropdown>
</template>

<script setup>
import { ref, computed, defineProps, defineEmits } from 'vue';
const props = defineProps({
  value: { type: String, default: '' },
  valueName: { type: String, default: 'name' },
  placeholder: { type: String, default: '' },
  list: { type: Array, default: () => [] },
});

defineEmits(['change']);
const search = ref('');

const currentUser = computed(() => props.value);
const getList = computed(() => {
  const list = props?.list || [];
  return list.filter(i =>
    search.value ? i[props.valueName].toLowerCase().includes(search.value.toLowerCase()) : true
  );
});
const placeholder = computed(() => props.placeholder);
const isEmpty = computed(() => Boolean(!getList?.value?.length));

const onInput = e => search.value = e.target.value;

</script>

<style lang="scss">
.v-autocomplete {
  position: relative;
  // width: 280px;
  &__empty {
    padding: 5px 15px;
    // text-align: center;
  }
}
</style>
