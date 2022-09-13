<template>
	<div class="v-dialog">
		<div class="v-dialog__card">
			<div class="v-dialog__header text-h6">
				<slot name="header">Attention !</slot>
			</div>
			<div class="v-dialog__body">
				<slot></slot>
			</div>
			<div class="v-dialog__footer">
				<slot name="footer">
					<b-button @click="$emit('close', false)">OK</b-button>
				</slot>
			</div>
		</div>
		<div class="v-dialog__overlay"></div>
	</div>
</template>

<style lang="scss">
.v-dialog {
	position: fixed;
	top: 0;
	left: 0;
	height: 100%;
	width: 100%;
	background-color: #5c575740;
	z-index: 5;
	&__card {
		position: absolute;
		top: 50%;
		left: 50%;
		transform: translate(-50%, -50%);
		min-height: 200px;
		min-width: 280px;
		border: 1px solid #dee2e6;
		border-radius: 4px;
		box-sizing: border-box;
		display: flex;
		flex-direction: column;
		background-color: white;
	}
	&__header {
		padding: 10px 15px;
		border-bottom: 1px solid #dee2e6;
	}
	&__body {
		flex: 1 1 auto;
		padding: 10px 15px;
	}
	&__footer {
		text-align: end;
		padding: 10px 15px;
		flex: 0 0 40px;
	}
}
</style>
