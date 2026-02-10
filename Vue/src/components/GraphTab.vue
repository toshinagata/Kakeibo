<script setup lang="ts">
import { ref, inject, computed, onMounted, onUnmounted, nextTick } from "vue"
import type { Ref } from "vue"
import type { DataEntry, DataType, DataMethods, CardEntry, Settings, SettingsMethods } from "../types.ts"
import { dataKey, settingsKey, monthsKey } from "./MainWindow.vue"
import { formatAmount } from "../utils.ts"
import PrintButton from "./PrintButton.vue"

/*  MainWindow のデータとメソッドにアクセス  */
const { data, methods } = inject(dataKey) as { data: Ref<DataType>; methods: DataMethods; };
const { settings, settingsMethods } = inject(settingsKey) as { settings: Ref<Settings>; settingsMethods: SettingsMethods; };
const { thisMonth, pageMonth, setPageMonth } = inject(monthsKey) as {
  thisMonth: Ref<number>; pageMonth: Ref<number>; setPageMonth: (ym: number) => void;
};

/*  canvas の幅・高さ  */
const cWidth = 630;
const cHeight = 400;

/*  devicePixelRatio を保持するレファレンス  */
const devicePixelRatio = ref(1.0);

/*  canvas を保持するレファレンス  */
const canvas = ref<HTMLCanvasElement>();

/*  svgを包含するdivのレファレンス  */
const svgdiv = ref<HTMLElement>();

/*  表示する支出費目：-1 なら合計  */
const totalLabel = "支出合計";
const kindValue = ref(totalLabel);
const kindMenu = ref<HTMLSelectElement>();

/*  表示する年度（この年度とひとつ前の年度を同時に表示する）  */
const yearValue = ref(0);
const yearMenu = ref<HTMLSelectElement>();

/*  年度の一覧  */
const yearList = computed(() => {
  let months = Object.keys(data.value).sort();
  let firstYear = calcFiscalYear(Number(months[0]) || thisMonth.value); 
  let lastYear = calcFiscalYear(Number(months[months.length - 1]) || thisMonth.value);
  let years = new Array(lastYear - firstYear + 1);
  for (let i = 0; i < years.length; i++) {
    years[i] = firstYear + i;
  }
  return years;
});

/*  表示するデータ  */
/*  undefined または number の配列  */
const displayData = computed(() => {
  return Array.from({ length: 24 }, (_value, index) => {
    /*  0..24 が前年度の4月〜今年度の3月に対応。その月のデータがなければ undefined  */
    let kind = kindValue.value;
    let month = (yearValue.value - 1 + Math.floor((index + 3) / 12)) * 100
      + (index + 3) % 12 + 1;
    let monthData = data.value[month];
    if (monthData === undefined) {
      return 0;
    } else {
      return monthData.reduce((acc, cur) => {
        if (cur !== undefined && cur.amount !== undefined &&
          ((kind === totalLabel && !cur.isIncome) || cur.kind === kind)) {
          return acc - cur.amount;
        } else {
          return acc;
        }
      }, 0);
    }
  });
});

/*  年月から年度を計算  */
function calcFiscalYear(yearMonth: number): number {
  let m = yearMonth % 100;
  let y = Math.floor(yearMonth / 100);
  return (m <= 3 ? y - 1 : y);
}

