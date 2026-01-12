<script setup lang="ts">
  const props = defineProps({
    enabled: { type: Boolean, default: true },
    selected: { type: Boolean, default: false },
    source: { type: String, default: "" },
    label: { type: String, default: "" },
  })
</script>

<template>
  <div :class="'button' + (selected ? ' selected' : '') + (enabled ? '' : ' dimmed')">
    <img v-if="source" :src="source" style="width:100%" />
    <span v-if="label">{{ label }}</span>
  </div>
</template>

<style scoped>
.button {
  position: absolute;
  width: 18px;
  height: 18px;
  border: 1px solid black;
  background-color: #cccccc;
  font-size: 14px;
  text-align: center;
  white-space: pre-line;
  display: grid;
  place-items: center;
  line-height: 16px;
}
.button.selected {
  background-color: #999999;
}
.button:hover {
  cursor: pointer;
  background-color: #888888;
}
.button.dimmed {
  border: 1px solid #888;
}
.button.dimmed:hover {
  cursor: default;
  background-color: #cccccc;
}
.button.selected.dimmed:hover {
  cursor: default;
  background-color: #999999;
}
.button.dimmed::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  background-color: rgba(255, 255, 255, 0.7);
  width: 100%;
  height: 100%;
}
</style>
