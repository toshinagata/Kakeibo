<script setup lang="ts">
import { onMounted } from "vue"
import IconButton from "./IconButton.vue"

let printWindow: ()=>void;

onMounted(()=> {
  printWindow = () => {
    window.print();
  }
})
</script>

<template>
  <div id="print_button">
    <IconButton :label="'印刷'" style="top:10px;left:610px;width:58px;height:28px;"
      @click="printWindow()" />
  </div>
</template>

<style scoped>
@media print {
  #print_button { display: none; }
}
</style>
