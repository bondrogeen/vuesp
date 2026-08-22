<script setup lang="ts">
import type { IListItem } from '@/types';

import { KEYS } from '@/utils/const';

import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

import { VButton, VLoader, VCheckbox, VDropdown, VExpansion, VList, VSelect, VTextField, VTable, VDialog, VTabs, VTooltip } from 'vuesp-components';
import { computed, ref } from 'vue';
import { dateUtcToString, timeUtcToString } from 'vuesp-components/helpers';

const { $t } = useLocale();

const { pkg, main, onSend } = useConnection((send) => {
  send(KEYS.DEVICE);
});

interface IItemTable {
  name: string;
  date: string;
  action: boolean;
}

const tabs = [{ title: 'Item 0' }, { title: 'Item 1' }, { title: 'Item 2' }];

const isFormatting = ref(false);

const dialogTable = ref(false);
const tabletItem = ref<IItemTable | null>(null);

const tabletHeaders = [
  { name: 'Name', key: 'name', className: 'w-50' },
  { name: 'Date', key: 'date', className: '' },
  { name: 'Action', key: 'action', className: 'w-20' },
];
const tabletItems: IItemTable[] = [
  { name: 'Item 1', date: `${dateUtcToString(new Date())} ${timeUtcToString(new Date())}`, action: false },
  { name: 'Item 2', date: `${dateUtcToString(new Date())} ${timeUtcToString(new Date())}`, action: true },
  { name: 'Item 3', date: `${dateUtcToString(new Date())} ${timeUtcToString(new Date())}`, action: true },
  { name: 'Item 4', date: `${dateUtcToString(new Date())} ${timeUtcToString(new Date())}`, action: false },
];

const onTable = (item: IItemTable) => {
  tabletItem.value = item;
  dialogTable.value = true;
};

const dialog = ref(false);
const input = ref('');
const checkbox = ref(true);
const loading = ref(false);
const onLoading = () => {
  loading.value = true;
  setTimeout(() => {
    loading.value = false;
  }, 1000);
};

const items: IListItem<string | number>[] = [
  { name: 'Item 1', value: 1 },
  { name: 'Item 2', value: 2 },
  { name: 'Item 3', value: '3' },
];

const select = ref<number | string>(2);
const getSelectValue = computed(() => items.find((i) => i.value == select.value)?.name || '');
const onSelect = ({ name, value }: IListItem<number | string>) => {
  select.value = value;
  input.value = name;
};

const listMenu: IListItem<number>[] = [{ name: $t('btnUpdate'), value: 2 }];

const onUpdate = () => {
  onSend('INFO');
};

const onMenu = ({ value }: IListItem<number>) => {
  if (value === 2) onUpdate();
};
</script>