//  軸の目盛りの位置を計算
//  base: 軸の始点
//  minorHint: 小さな目盛り間隔のヒント（この値以下の最大値を選ぶ）
//  majorHint: 大きな目盛り間隔のヒント（この値以下であること）
//  return [tick, tbase, minor]
//    tick = 大きな目盛りの間隔
//    tbase = 最初の大きな目盛り（画面外かも）
//    minor = 小さな目盛りは大きな目盛りの何等分か
function calcTick(base: number, minorHint: number, majorHint: number): [number, number, number] {
  let minorTick: number;
  let majorTick: number;
  let ex = Math.pow(10, Math.floor(Math.log10(minorHint)));
  let r = minorHint / ex;
  if (r >= 5) {
    minorTick = 5 * ex;
    if (minorTick * 4 <= majorHint) {
      majorTick = minorTick * 4;
    } else if (minorTick * 2 <= majorHint) {
      majorTick = minorTick * 2;
    } else {
      majorTick = minorTick;
    }
  } else if (r >= 2.5) {
    minorTick = 2.5 * ex;
    if (minorTick * 4 <= majorHint) {
      majorTick = minorTick * 4;
    } else if (minorTick * 2 <= majorHint) {
      majorTick = minorTick * 2;
    } else {
      majorTick = minorTick;
    }
  } else if (r >= 2) {
    minorTick = 2 * ex;
    if (minorTick * 5 <= majorHint) {
      majorTick = minorTick * 5;
    } else {
      majorTick = minorTick;
    }
  } else {
    minorTick = ex;
    if (minorTick * 5 <= majorHint) {
      majorTick = minorTick * 5;
    } else if (minorTick * 2 <= majorHint) {
      majorTick = minorTick * 2;
    } else {
      majorTick = minorTick;
    }
  }
  let tbase = Math.floor(base / majorTick) * majorTick;
  let minor = Math.floor(majorTick / minorTick + 0.5);
  return [majorTick, tbase, minor];
}

/*  canvas にグラフを描画  */
function drawGraph() {
  const left = 80;
  const right = 20;
  const top = 35;
  const bottom = 20;
  let ctx = canvas.value?.getContext("2d");
  if (ctx !== undefined && ctx !== null) {
    ctx.font = '10pt sans-serif';
    ctx.strokeStyle = "black";
    ctx.fillStyle = "black";
    ctx.clearRect(0, 0, cWidth, cHeight);
    /*  軸を描く  */
    ctx.beginPath();
    ctx.moveTo(left, top);
    ctx.lineTo(left, cHeight - bottom);
    ctx.lineTo(cWidth - right, cHeight - bottom);
    ctx.stroke();
    /*  X軸の目盛り  */
    for (let m = 0; m < 12; m++) {
      let x = Math.floor(left + (cWidth - left - right) / 12 * (m + 0.5));
      ctx.beginPath();
      ctx.moveTo(x, cHeight - bottom);
      ctx.lineTo(x, cHeight - bottom - 4);
      ctx.stroke();
      let label = String((m + 3) % 12 + 1) + "月";
      ctx.fillText(label, x - Math.floor(ctx.measureText(label).width / 2), cHeight - bottom + 16);
    }
    /*  Y軸の目盛り  */
    /*  データの最大値（100より小さければ100）  */
    let ymax = displayData.value.reduce((acc, cur) => (acc < cur ? cur : acc), 100);
    let [tick, tbase, minor] = calcTick(0, ymax / 10, ymax / 2);
    let ybase = cHeight - bottom;
    let yheight = cHeight - top - bottom - 4;
    for (let i = 0; i * tick / minor <= ymax; i++) {
      let y = Math.floor(ybase - yheight * (i * tick / minor) / ymax);
      let dx = (i % minor == 0 ? 6 : 4);
      ctx.beginPath();
      ctx.moveTo(left, y);
      ctx.lineTo(left + dx, y);
      ctx.stroke();
      if (i % minor == 0) {
        let label = formatAmount(Math.floor(i * tick / minor + 0.5));
        ctx.fillText(label, left - 10 - ctx.measureText(label).width, y + 4);
      }
    }
    let unit = "(円)";
    ctx.fillText(unit, left - 10 - ctx.measureText(unit).width, top - 11);
    /*  グラフ  */
    for (let yr = 0; yr < 2; yr++) {
      const lineDash = (yr == 0 ? [4, 2] : []);
      let last_y = undefined;
      let last_x = undefined;
      let color = (yr == 0 ? "#cc0000" : "blue");
      ctx.strokeStyle = color;
      ctx.fillStyle = color;
      let sum = 0;
      for (let m = 0; m < 12; m++) {
        let y = undefined;
        let x = Math.floor(left + (cWidth - left - right) / 12 * (m + 0.5));
        let val = displayData.value[yr * 12 + m];
        y = Math.floor(ybase - yheight * val / ymax);
        if (yr == 0) {
          ctx.setLineDash(lineDash);
        }
        if (last_x !== undefined && last_y !== undefined) {
          ctx.beginPath();
          ctx.moveTo(last_x, last_y);
          ctx.lineTo(x, y);
          ctx.stroke();
        }
        ctx.setLineDash([]);
        sum += val;
        ctx.beginPath();
        ctx.arc(x, y, 4, 0, Math.PI * 2);
        ctx.closePath();
        if (yr == 0) {
          ctx.stroke();
        } else {
          ctx.fill();
        }
        last_x = x;
        last_y = y;
      }
      sum = Math.floor(sum / 12 + 0.5);
      /*  凡例  */
      let x1 = left + 10 + (1 - yr) * 300;
      let y1 = top - 11;
      if (yr == 0) {
        ctx.setLineDash(lineDash);
      }
      ctx.beginPath();
      ctx.moveTo(x1 + 5, y1 - 8);
      ctx.lineTo(x1 + 45, y1 - 8);
      ctx.stroke();
      ctx.setLineDash([]);
      ctx.beginPath();
      ctx.arc(x1 + 25, y1 - 8, 4, 0, Math.PI * 2);
      ctx.closePath();
      if (yr == 0) {
        ctx.stroke();
      } else {
        ctx.fill();
      }
      ctx.fillText(`${yearValue.value + yr - 1}年度`, x1 + 50, y1 - 4);
      ctx.fillText("月平均 " + formatAmount(sum) + " 円", x1 + 120, y1 - 4);
    }
  }
}

