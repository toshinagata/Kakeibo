<script setup lang="ts">
import { onMounted } from "vue"
import MainWindow from './components/MainWindow.vue'
import { type } from '@tauri-apps/plugin-os';
import { getName, getVersion } from '@tauri-apps/api/app';
import { Menu, Submenu, PredefinedMenuItem } from '@tauri-apps/api/menu';
import { isTauriAvailable, isVueRunnerAvailable } from "./utils.ts";

/*  アプリケーションメニューを作成  */
/*  日本語メニューを作成 (MacOS only)  */
async function createAppMenu() {
  const appName = await getName();
  const appVersion = await getVersion();
  const about = await PredefinedMenuItem.new({
    text: appName + " について",
    item: { About: {
      name: appName,
      version: appVersion
    }}});
  const aboutMenu = await Submenu.new({
    text: appName,
    items: [
      about,
      await PredefinedMenuItem.new({text: "", item: "Separator"}),
      await PredefinedMenuItem.new({text: "サービス", item: "Services"}),
      await PredefinedMenuItem.new({text: "", item: "Separator"}),
      await PredefinedMenuItem.new({text: appName + " を隠す", item: "Hide"}),
      await PredefinedMenuItem.new({text: "他を隠す", item:"HideOthers"}),
      await PredefinedMenuItem.new({text: "すべて表示", item:"ShowAll"}),
      await PredefinedMenuItem.new({text: "", item: "Separator"}),
      await PredefinedMenuItem.new({text: appName + " を終了", item: "Quit"})
    ]
  });
  const appMenu = await Menu.new({
    items: [aboutMenu]
  });
  await appMenu.setAsAppMenu();
}

onMounted(async () => {
  if (isTauriAvailable()) {
    const osType = type();
    if (osType === "macos") {
      createAppMenu();
    }
  }
})
</script>

<template>
  <MainWindow />
</template>

<style scoped>
</style>

<style>
body {
  background-color: whitesmoke;
}
</style>