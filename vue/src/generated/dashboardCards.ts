// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/config/generate.mjs

import { CardButton, CardDate, CardDimmer, CardInfo, CardInput, CardList } from 'vuesp-components/components';
import CardChart from '@/components/dashboard/CardChart.vue';

export const DASHBOARD_CARDS = {
  button: CardButton,
  date: CardDate,
  dimmer: CardDimmer,
  info: CardInfo,
  input: CardInput,
  list: CardList,
  chart: CardChart,
} as const;

export const DASHBOARD_CARD_TYPES = ["button","date","dimmer","info","input","list","chart"] as const;
