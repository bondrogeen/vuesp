<template>
	<v-dropdown class="v-select" v-bind="$attrs">
		<template #activator="{ on }">
			<v-input :value="currentUser" readonly :placeholder="placeholder" @click="on.click">
				<template #icon>
					<svg width="10" height="6" viewBox="0 0 10 6" fill="none" xmlns="http://www.w3.org/2000/svg">
						<path d="M1 1L5 5L9 1" stroke="#ABB5BE" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
					</svg>
				</template>
			</v-input>
		</template>
		<v-list v-slot="{ item }" :list="list" @click="$emit('change', $event)">
			<slot :item="item">{{ item.name }}</slot>
		</v-list>
	</v-dropdown>
</template>

<script setup>
import { computed, defineProps } from 'vue';
const props = defineProps({
	value: { type: String, default: '' },
	placeholder: { type: String, default: '' },
	list: { type: Array, default: () => [] },
});
const currentUser = computed(() => props.value);
const list = computed(() => props.list);
const placeholder = computed(() => props.placeholder);
</script>

<style lang="scss">
.v-select {
	position: relative;
	width: 280px;
}
</style>
