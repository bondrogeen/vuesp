<template>
  <header class="bg-white dark:bg-gray-900 border-b border-gray-300 dark:border-gray-700 left-0 w-full z-10 sticky top-0">
    <div class="px-4 py-4 sm:px-6 lg:px-8">
      <div class="container mx-auto flex flex-auto items-center">
        <v-button type="icon" color="gray" class="me-6" @click="onSidebar">
          <icon-burger class="fill-current" />
        </v-button>

        <div class="lg:hidden flex-auto"></div>

        <div class="lg:hidden h-[30px] me-10">
          <router-link to="/">
            <icon-logo class="h-[30px]"></icon-logo>
          </router-link>
        </div>

        <div class="flex-auto"></div>

        <div class="flex gap-4">
          <v-button type="icon" color="gray" class="hidden md:flex" @click="onChangeTheme">
            <IconTheme />
          </v-button>

          <v-button
            type="icon"
            color="gray"
            class="hidden md:flex"
            @click.prevent="
              dropdownOpen = !dropdownOpen;
              notifying = false;
            "
          >
            <span :class="!notifying ? 'hidden' : 'flex'" class="absolute right-0 top-0.5 z-1 h-2 w-2 rounded-full bg-orange-400 flex">
              <span class="absolute -z-1 inline-flex h-full w-full animate-ping rounded-full bg-orange-400 opacity-75"></span>
            </span>
            <icon-noti />
          </v-button>

          <v-button type="icon" color="gray" class="hidden md:flex" @click.prevent="onLogout">
            <icon-logout />
          </v-button>
        </div>

        <div class="md:hidden flex gap-4">
          <v-dropdown left="unset" right="0" top="0">
            <template #activator="{ on }">
              <v-button type="icon" color="gray" class="flex" @click="on.click()">
                <icon-dots />
              </v-button>
            </template>

            <v-list v-slot="{ item }" :list="listMenu" @click="onMenu">
              <component :is="item.icon"></component>
              <span class="ms-2">{{ item.name }}</span>
            </v-list>
          </v-dropdown>
        </div>
      </div>
    </div>
  </header>
</template>

<script setup lang="ts">
import { ref, computed, defineEmits, defineProps } from 'vue';

interface Props {
  changeTheme?: (value?: string) => void;
}

const { changeTheme } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'sidebar', value: Event): void;
}>();

const dropdownOpen = ref(false);
const notifying = ref(false);

const listMenu = computed(() => [
  { name: 'Theme', icon: 'icon-theme' },
  { name: 'Logout', icon: 'icon-logout' },
]);

const onSidebar = (e: Event) => emit('sidebar', e);

const onLogout = async () =>
  await fetch('/', {
    method: 'get',
    headers: { Authorization: 'Basic AAAAAAAAAAAAAAAAAAA=' },
  });

const onChangeTheme = () => {
  if (changeTheme) changeTheme();
};

const onMenu = ({ name }: any) => {
  if (name == 'Logout') onLogout();
  if (name == 'Theme') onChangeTheme();
};
</script>
