import { createRouter, createWebHistory } from 'vue-router';
import HomeView from '../views/HomeView.vue';

const routes = [
  {
    path: '/',
    name: 'home',
    component: HomeView,
  },
  {
    path: '/main',
    name: 'Main',
    component: () => import('../views/MainView.vue'),
  },
  {
    path: '/service',
    name: 'Service',
    component: () => import('../views/ServiceView.vue'),
  },
];

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});

export default router;
