import { createRouter, createWebHistory } from 'vue-router';
import routes from 'vue-auto-routing';

// const routes = [{ path: '*', component: PageNotFound }];


const router = createRouter({
  linkActiveClass: 'text-primary',
  linkExactActiveClass: 'text-primary',
  history: createWebHistory(process.env.BASE_URL),
  routes,
  scrollBehavior() {
    //to, from, savedPosition
    return { top: 0, behavior: 'smooth' };
  },
});

export default router;
