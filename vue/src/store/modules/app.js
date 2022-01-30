export default {
  namespaced: true,
  state: () => ({
    name: 'Vuesp',
    info: {},
    settings: {},
    ping: 0,
    progress: 0,
    connected: false,
    overlay: false,
    scanList: {},
    fileList: [],

  }),
  mutations: {
    SET_CONNECT: (state, value) => {
      state.connected = value;
    },
    SET_PING: (state) => {
      state.ping = Date.now();
    },
    SET_INFO: (state, info) => {
      state.info = info;
    },
    SET_SCAN: (state, { id, name, clean }) => {
      state.scanList = clean ? {} : { ...state.scanList, [id]: name };
    },
    SET_FILES: (state, { size, name, clean }) => {
      state.fileList = clean ? [] : [ ...state.fileList, { name, size }];
    },
    SET_SETTINGS: (state, value) => {
      state.settings = value;
    },
    SET_OVERLAY: (state, value) => {
      state.overlay = value;
    },
    SET_PROGRESS: (state, value) => {
      state.progress = value;
    },
  },
  actions: {
    setOverlay: ({ commit }, value) => {
      commit('SET_OVERLAY', value);
    },
    setConnect: ({ commit }, value) => {
      commit('SET_CONNECT', value);
    },
    clearScanList: ({ commit }) => {
      commit('SET_SCAN', { clean: true });
    },
    clearFileList: ({ commit }) => {
      commit('SET_FILES', { clean: true });
    },
  },
  getters: {
    getName: ({ name }) => name,
    getOverlay: ({ overlay }) => overlay,
    getInfo: ({ info }) => info,
    getSettings: ({ settings }) => settings,
    getPing: ({ ping }) => ping,
    isConnect: ({ connected }) => connected,
    getScanList: ({ scanList }) => scanList,
    getFileList: ({ fileList }) => fileList,
  },
};
