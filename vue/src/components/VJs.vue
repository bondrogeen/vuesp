<template>
  <div class="minimal-highlighter">
    <h3>JavaScript код</h3>
    <pre class="code"><code>{{ code }}</code></pre>
    <button @click="copy" class="btn">Копировать</button>
    <p v-if="copied" class="message">Код скопирован!</p>
  </div>
</template>

<script>
import { ref } from 'vue'

export default {
  setup() {
    const code = ref(`function hello() {
  console.log("Hello, Vue 3!");
  return "Готово!";
}`)
    
    const copied = ref(false)
    
    const copy = () => {
      navigator.clipboard.writeText(code.value)
      copied.value = true
      setTimeout(() => copied.value = false, 2000)
    }
    
    return { code, copied, copy }
  }
}
</script>

<style>
.minimal-highlighter {
  font-family: Arial, sans-serif;
  max-width: 600px;
  margin: 20px auto;
  padding: 20px;
}

.code {
  background: #2d2d2d;
  color: #f8f8f2;
  padding: 20px;
  border-radius: 8px;
  overflow-x: auto;
  font-family: 'Courier New', monospace;
  font-size: 14px;
  line-height: 1.5;
}

.btn {
  background: #007acc;
  color: white;
  border: none;
  padding: 10px 20px;
  border-radius: 4px;
  cursor: pointer;
  margin-top: 10px;
}

.message {
  color: green;
  margin-top: 10px;
}
</style>