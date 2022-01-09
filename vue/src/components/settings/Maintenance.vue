<template>
  <div class="row v-settings__item">
    <div class="col-24">
      <h3>Maintenance</h3>
    </div>
    <div class="col-24">
      <p class="label">Reboot</p>
      <div class="flex flex-middle">
        <at-button class="mr-2" type="primary" :loading="loading.reboot" @click="modalReboot">Reboot</at-button>
        <p>Reboot the device.</p>
      </div>
    </div>
    <div class="col-24">
      <p class="label">Default</p>
      <div class="flex flex-middle">
        <at-button class="mr-2" type="primary" @click="onReset">Restor</at-button>
        <p>Reset all the parameters, to the default settings.</p>
      </div>
    </div>
    <div class="col-24">
      <div class="row">
        <div class="col-24 col-md-12 col-lg-8">
          <p class="label">Upgrade</p>
          <at-input :value="fileFirmware.name" placeholder="Firmware" append-button @focus="onUploadFirmware">
            <template slot="append">
              <span>Upgrade</span>
            </template>
          </at-input>
          <input ref="upgrade" type="file" class="d-none" @change="onChange" />
        </div>
      </div>
    </div>
    <div class="col-24">
      <div class="row">
        <div class="col-24 col-md-12 col-lg-8">
          <p class="label">Upload</p>
          <at-input placeholder="Files" append-button>
            <template slot="append">
              <span>Upload</span>
            </template>
          </at-input>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  data: () => ({
    showPass: false,
    fileFirmware: {},
    loading: {
      reboot: false,
    },
  }),
  computed: {},
  methods: {
    async onReboot() {
      this.loading.reboot = true;
      await fetch('/reboot');
      this.loading.reboot = false;
    },
    onReset() {
      this.$store.dispatch('socket/sendStruct', { comm: 'SETTINGS' });
    },
    onChange({ target }) {
      this.fileFirmware = target.files[0];
    },
    onUploadFirmware(e) {
      // e.preventdefault()
      console.log(e);
      e.target.blur();
      this.$refs.upgrade.click();
    },
    async modalReboot() {
      try {
        const get = await this.$Modal.confirm({
          title: 'Reboot',
          content: 'Do you want to reboot the unit?',
        });
        console.log(get);
        if (get) {
          this.onReboot();
        }
      } catch (error) {
        console.log(error);
      }
    },
  },
};
</script>

<style lang="scss">
.v-settings {
  &__item {
    margin-bottom: 20px;
    .label {
      margin: 10px 0;
      user-select: none;
    }
  }
}
</style>
