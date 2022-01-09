export default {
  namespaced: true,
  state: () => ({
    name: 'Vuesp',
    info: {},
    settings: {},
    ping: 0,
    connected: false,
    overlay: false
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
    SET_SETTINGS: (state, value) => {
      state.settings = value;
    },
    SET_OVERLAY: (state, value) => {
      state.overlay = value;
    },
  },
  actions: {
    setOverlay: ({ commit }, value) => {
      commit('SET_OVERLAY', value);
    },
    setConnect: ({ commit }, value) => {
      commit('SET_CONNECT', value);
    },
  },
  getters: {
    getName: ({ name }) => name,
    getOverlay: ({ overlay }) => overlay,
    getInfo: ({ info }) => info,
    getSettings: ({ settings }) => settings,
    getPing: ({ ping }) => ping,
    isConnect: ({ connected }) => connected,
  },
};
