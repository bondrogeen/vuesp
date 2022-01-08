export default {
  namespaced: true,
  state: () => ({
    name: 'Vuesp',
    info: {},
    settings: {},
    ping: {},
  }),
  mutations: {
    SET_PING: (state) => {
      state.ping = Date.now();
    },
    SET_INFO: (state, info) => {
      state.info = info;
    },
    SET_SETTINGS: (state, value) => {
      state.settings = value;
    },
  },
  actions: {
    setDrawer: ({ commit }, value) => {
      commit('SET_DRAWER', value);
    },
    setCommand: ({ commit }, value) => {
      commit('SET_DRAWER', value);
    },
  },
  getters: {
    getName: state => state.name,
    getDrawer: state => state.drawer,
    getInfo: state => state.info,
    getSettings: state => state.settings
  },
};
