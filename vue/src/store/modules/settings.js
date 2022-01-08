export default {
  namespaced: true,
  state: () => ({
    name: 'Vuesp',
  }),
  mutations: {
    SET_DRAWER: (state, value) => {
      state.drawer = value;
    },
  },
  actions: {
    setDrawer: ({ commit }, value) => {
      commit('SET_DRAWER', value);
    },
  },
  getters: {
    getName: state => state.name
  },
};
