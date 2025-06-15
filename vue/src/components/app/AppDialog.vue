<template>
  <Teleport to="body">
    <transition
      enter-active-class="duration-300 ease-out"
      enter-from-class="transform opacity-0 -translate-y-2"
      enter-to-class="opacity-100 translate-y-0"
      leave-active-class="duration-200 ease-in"
      leave-from-class="opacity-100 translate-y-0"
      leave-to-class="transform opacity-0 -translate-y-2"
    >
      <div v-if="show" class="z-20 fixed top-0 h-[100dvh] w-full left-0 flex flex-col" @wheel.stop>
        <div class="absolute h-full w-full top-0 left-0 bg-black/60" @click="onClose"></div>

        <div class="flex-auto flex align-center p-4">
          <div class="m-auto w-full bg-white dark:bg-gray-900 shadow-lg flex flex-col rounded-lg z-30" :class="getClass">
            <div class="flex items-center justify-between px-4 py-2 border-b dark:border-gray-700 border-gray-200">
              <slot name="header">
                <h4>
                  {{ title }}
                </h4>
              </slot>

              <v-button type="icon" size="" @click="onClose">
                <icon-close class="h-5 w-5"></icon-close>
              </v-button>
            </div>

            <div class="px-4 py-2 flex-auto scroll-none">
              <slot>
                <div v-html="message"></div>
              </slot>
            </div>

            <div v-if="$slots.footer || callback" class="px-4 py-2 flex justify-end">
              <slot name="footer">
                <v-button color="blue" size="small" @click="onButton">{{ button }}</v-button>
              </slot>
            </div>
          </div>
        </div>
      </div>
    </transition>
  </Teleport>
</template>

<script setup lang="ts">
import { ref, defineProps, defineEmits, computed, onMounted } from 'vue';

interface Props {
  value?: boolean;
  title?: string;
  message?: string;
  callback?: () => void;
  button?: string;
  size?: string;
}

const { title = 'Attention !', message = '', callback, button = 'OK', size = 'sm' } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'close', value: Event): void;
}>();

const show = ref(false);

const sizes: { [index: string]: string } = { sm: 'max-w-[330px]', md: 'max-w-[600px]', lg: 'max-w-[960px]' };

const getClass = computed(() => [`${sizes?.[size] || ''}`]);

const onClose = (e: Event) => {
  show.value = false;
  setTimeout(() => {
    emit('close', e);
  }, 300);
};

const onButton = (e: Event) => {
  if (callback) callback();
  onClose(e);
};

onMounted(() => {
  setTimeout(() => {
    show.value = true;
  }, 300);
});
</script>
