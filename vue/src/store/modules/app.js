export default {
  namespaced: true,
  state: () => ({
    name: 'Vuesp',
    info: {},
    settings: {},
  }),
  mutations: {
    SET_DRAWER: (state, value) => {
      state.drawer = value;
    },
    SET_INFO: (state, info) => {
      state.info = { ...info };
    },
    SET_SETTINGS: (state, settings) => {
      state.settings = { ...settings };
    },
  },
  actions: {
    setDrawer: ({ commit }, value) => {
      commit('SET_DRAWER', value);
    },
  },
  getters: {
    getName: state => {
      return state.name;
    },
    getDrawer: state => {
      return state.drawer;
    },
    getInfo: state => {
      return state.info;
    },
    getSettings: state => {
      return state.settings;
    },
  },
};
