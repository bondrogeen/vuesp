<template>
  <at-modal :value="value" title="Network">
    <at-list>
      <at-list-item v-for="(item, key) in list" :key="key" @click="$emit('click', item)">{{ item }}</at-list-item>
    </at-list>
    <div v-if="!isList" class="at-empty">Loading...</div>
    <template #footer>
      <at-button type="primary" @click.native="$emit('scan', $event)">Scan</at-button>
      <at-button @click.native="noCancel">Close</at-button>
    </template>
  </at-modal>
</template>

<script>
import { mapGetters } from 'vuex';
import AtList from '@/components/general/list/AtList';
import AtListItem from '@/components/general/list/AtListItem';
export default {
  components: {
    AtListItem,
    AtList,
  },
  props: {
    value: {
      type: Boolean,
      default: false,
    },
  },
  computed: {
    ...mapGetters({
      list: 'app/getScanList',
    }),
    isList() {
      return Boolean(Object.keys(this.list).length);
    },
  },
  methods: {
    onConfirm(e) {
      this.$emit('scan', e);
    },
    noCancel() {
      this.$emit('input', false);
    },
  },
};
</script>

<style lang="scss">
.at-empty {
  font-size: 1.2em;
  display: flex;
  justify-content: center;
  padding: 10px 0;
}
</style>