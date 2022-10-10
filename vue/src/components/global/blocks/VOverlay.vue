<template>
	<div class="v-overlay">
		<div class="v-overlay__content">
			<slot></slot>
		</div>
	</div>
</template>

<style lang="scss">
.v-overlay {
	position: fixed;
	top: 0;
	left: 0;
	height: 100%;
	width: 100%;
	background-color: #5c575740;
	z-index: 5;
	&__content {
		position: absolute;
		top: 50%;
		left: 50%;
		transform: translate(-50%, -50%);
	}
}
</style>
