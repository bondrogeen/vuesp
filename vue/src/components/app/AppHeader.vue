<template>
  <header class="bg-white dark:bg-gray-900 border-b border-gray-300 dark:border-gray-700 left-0 w-full z-10 sticky top-0">
    <div class="px-4 py-4 sm:px-6 lg:px-8">
      <div class="container mx-auto flex flex-auto items-center">
        <VButton type="icon" color="gray" class="me-6" @click="onSidebar">
          <IconBurger class="fill-current" />
        </VButton>

        <div class="lg:hidden flex-auto"></div>

        <div class="lg:hidden h-[30px] me-10">
          <router-link to="/">
            <IconLogo class="h-[30px]"></IconLogo>
          </router-link>
        </div>

        <div class="flex-auto"></div>

        <div class="flex gap-4">
          <VButton type="icon" color="gray" class="hidden md:flex" @click="changeTheme">
            <IconDark class="hidden dark:block" />
            <IconLight class="dark:hidden" />
          </VButton>

          <VButton
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
            <IconNoti />
          </VButton>

          <VButton type="icon" color="gray" class="hidden md:flex" @click.prevent="">
            <IconLogout />
          </VButton>
        </div>

        <div class="md:hidden flex gap-4">
          <VDropdown left="unset" right="0" top="calc(100%)">
            <template #activator="{ on }">
              <VButton type="icon" color="gray" class="flex" @click="on.click()">
                <IconDots />
              </VButton>
            </template>

            <VList v-slot="{ item }" class="py-2 rounded-lg" :list="listMenu" @click="onMenu">
              <IconLogout v-if="item.icon === 'logout'" class="h-4"></IconLogout>

              <IconDark v-if="item.icon === 'theme'" class="hidden dark:block" />
              <IconLight v-if="item.icon === 'theme'" class="dark:hidden" />

              <span class="ms-2">{{ item.name }}</span>
            </VList>
          </VDropdown>
        </div>
      </div>
    </div>
  </header>
</template>

<script setup lang="ts">
import { ref, computed, defineEmits, defineProps } from 'vue';

import VDropdown from '@/components/general/VDropdown.vue';
import VList from '@/components/general/VList.vue';
import VButton from '@/components/general/VButton.vue';

import IconLogo from '@/components/icons/IconLogo.vue';
import IconBurger from '@/components/icons/IconBurger.vue';
import IconNoti from '@/components/icons/IconNoti.vue';
import IconLogout from '@/components/icons/IconLogout.vue';
import IconDark from '@/components/icons/IconDark.vue';
import IconLight from '@/components/icons/IconLight.vue';
import IconDots from '@/components/icons/IconDots.vue';

interface Props {
  changeTheme?: () => void;
}

const { changeTheme } = defineProps<Props>();

const emit = defineEmits<{
  (e: 'sidebar', value: Event): void;
}>();

const dropdownOpen = ref(false);
const notifying = ref(false);

const listMenu = computed(() => [
  { name: 'Theme', icon: 'theme' },
  { name: 'Logout', icon: 'logout' },
]);

const onSidebar = (e: Event) => emit('sidebar', e);

const onLogout = async () =>
  await fetch('/', {
    method: 'get',
    headers: { Authorization: 'Basic AAAAAAAAAAAAAAAAAAA=' },
  });

const onMenu = ({ name }: any) => {
  if (name == 'Logout') onLogout();
  if (name == 'Theme' && changeTheme) changeTheme();
};
</script>