<template>
  <div class="container mx-auto pb-10">
    <section class="mb-6 flex items-center justify-between">
      <h1>{{ $t('home') }}</h1>

      <v-select modelValue="2" :items="listMenu" @change="onMenu">
        <template #activator="{ on }">
          <v-button type="icon" @click="on.click">
            <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
          </v-button>
        </template>
      </v-select>
    </section>

    <div>
      <p class="text-lg font-bold mb-4 text-gray-400 max-w-200">
        {{ pkg.description }}
      </p>
    </div>

    <section class="flex flex-col gap-6">
      <div>
        <div class="flex items-center gap-6 mb-5">
          <h2 class="">Css</h2>
          <a href="https://tailwindcss.com/" target="_blank" class="text-lg text-blue-400 underline">https://tailwindcss.com/</a>
        </div>

        <div class="flex items-center justify-center rounded h-5 bg-green-500 md:bg-red-500 lg:bg-blue-500 xl:bg-gray-300 2xl:bg-yellow-500 uppercase py-4">
          <span class="text-white sm:hidden">sm</span>
          <span class="hidden sm:block md:hidden text-white">sm</span>
          <span class="hidden md:block lg:hidden text-white">md</span>
          <span class="hidden lg:block xl:hidden text-white">lg</span>
          <span class="hidden xl:block 2xl:hidden text-black">xl</span>
          <span class="hidden 2xl:block text-white">2xl</span>
        </div>
      </div>

      <div class="">
        <div class="flex items-center gap-6 mb-5">
          <h2>Icon</h2>
          <a href="https://remixicon.com/" target="_blank" class="text-lg text-blue-400 underline">https://remixicon.com/</a>
        </div>

        <div>
          <div class="flex items-center gap-3 mb-6">
            <icon-ri-home-line />
            <icon-ri-mail-line class="hover:scale-130 transition-all" />
            <icon-ri-printer-line class="hover:text-blue-500 transition-all" />
            <icon-ri-chat-1-line class="text-blue-500" />
            <icon-ri-settings-2-line class="size-4" />
            <v-button color="blue" href="https://remixicon.com/" target="_blank">https://remixicon.com/</v-button>

            <VTooltip>
              <template #activator>
                <icon-ri-information-line class="size-5 text-gray-500" />
              </template>
              <template #default="{ show }">
                <div v-if="show" class="absolute top-0 right-0 bg-white-main px-3 py-2 rounded shadow-nav z-5 w-70 bg-white dark:bg-gray-800">
                  <p>{{ $t('message.shortPress') }}</p>
                </div>
              </template>
            </VTooltip>
          </div>
        </div>
      </div>

      <div class="">
        <div>
          <div class="flex items-center gap-6 mb-5">
            <h2>Components</h2>
            <a href="https://github.com/bondrogeen/vuesp-components" target="_blank" class="text-lg text-blue-400 underline">vuesp-components</a>
          </div>

          <h3 class="mb-5">Button</h3>

          <div class="flex flex-wrap gap-3 mb-6">
            <v-button color="blue" size="sm" @click="dialog = true">Dialog</v-button>
            <v-button color="red" size="md" @click="dialog = true">Dialog</v-button>
            <v-button color="green" size="lg" :loading="loading" @click="onLoading">Loading</v-button>
            <v-button color="gray" size="xl" :loading="loading" @click="onLoading">Loading</v-button>
          </div>

          <div class="flex flex-wrap gap-3 mb-6">
            <v-button outline color="blue" size="sm" @click="dialog = true">Dialog</v-button>
            <v-button outline color="red" size="md" @click="dialog = true">Dialog</v-button>
            <v-button outline color="green" size="lg" :loading="loading" @click="onLoading">Loading</v-button>
            <v-button outline color="gray" size="xl" :loading="loading" @click="onLoading">Loading</v-button>
            <v-button outline color="transparent" class="text-white bg-yellow-400 dark:bg-yellow-800 h-8 rounded-2xl" :loading="loading" @click="onLoading">Custom color</v-button>
          </div>

          <div class="flex flex-wrap gap-3 mb-6">
            <v-button icon color="blue" size="sm" @click="dialog = true">
              <icon-ri-home-line />
            </v-button>
            <v-button outline color="red" size="md" @click="dialog = true">
              <icon-ri-mail-line class="hover:scale-130 transition-all" />
            </v-button>
            <v-button outline color="green" size="lg" loading @click="onLoading">
              <icon-ri-printer-line class="hover:text-blue-500 transition-all" />
            </v-button>
            <v-button outline color="gray" size="xl" :loading="loading" @click="onLoading">
              <icon-ri-home-line />
            </v-button>
          </div>
        </div>

        <div>
          <h3 class="mb-5">Form</h3>

          <div class="flex flex-col gap-3">
            <VTextField v-model="input" label="Text" />

            <VTextField label="Error" message="message">
              <template #icon>
                <v-button color="transparent" @click="dialog = true">
                  <icon-ri-home-line />
                </v-button>
              </template>
            </VTextField>

            <VSelect :model-value="getSelectValue" :items="items" label="Select" @change="onSelect" />

            <div class="flex gap-3 mt-5">
              <VCheckbox v-model="checkbox">Checkbox</VCheckbox>
              <VCheckbox @update:model-value="checkbox = !checkbox">!Checkbox</VCheckbox>
            </div>
          </div>
        </div>
      </div>

      <div class="">
        <h3 class="mb-5">Card</h3>

        <card-main :title="$t('Data')" :loading="loading">
          <template #header>
            <div class="col-span-full">
              <VCheckbox v-model="isFormatting">Formatting</VCheckbox>
            </div>
          </template>

          <div v-if="loading" class="absolute top-0 left-0 flex items-center justify-center h-full w-full bg-black/40 rounded">
            <VLoader />
          </div>

          <div class="flex flex-col gap-3">
            <div class="flex items-center gap-3">
              <h3 class="self-start">main:</h3>
              <component :is="isFormatting ? 'pre' : 'p'">{{ main }}</component>
            </div>
            <div class="flex items-center gap-3">
              <h3 class="self-start">main.device:</h3>
              <component :is="isFormatting ? 'pre' : 'p'">{{ main.device }}</component>
            </div>
            <div class="flex items-center gap-3">
              <h3 class="self-start">main.info:</h3>
              <component :is="isFormatting ? 'pre' : 'p'">{{ main.info }}</component>
            </div>
          </div>
        </card-main>
      </div>

      <div class="">
        <h3 class="mb-5">Tabs</h3>
        <VTabs class="flex-auto min-h-75" :value="0" :items="tabs">
          <template #tab-0>Item 0</template>

          <template #tab-1>Item 1</template>

          <template #tab-2>Item 2</template>
        </VTabs>
      </div>

      <div class="">
        <h3 class="mb-5">Table</h3>

        <VTable :headers="tabletHeaders" :items="tabletItems" @click="onTable">
          <template #name="{ item, i }">
            <span class="me-2"># {{ i + 1 }}</span>
            <span class="uppercase text-black dark:text-white">{{ item.name }}</span>
          </template>

          <template #action="{ item }">
            <div>
              <VDropdown :hide-on-click="false" right="0" left="unset" class="relative">
                <template #activator="{ on }">
                  <v-button v-if="item.action" color="transparent" size="sm" @click.stop="on.click">
                    <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
                  </v-button>
                </template>

                <div class="bg-white dark:bg-gray-900 rounded w-50 shadow-nav p-4">
                  <p class="">
                    {{ item }}
                  </p>
                </div>
              </VDropdown>
            </div>
          </template>
        </VTable>
      </div>

      <div class="">
        <h3 class="mb-5">Expansion</h3>

        <VExpansion label="Item 1" value>Lorem ipsum dolor sit amet consectetur adipisicing elit. Ut,</VExpansion>

        <VExpansion label="Item 1">Lorem ipsum dolor sit amet consectetur adipisicing elit.</VExpansion>

        <VExpansion label="Item 1">Lorem ipsum dolor sit amet consectetur adipisicing elit.</VExpansion>
      </div>
    </section>

    <v-dialog v-if="dialog" size="md" title="List" @close="dialog = false">
      <VList :items="tabletItems">
        <template #default="{ item }">{{ item.name }} ({{ item.date }})</template>
      </VList>
    </v-dialog>

    <v-dialog v-if="dialogTable" size="md" title="Dialog table" @close="dialogTable = false">
      <h4 class="mb-5">
        {{ tabletItem?.name }}
      </h4>
      <p>
        {{ tabletItem }}
      </p>
    </v-dialog>
  </div>
</template>
