<template>
	<div v-outside="outside" class="v-dropdown" :class="{ 'v-dropdown--opened': isShown }">
		<div class="v-dropdown__activator">
			<slot name="activator" :on="on" :value="isShown"></slot>
		</div>
		<transition name="slide-up">
			<div v-if="isShown" class="v-dropdown__content" :style="getStyle" @click="click">
				<slot :show="show" :hide="hide" :is-shown="isShown"></slot>
			</div>
		</transition>
	</div>
</template>

<script>
export default {
	props: {
		top: {
			type: String,
			default: 'calc(100% + 5px)',
		},
		left: {
			type: String,
			default: '',
		},
		right: {
			type: String,
			default: '0',
		},
		hideOnClick: {
			type: Boolean,
			default: true,
		},
	},
	data() {
		return {
			on: {
				click: this.click,
			},
			isShown: false,
		};
	},
	computed: {
		getStyle() {
			return {
				top: this.top,
				left: this.left,
				right: this.right,
			};
		},
	},
	methods: {
		outside() {
			if (this.isShown) this.hide();
		},
		click() {
			if (!this.isShown) {
				this.show();
			} else if (this.hideOnClick) {
				this.hide();
			}
			this.$emit('click', this.isShown);
		},
		show() {
			this.isShown = true;
			this.$emit('show');
		},
		hide() {
			this.isShown = false;
			this.$emit('hide');
		},
	},
};
</script>

<style lang="scss">
.v-dropdown {
	position: relative;
	&__activator {
		display: flex;
		align-items: center;
		cursor: pointer;
	}
	&__content {
		padding: 5px 0;
		position: absolute;
		overflow: auto;
		// left: 0;
		top: calc(100% + 5px);
		box-shadow: 0 6px 12px rgb(0 0 0 / 18%);
		border: none;
		// height: 0;
		border-radius: 0;
		min-width: 100%;
		z-index: 2;
		max-height: 200px;
		transition: 0.3s cubic-bezier(0.25, 0.8, 0.5, 1);
		background-color: var(--var-bg-1);
	}
	&--opened {
		.v-dropdown__content {
			// height: 200px;
		}
		.v-dropdown__activator::before {
			transform: rotate(180deg);
		}
	}
}
</style>
