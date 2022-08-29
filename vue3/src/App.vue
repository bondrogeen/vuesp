<template>
  <nav>
    <router-link to="/">Home</router-link>
    <router-link to="/about" >About</router-link>
  </nav>
  <div @click="onClick">
  {{ isConnect }}

  </div>
  <router-view />
</template>

<script setup>
import { onMounted, onUnmounted } from 'vue';
const host = process.env.NODE_ENV === 'production' ? window.location.host : process.env.PROXY;
import { storeToRefs } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';

const webSocket = useWebSocket();
const { socket, isConnect } = storeToRefs(webSocket);
let ping = null 

const connect = () => {
  const instance = new WebSocket(`ws://${host}/ws`);
  instance.binaryType = 'arraybuffer';
  // socket.send("Меня зовут Джон");
  instance.onopen = webSocket.onopen;
  instance.onmessage = webSocket.onmessage;
  instance.onclose = e => { 
    webSocket.onclose(e);
    if(e.code !== 1000) connect();
  };
  instance.onerror = webSocket.onerror;
  socket.value = instance;
};

const onClick = () => {
  webSocket.onSend('SETTINGS')
}

onMounted(() => {
  ping = setInterval(webSocket.onPing, 1000);
  connect();
}); 

onUnmounted(() => {
  clearInterval(ping)
  socket.value.close(1000)
});
</script>

<style lang="scss"></style>
