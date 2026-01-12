<script setup lang="ts">
import { ref, inject, computed, onMounted, onUnmounted, nextTick } from "vue"
import type { Ref } from "vue"
import type { DataEntry, DataType, DataMethods, Settings, SettingsMethods } from "../types.ts"
import { dataKey, settingsKey, monthsKey } from "./MainWindow.vue"
import IconButton from "./IconButton.vue"
import PrintButton from "./PrintButton.vue"
import { yearMonthToString, formatAmount } from "../utils.ts"

/*  MainWindow のデータとメソッドにアクセス  */
const { data, methods } = inject(dataKey) as { data: Ref<DataType>; methods: DataMethods; };
const { settings, settingsMethods } = inject(settingsKey) as { settings: Ref<Settings>; settingsMethods: SettingsMethods; };
const { thisMonth, pageMonth, setPageMonth } = inject(monthsKey) as {
  thisMonth: Ref<number>; pageMonth: Ref<number>; setPageMonth: (ym: number) => void;
};

/*  テーブル表示開始月（4月または10月）  */
const dispMonth = ref(calcDispMonth(pageMonth.value));

/*  ある月を表示するための開始月  */
function calcDispMonth(ym: number) {
  let m = ym % 100;
  if (m >= 4 && m <= 9) {
    return ym - m + 4;
  } else if (m <= 3) {
    return ym - m - 90;
  } else {
    return ym - m + 10;
  }
}

/*  ある月を含む年度  */
function fiscalYear(ym: number) {
  let m = ym % 100;
  if (ym < 4) {
    return Math.floor(ym / 100 - 1);
  } else {
    return Math.floor(ym / 100);
  }
}

/*  表示開始月になれる最初の月、最後の月  */
function firstOrEndDispMonth(isFirst: boolean) {
  let keys = Object.keys(data.value);
  let month = thisMonth.value;
  if (keys.length > 0) {
    if (isFirst) {
      /*  データを持っている最初の月  */
      month = Number(keys.reduce((a, b) => (a < b ? a : b)));
    } else {
      /*  データを持っている最後の月  */
      month = Number(keys.reduce((a, b) => (a > b ? a : b)));
    }
  }
  let m = month % 100;
  if (m >= 4 && m <= 9) {
    return month - m + 4;   /*  この年の4月  */
  } else if (m < 4) {
    return month - m - 90;  /*  前の年の10月  */
  } else {
    return month - m + 10;  /*  この年の10月  */
  }
}
const firstDispMonth = computed(() => firstOrEndDispMonth(true));
const endDispMonth = computed(() => firstOrEndDispMonth(false));
const dispMonthList = computed(() => {
  let ym = firstDispMonth.value;
  let ym_end = endDispMonth.value;
  let ls:number[] = [];
  while (ym <= ym_end) {
    ls.push(ym);
    ym = nextDispMonth(ym);
  }
  return ls;
});

/*  表示開始月を月範囲文字列に変換  */
function rangeStringFromDispMonth(ym: number) {
  let ym2 = offsetMonth(ym, 5);
  return `${yearMonthToString(ym)}〜${yearMonthToString(ym2)}`;
}

/*  次の表示開始月  */
function nextDispMonth(ym: number) {
  let m = ym % 100;
  if (m >= 4 && m <= 9) {
    return ym - m + 10;
  } else if (m >= 10) {
    return ym - m + 104;
  } else {
    return ym - m + 4;
  }
}
/*  前の表示開始月  */
function lastDispMonth(ym: number) {
  let m = ym % 100;
  if (m >= 5 && m <= 10) {
    return ym - m + 4;
  } else if (m <= 4) {
    return ym - m - 90;
  } else {
    return ym - m + 10;
  }
}

/*  「次の月」ボタンが有効かどうか  */
const nextMonthEnabled = computed(() => {
  return (dispMonth.value < endDispMonth.value);
});
/*  「前の月」ボタンが有効かどうか  */
const lastMonthEnabled = computed(() => {
  return (dispMonth.value > firstDispMonth.value);
});

