<script setup lang="ts">
import { computed } from 'vue';
import { VButton, VDialog } from 'vuesp-components/components';

import { useLocale } from '@/composables/useLocale';
import type { IScriptSnippetsDialogEmits, IScriptSnippetsDialogProps } from './types';

const { $t } = useLocale();
const { manifest, suggestions } = defineProps<IScriptSnippetsDialogProps>();
const emit = defineEmits<IScriptSnippetsDialogEmits>();

const deviceSuggestions = computed(() => {
  const functions = new Set(manifest.functions.map((fn) => fn.name));
  const events = new Set(manifest.events.map((event) => event.name));
  return suggestions.filter((suggestion) => {
    if (suggestion.type === 'func') return !functions.has(suggestion.label);
    if (suggestion.type === 'event') return !events.has(suggestion.label);
    return suggestion.type === 'word' || suggestion.type === 'var';
  });
});
</script>

<template>
  <VDialog size="md" :title="$t('scriptStudio.suggestions')" @close="emit('close')">
    <div class="grid gap-5 md:grid-cols-2">
      <section>
        <h3 class="mb-2 text-xs uppercase tracking-wide text-gray-dark-30 dark:text-white/50">
          {{ $t('scriptStudio.functions') }}
        </h3>
        <div class="flex flex-wrap gap-1">
          <VButton
            v-for="fn in manifest.functions"
            :key="`fn-${fn.id}`"
            size="sm"
            variant="outline"
            color="blue"
            :title="`${fn.name}(${fn.min}..${fn.max}) => ${fn.ret}`"
            @click="emit('insert-function', fn)"
          >
            {{ fn.name }}
          </VButton>
        </div>
      </section>

      <section>
        <h3 class="mb-2 text-xs uppercase tracking-wide text-gray-dark-30 dark:text-white/50">
          {{ $t('scriptStudio.events') }}
        </h3>
        <div class="flex flex-wrap gap-1">
          <VButton
            v-for="event in manifest.events"
            :key="`ev-${event.id}`"
            size="sm"
            variant="outline"
            color="blue"
            :title="`${event.name}: ${event.params} ${$t('scriptStudio.params')}`"
            @click="emit('insert-event', event)"
          >
            {{ event.name }}
          </VButton>
        </div>
      </section>

      <section v-if="deviceSuggestions.length > 0" class="md:col-span-2">
        <h3 class="mb-2 text-xs uppercase tracking-wide text-gray-dark-30 dark:text-white/50">
          {{ $t('scriptStudio.deviceHints') }}
        </h3>
        <div class="flex flex-wrap gap-1">
          <VButton
            v-for="suggestion in deviceSuggestions"
            :key="`${suggestion.type}-${suggestion.label}`"
            size="sm"
            variant="outline"
            color="gray"
            :title="suggestion.detail || suggestion.description || suggestion.type"
            @click="emit('insert-suggestion', suggestion)"
          >
            {{ suggestion.label }}
          </VButton>
        </div>
      </section>
    </div>
  </VDialog>
</template>
