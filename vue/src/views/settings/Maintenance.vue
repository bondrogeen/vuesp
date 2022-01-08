<template>
  <div class="row v-settings">
    <div class="col-24">
      <h1>{{ $route.name }}</h1>
    </div>
    <div class="col-24 v-settings__item">
      <div class="label">Reboot</div>
      <at-button class="mr-2" type="primary" :loading="loading.reboot" @click="modalReboot">Reboot</at-button>
      <span>Reboot the device.</span>
    </div>
    <div class="col-24 v-settings__item">
      <div class="label">Default</div>
      <at-button class="mr-2" type="primary" @click="onReset">Restor</at-button>
      <span>Reset all the parameters, to the default settings.</span>
    </div>
    <div class="col-24">
      <div class="row v-settings__item">
        <div class="col-24 col-md-12 col-lg-8">
          <div class="label">Upgrade</div>
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
      <div class="row v-settings__item">
        <div class="col-24 col-md-12 col-lg-8">
          <div class="label">Upload</div>
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