/*  開始月+offset の月  */
function offsetMonth(ym: number, offset: number) {
  let m = ym % 100;
  let m2 = m + offset;
  if (m2 > 0) {
    let m3 = (m2 - 1) % 12 + 1;
    return (ym - m) + (m2 - m3) / 12 * 100 + m3;
  } else {
    let m3 = 12 + (m2 % 12); /* m2は[-11..0], m3は[1..12]の範囲になる */
    return (ym - m) + (m2 - m3) / 12 * 100 + m3;
  }
}

/*  年月から「年/月」の文字列に変換  */
function yearMonthToShortString(ym: number) {
  let m = ym % 100;
  return `${(ym - m) / 100}/${m}`
}

function setDispMonth(ym: number) {
  dispMonth.value = ym;
  setPageMonth(ym);  /*  pageMonth も同時に更新する  */
}

const dispData = computed(() => {
  let d: { [ym: number]: { [kind: string]: number }} = {};
  /*  表示は半年分だが集計は1年分行う（年度合計を求めるため） */
  let startMonth = fiscalYear(dispMonth.value) * 100 + 4;
  for (let i = 0; i < 12; i++) {
    let ym = offsetMonth(startMonth, i);
    let page = data.value[ym];
    let r: {[kind: string]: number} = { "収入合計": 0, "支出合計": 0 };
    if (page !== undefined) {
      for (let entry of page) {
        if (r[entry.kind] === undefined) {
          r[entry.kind] = 0;
        }
        let amount = (entry.amount || 0) * (entry.isIncome ? 1 : -1);
        r[entry.kind] += amount;
        if (entry.isIncome) {
          r["収入合計"] += amount;
        } else {
          r["支出合計"] += amount;
        }
      }
    }
    r["収支"] = r["収入合計"] - r["支出合計"];
    d[ym] = r;
  }
  /*  年度ごとの合計：4ケタの年をキーとして値を格納  */
  let rs: {[kind: string]: number} = {};
  for (let ym in d) {
    for (let kind in d[ym]) {
      if (rs[kind] === undefined) {
        rs[kind] = 0;
      } else {
        rs[kind] += d[ym][kind];
      }
    }
  }
  d[Math.floor(startMonth / 100)] = rs;
  return d;
});

/*  data URL にエンコードする  */
import leftTriangleURL from "../../src/assets/left-triangle.svg?inline";
import rightTriangleURL from "../../src/assets/right-triangle.svg?inline";

onMounted(() => {
  /*  テーブルの行数（タイトル行を除く）  */
  let rows = 3 + settings.value.incomeKinds.length + settings.value.paymentKinds.length;
  let rowHeight = Math.floor((380 / rows - 3) * 10) / 10;
  let fontSize = Math.floor((rowHeight - 2) * 2 + 0.5) / 2;
  loop: {
    for (let sheet of document.styleSheets) {
      for (let rule of sheet.cssRules) {
        console.log(rule);
        if (rule instanceof CSSStyleRule && rule.selectorText.slice(0, 7) === "td.body") {
          rule.style.setProperty("height", rowHeight + "px");
          rule.style.setProperty("font-size", fontSize + "px");
          break loop;
        }
      }
    }
  }
});
onUnmounted(() => {
});

</script>