function onWindowResize() {
  let dpr = window.devicePixelRatio || 1.0;
  if (dpr < 0.5) {
    dpr = 0.5;
  } else if (dpr > 4) {
    dpr = 4;
  }
  devicePixelRatio.value = dpr;
  if (canvas.value !== undefined) {
    canvas.value.width = cWidth * dpr;
    canvas.value.height = cHeight * dpr;
    let ctx = canvas.value.getContext("2d");
    if (ctx !== null && ctx !== undefined) {
      ctx.scale(dpr, dpr);
    }
    canvas.value.style.width = String(cWidth) + "px";
    canvas.value.style.height = String(cHeight) + "px";
  }
}

/*  支出費目が変更された  */
function onChangeKind() {
  kindValue.value = kindMenu.value?.value || totalLabel;
  drawGraph();
}

/*  年度が変更された  */
function onChangeYear() {
  let pageYear = calcFiscalYear(pageMonth.value);
  yearValue.value = Number(yearMenu.value?.value) || pageYear;
  if (yearValue.value != pageYear) {
    setPageMonth(yearValue.value * 100 + 4);
  }
  drawGraph();
}
onMounted(() => {
  onWindowResize();
  window.addEventListener('resize', onWindowResize);
  yearValue.value = calcFiscalYear(pageMonth.value);
  drawGraph();
});

onUnmounted(() => {
  window.removeEventListener('resize', onWindowResize);
});

</script>

<template>
  <div id="header">
    <span class="label">支出内訳別グラフ</span>
    <select ref="kindMenu" :value="kindValue"
      @change="onChangeKind()">
      <option key="-1" :value="totalLabel">{{ totalLabel }}</option>
      <hr />
      <option v-for="(kind,index) in settings.paymentKinds"
        :key="index" :value="kind">
        {{ kind }}
      </option>
    </select>
    <select ref="yearMenu" :value="yearValue"
      @change="onChangeYear()">
      <option v-for="(year,index) in yearList"
        :key="index" :value="year">
        {{ `${year}年度` }}
      </option>
    </select>
  </div>
  <PrintButton />
  <canvas ref="canvas" :width="cWidth" :height="cHeight"></canvas>

</template>

<style scoped>
#header {
  position: absolute;
  top: 20px;
  left: 35px;
}
.label {
  font-size: 14px;
  margin-right: 14px;
}
select {
  font-size: 14px;
  margin-right: 14px;
  border: 1px solid black;
  text-align: center;
  background-color: white;
}
canvas {
  position: absolute;
  top: 50px;
  left: 35px;
  border: 1px solid black;
  background-color: white;
}

#svgdiv {
  position: absolute;
  top: 50px;
  left: 90px;
  border: 1px solid black;
  background-color: white;
}

</style>
