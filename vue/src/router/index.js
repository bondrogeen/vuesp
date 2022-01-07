import Vue from 'vue';
import VueRouter from 'vue-router';

const originalPush = VueRouter.prototype.push;
VueRouter.prototype.push = function push (location) {
  return originalPush.call(this, location).catch(err => err)
};
Vue.use(VueRouter);

const routes = [
  {
    path: '/',
    name: 'Vuesp',
    meta: {
      title: 'Vuesp',
      access: true,
      icon: 'icon-cast',
      text: ''
    },
    component: () => import('@/views/Home'),
    // redirect: '/datasets/choice'
  },
  {
    path: '/settings/',
    component: () => import('@/views/settings/index'),
    meta: {
      title: 'Settings',
      access: true,
      icon: 'icon-settings',
      text: ''
    },
    children: [
      {
        path: 'network',
        name: 'Network',
        component: () => import('@/views/settings/Network'),
        meta: {
          title: 'Network',
          access: true,
        },
      },
      {
        path: 'security',
        name: 'Security',
        component: () => import('@/views/settings/Security'),
        meta: {
          title: 'Security',
          access: true,
        },
      },
      {
        path: 'maintenance',
        name: 'Maintenance',
        component: () => import('@/views/settings/Maintenance'),
        meta: {
          title: 'Maintenance',
          access: true,
        },
      },
    ]
  },
  {
    path: "*",
    name: '404',
    meta: {
      access: true,
    },
    component: () => import('@/views/other/404')
  }
]


const router = new VueRouter({
  mode: 'history',
  base: process.env.BASE_URL,
  routes,
});

export default router;