<template>
  <div id="header_box">
    <IconButton :source="leftTriangleURL" style="top:3px;left:140px;"
      :enabled="lastMonthEnabled" @click="lastMonthEnabled && setDispMonth(lastDispMonth(dispMonth)) " />
    <select id="this_month" v-model="dispMonth" style="left:161px;width:240px;">
      <option v-for="(ym, index) in dispMonthList" :key="ym" :value="ym">
        {{ rangeStringFromDispMonth(ym) }}
      </option>
    </select>
    <IconButton :source="rightTriangleURL" style="top:3px;left:402px;"
      :enabled="nextMonthEnabled" @click="nextMonthEnabled && setDispMonth(nextDispMonth(dispMonth))" />
  </div>
  <PrintButton />
  <div id="table_box">
    <table>
      <thead>
        <tr>
          <td class="head noBorderRight noPadding" style="width:20px"></td>
          <td class="head" style="width:79px">費目</td>
          <td :class="'head'+(i==1?' fatBorderLeft':'')" v-for="i in 6" :key="i" style="width:67px">
            {{ yearMonthToShortString(offsetMonth(dispMonth, i - 1)) }}
          </td>
          <td class="head fatBorderLeft" style="width:79px">
            {{ fiscalYear(dispMonth) + "年度" }}
          </td>
        </tr>
      </thead>
      <tbody>
        <tr v-for="(kind, index) in settings.incomeKinds" :key="kind">
          <td class="body spannedColumn" v-if="index==0" :rowspan="settings.incomeKinds.length+1">
            収<br/><br/>入
          </td>
          <td class="body kind" :id="`income${index}`">
            {{ kind }}
          </td>
          <td :class="'body amount'+(i==1?' fatBorderLeft':'')" v-for="i in 6" :key="i">
            {{ formatAmount(dispData[offsetMonth(dispMonth, i - 1)]?.[kind] || 0) }}
          </td>
          <td class="body amount fatBorderLeft">
            {{ formatAmount(dispData[fiscalYear(dispMonth)]?.[kind] || 0) }}
          </td>
        </tr>
        <tr id="incomeTotalRow">
          <td class="body kind" id="incomeTotal">合計</td>
          <td :class="'body amount'+(i==1?' fatBorderLeft':'')" v-for="i in 6" :key="i">
            {{ formatAmount(dispData[offsetMonth(dispMonth, i - 1)]?.["収入合計"] || 0) }}
          </td>
          <td class="body amount fatBorderLeft">
            {{ formatAmount(dispData[fiscalYear(dispMonth)]?.["収入合計"] || 0) }}
          </td>
        </tr>
        <tr v-for="(kind, index) in settings.paymentKinds" :key="kind">
          <td class="body spannedColumn" v-if="index==0" :rowspan="settings.paymentKinds.length+1">
            支<br/><br/>出
          </td>
          <td class="body kind" :id="`payment${index}`">
            {{ kind }}
          </td>
          <td :class="'body amount'+(i==1?' fatBorderLeft':'')" v-for="i in 6" :key="i">
            {{ formatAmount(dispData[offsetMonth(dispMonth, i - 1)]?.[kind] || 0) }}
          </td>
          <td class="body amount fatBorderLeft">
            {{ formatAmount(dispData[fiscalYear(dispMonth)]?.[kind] || 0) }}
          </td>
        </tr>
        <tr id="paymentTotalRow">
          <td class="body kind" id="paymentTotal">合計</td>
          <td :class="'body amount'+(i==1?' fatBorderLeft':'')" v-for="i in 6" :key="i">
            {{ formatAmount(dispData[offsetMonth(dispMonth, i - 1)]?.["支出合計"] || 0) }}
          </td>
          <td class="body amount fatBorderLeft">
            {{ formatAmount(dispData[fiscalYear(dispMonth)]?.["支出合計"] || 0) }}
          </td>
        </tr>
        <tr id="balanceRow">
          <td class="body noBorderRight noPadding"></td>
          <td class="body kind" id="balance">収支</td>
          <td :class="'body amount'+(i==1?' fatBorderLeft':'')" v-for="i in 6" :key="i">
            {{ formatAmount(dispData[offsetMonth(dispMonth, i - 1)]?.["収支"] || 0) }}
          </td>
          <td class="body amount fatBorderLeft">
            {{ formatAmount(dispData[fiscalYear(dispMonth)]?.["収支"] || 0) }}
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
  left: 90px;
  top: 15px;
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
}
#incomeTotalRow, #paymentTotalRow {
  border: solid 1px black;
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
</style>
