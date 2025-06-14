<template>
  <ul class="">
    <li v-for="(item, key) of items" :key="key" class="" :class="typeof item === 'object' ? '' : `flex items-center ${getClass()}`">
      <div class="flex items-center cursor-pointer ps-2 h-8" :class="typeof item === 'object' ? `${getClass()}` : ''" @click="onClick(key)">
        <div v-if="typeof item === 'object'" class="me-1">
          <icon-folder class="size-4" />
        </div>

        <div>{{ key }}</div>

        <template v-if="typeof item === 'object'">
          <div class="flex-auto"></div>

          <div class="flex items-center z-10">
            <button class="px-2 cursor-pointer" @click.stop="onGetKey(key)">
              <icon-save class="size-5" />
            </button>
          </div>
        </template>
      </div>

      <div v-if="typeof item === 'object'" class="flex ps-5">
        <VListObject v-show="show[key]" class="flex-auto" :path="getPath(key)" :items="item" @click="emit('click', $event)"></VListObject>
      </div>

      <template v-else>
        <div class="flex-auto"></div>

        <div class="text-blue-400">
          {{ item }}
        </div>

        <button class="px-2 cursor-pointer z-10" @click.stop="onGetKey(key)">
          <icon-save class="size-5" />
        </button>
      </template>
    </li>
  </ul>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import type { Ref } from 'vue';

interface Props {
  items: object;
  path?: string;
}

const { items = {}, path = '' } = defineProps<Props>();

import VListObject from '@/components/general/VListObject.vue';

const emit = defineEmits<{
  (e: 'click', value: string): void;
}>();

const getClass = () => `before:absolute before:w-full before:h-8 before:t-0 before:left-0 before:rounded-sm hover:before:bg-gray-400/50`;

const show: Ref<{ [index: string]: boolean }> = ref({});

const onClick = (key: string) => {
  show.value[key] = !show.value[key];
};

const getPath = (key: string) => `${path ? path + '.' : ''}${key}`;

const onGetKey = (key: string) => {
  emit('click', getPath(key));
};
</script>
