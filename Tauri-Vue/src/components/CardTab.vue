<script setup lang="ts">
import { ref, inject, computed, onMounted, onUnmounted, nextTick } from "vue"
import type { Ref } from "vue"
import type { DataEntry, DataType, DataMethods, Settings, SettingsMethods } from "../types.ts"
import IconButton from "./IconButton.vue"
import PrintButton from "./PrintButton.vue"
import { dataKey, settingsKey, monthsKey } from "./MainWindow.vue"
import { yearMonthToString, nextMonth, lastMonth, firstMonthInData, endMonthInData, formatAmount } from "../utils.ts"

/*  MainWindow のデータとメソッドにアクセス  */
const { data, methods } = inject(dataKey) as { data: Ref<DataType>; methods: DataMethods; };
const { settings, settingsMethods } = inject(settingsKey) as { settings: Ref<Settings>; settingsMethods: SettingsMethods; };
const { thisMonth, pageMonth, setPageMonth } = inject(monthsKey) as {
  thisMonth: Ref<number>; pageMonth: Ref<number>; setPageMonth: (ym: number) => void;
};

/*  明細を表示する月  */
const cardMonth = ref(pageMonth.value);

/*  カード名称  */
const cardName = ref(settings.value.cards[0]?.name || "");

/*  明細を表示可能な最初の月、最後の月  */
const firstCardMonth = computed(() => {
  let ym = firstMonthInData(data.value)
  if (ym === undefined) {
    ym = thisMonth.value;
  }
  return lastMonth(ym); /* １つ前の月まで表示可能 */
});
const endCardMonth = computed(() => {
  let ym = endMonthInData(data.value)
  if (ym === undefined || ym < thisMonth.value) {
    ym = thisMonth.value;
  }
  return ym;
});
const cardMonthList = computed(() => {
  let ym = firstCardMonth.value;
  let ym_end = endCardMonth.value;
  let ls:number[] = [];
  while (ym <= ym_end) {
    ls.push(ym);
    ym = nextMonth(ym);
  }
  return ls;
});

/*  「次の月」ボタンが有効かどうか  */
const nextMonthEnabled = computed(() => {
  return (cardMonth.value < endCardMonth.value);
});
/*  「前の月」ボタンが有効かどうか  */
const lastMonthEnabled = computed(() => {
  return (cardMonth.value > firstCardMonth.value);
});

function setCardMonth(ym: number) {
  cardMonth.value = ym;
  setPageMonth(ym);  /*  pageMonth も同時に更新する  */
}

const cardData = computed(() => {
  let d: DataEntry[] = [];
  let c = settings.value.cards.find((entry) => entry.name === cardName.value);
  if (cardName.value === "" || c === undefined) {
    return d;
  }
  let ym = cardMonth.value;
  for (let i = 0; i < 2; i++) {
    let page = data.value[ym];
    if (page !== undefined) {
      for (let entry of page) {
        if (entry.card === cardName.value) {
          let date = entry.date;
          if (date === undefined || date == 0 || entry.amount === undefined) {
            continue;
          }
          if ((i == 0 && date > c.closing) || (i == 1 && date <= c.closing)) {
            let e = {...entry};
            e.date = (ym % 100) * 100 + date;  /* 月の値も含める */
            d.push(e);
          }
        }
      }
    }
    ym = nextMonth(ym);
  }
  return d;
});

const cardSum = computed(() => {
  return cardData.value.reduce((acc, cur) => acc - (cur?.amount || 0), 0);
});

/*  data URL にエンコードする  */
import leftTriangleURL from "../../src/assets/left-triangle.svg?inline";
import rightTriangleURL from "../../src/assets/right-triangle.svg?inline";

onMounted(() => {
});
onUnmounted(() => {
});

</script>

<template>
  <div id="header_box">
    <select id="card_name" v-model="cardName" style="left:0px;width:120px">
      <option v-for="(entry, index) in settings.cards" :key="entry.name" :value="entry.name">
        {{ entry.name }}
      </option>
    </select>
    <IconButton :source="leftTriangleURL" style="top:3px;left:140px;"
      :enabled="lastMonthEnabled" @click="lastMonthEnabled && setCardMonth(lastMonth(cardMonth)) " />
    <select id="this_month" v-model="cardMonth" style="left:161px;width:140px;">
      <option v-for="(ym, index) in cardMonthList" :key="ym" :value="ym">
        {{ yearMonthToString(ym) }}
      </option>
    </select>
    <IconButton :source="rightTriangleURL" style="top:3px;left:302px;"
      :enabled="nextMonthEnabled" @click="nextMonthEnabled && setCardMonth(nextMonth(cardMonth))" />
    <div class="label" style="top:6px;left:340px;width:80px">
      今月の合計
    </div>
    <div class="sum_label" style="top:3px;left:420px;width:80px">
      {{ formatAmount(cardSum) }}
    </div>
    <div class="label" style="top:6px;left:524px;width:20px">
      円
    </div>
  </div>
  <PrintButton />
  <div id="table_box">
    <table>
      <thead>
        <tr>
          <th width="100">月日</th>
          <th width="260">内容</th>
          <th width="140">費目</th>
          <th width="140">支出</th>
        </tr>
      </thead>
      <tbody>
        <tr v-for="(entry, index) in cardData" :key="entry.date">
          <td>
            {{ (entry.date !== undefined ?
              `${Math.floor(entry.date / 100)}/${entry.date % 100}` : "") }}
          </td>
          <td>{{ entry.item }}</td>
          <td>{{ entry.kind }}</td>
          <td style="text-align: right; padding-right: 0.5em;">
            {{ (entry.amount !== undefined ? formatAmount(-entry.amount) : "") }}
          </td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<style scoped>

#this_month {
  position: absolute;
  height: 24px;
  font-size: 14px;
  border: 1px solid black;
  text-align: center;
  background-color: white;
}
#card_name {
  position: absolute;
  height: 24px;
  font-size: 14px;
  border: 1px solid black;
  text-align: center;
  background-color: white;
}
#menu_box {
  position: absolute;
  left: 10px;
  top: 7px;
}
#button_box {
  position: absolute;
  left: 10px;
  top: 50px;
}
#header_box {
  position: absolute;
  left: 35px;
  top: 15px;
}
.label {
  position: absolute;
  font-size: 14px;
}
.sum_label {
  position: absolute;
  font-size: 14px;
  background-color: white;
  border: solid 1px black;
  padding: 0 7px;
  text-align: right;
  height: 20px;
}
#table_box {
  position: absolute;
  top: 50px;
  left: 35px;
  width: 630px;
  height: 380px;
  overflow: auto;
}
table {
  border-collapse: collapse;
  border: solid 1px black;
  background-color: white;
}
thead {
  border: solid 1px black;
  background-color: #ccc;
}
td,th {
  font-size: 14px;
  padding: 2px;
  border: solid 1px black;
  text-align: center;
}

td.head {
  font-size: 14px;
  padding: 0 2px;
  border-right: solid 1px black;
  text-align: center;
}
td.body {
  height: 15px;
  padding: 0 10px;
  padding-top: 1px;
  border-right: solid 1px black;
  line-height: 1;
}
td.noBorderRight {
  border-right: none;
}
td.noPadding {
  padding: 0 0;
}
td.spannedColumn {
  padding: 0 2px;
  text-align: center;
}
td.amount {
  padding-right: 0.25em;
  text-align: right;
}
td.fatBorderLeft {
  border-left: 4px double #444;
}
td.kind {
  padding-right: 2px;
  padding-left: 2px;
  text-align: center;
}
@media print {
  #print_button { display: none; }
}
</style>
