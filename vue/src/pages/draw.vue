<script setup lang="ts">
import { computed } from 'vue';

import { KEYS } from '@/utils/const';
import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

import MainPaint from '@/components/pages/main/MainPaint.vue';

const { $t } = useLocale();

const { main, onSend } = useConnection((send) => {
  send(KEYS.DRAW);
});

const draw = computed(() => main.value.draw);

const onSendDraw = (data: Record<string, unknown>) => {
  onSend(KEYS.DRAW, data);
};
</script>

<template>
  <div class="container mx-auto">
    <section class="mb-6 flex items-center justify-between">
      <h1>{{ $t('menu.draw') }}</h1>
    </section>

    <MainPaint :draw="draw" @send="onSendDraw" />
  </div>
</template>
