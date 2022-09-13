<template>
	<div class="v-input text-body-1" @click="$emit('click', $event)">
		<input v-bind="$attrs" type="text" />
		<div class="v-input__icon" @click="$emit('click', $event)">
			<slot name="icon"></slot>
		</div>
	</div>
</template>

<script setup>
// import { computed, defineProps } from 'vue';
// const props = defineProps({ width: { type: Number, default: 0 } });
// const getStyle = computed(() => ({ width: props?.width ? `${props.width}px` : null }));
</script>

<style lang="scss">
.v-input {
	position: relative;
	height: 40px;
	width: 100%;
	border: 1px solid map-get($grey, 'base');
	border-radius: 4px;
	box-sizing: border-box;
	display: flex;
	align-items: center;
	input {
		border: 0;
		height: 100%;
		padding: 0 10px;
		font-size: $font-size-root;
		width: calc(100% - 20px);
		text-overflow: ellipsis;
		white-space: nowrap;
		overflow: hidden;
		flex: 1 1 auto;
		&::placeholder {
			color: map-get($grey, 'darken-3');
		}
		&:focus {
			outline: none !important;
		}
	}
	&:hover {
		@media (hover: hover) {
			border-color: map-get($grey, 'base');
		}
	}
	&__icon {
		height: 100%;
		flex: 0 0 40px;
		display: flex;
		align-items: center;
		justify-content: center;
	}
}
</style>
