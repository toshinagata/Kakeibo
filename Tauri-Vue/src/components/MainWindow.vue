<script lang="ts">
  /*  Injection key の定義, export (script setup の外で行う)  */
  import { InjectionKey, Ref } from "vue";
  export const dataKey: InjectionKey<{
    data: Ref<DataType>;
    methods: DataMethods;
  }> = Symbol("data");
  export const settingsKey: InjectionKey<{
    settings: Ref<Settings>;
    settingsMethods: SettingsMethods;
  }> = Symbol("settings");
  export const monthsKey: InjectionKey<{
    thisMonth: Ref<number>;
    pageMonth: Ref<number>;
    setPageMonth: (ym: number) => void;
  }> = Symbol("months")
</script>

<script setup lang="ts">
import { ref, provide, nextTick, onMounted, onUnmounted } from "vue"
import type { DataEntry, CardEntry, DataType, DataMethods, Settings, SettingsMethods } from "../types.ts"
import { isVueRunnerAvailable, myAlertAsync, myConfirmAsync, myAskAsync, yearMonthToString, endMonthInData } from "../utils.ts"
import MainTab from "./MainTab.vue"
import SettingsTab from "./SettingsTab.vue"
import CardTab from "./CardTab.vue"
import TableTab from "./TableTab.vue"
import GraphTab from "./GraphTab.vue"
import IconButton from "./IconButton.vue"
import { vHomeDir, vJoin, vMkdir, vExists, vCreate, vRename, vRemove, vReadTextFile, vWriteTextFile, vReadDir, vSaveDialog, vTerminate, vListenToServer }
  from "../vueRunner.ts";

/*  テスト用データ（デバッグ用）  */
import { testData } from "../testData.ts";

/*  ブック名 (default は「わたし」と表示する)  */
const bookName = ref("default");

/*  「わたし」以外のブック名  */
const bookNames = ref<string[]>([]);

/*  ブック名の選択ボックス  */
const selectBook = ref<HTMLSelectElement>();

/*  data, settings が更新されたかどうか  */
let dataAreModified = false;
let settingsAreModified = false;

/*  dataは連想配列、数値 YYYYMM をキーとする値が DataEntry の配列（その月の入出金データ） */
const data = ref<DataType>({});

/*  データ変更のためのメソッド  */
const methods: DataMethods = {
  setValue(page: number, row: number, key: keyof DataEntry,
    value: string | number | boolean | undefined): void {
      let r = data.value?.[page]?.[row];
      if (r !== undefined) {
        if (key == "date" || key == "amount") {
          r[key] = value as number | undefined;
        } else if (key == "item" || key == "kind" || key == "card") {
          r[key] = value as string;
        } else if (key == "isIncome") {
          r[key] = value as boolean;
        }
        dataAreModified = true;
      }
  },
  insertRow(page: number, row: number,
    entry: DataEntry | undefined): void {
      let p = data.value?.[page];
      if (p !== undefined) {
        if (!entry) {
          entry = { date: undefined, item: "", kind: "", isIncome: false, amount: undefined, card: ""};
        }
        p.splice(row, 0, entry);
        dataAreModified = true;
      }
  },
  deleteRow(page: number, row: number): void {
    let p = data.value?.[page];
    if (p !== undefined) {
      p.splice(row, 1);
      dataAreModified = true;
    }
  },
  insertPage(page: number): void {
    data.value[page] = [];
    dataAreModified = true;
  },
  deletePage(page: number): void {
    delete data.value[page];
    dataAreModified = true;
  },
  importCSV: async (file: File) => {
    let stage = 0;
    try {
      console.log("import csv from " + file);
      const content = await file.text();
      stage = 1;
      const result = readDataFromString(content);
      stage = 2;
      if (result === undefined || result.data === undefined || Object.keys(result.data).length == 0) {
        throw new Error("No data");
      }
      const months = Object.keys(result.data).sort();
      const firstMonth = yearMonthToString(months[0]);
      const lastMonth = yearMonthToString(months[months.length - 1]);
      const rows = countRows(result.data);
      const mes = file.name + "のデータを読み込みました(" + firstMonth + "-" + lastMonth + ", " + String(rows) + "行)\n"
        + "現在のデータを置き換えてもよいですか？";
      const ans = await myConfirmAsync(mes)
      if (ans === "ok") {
        data.value = result.data;
        settings.value.incomeKinds = result.income;
        settings.value.paymentKinds = result.payment;
        settings.value.cards = result.cards;
        /* データのある一番最近の月をpageMonthに設定 */
        let ym = endMonthInData(data.value);
        if (ym !== undefined) {
          setPageMonth(ym);
        }
      }
    } catch (error: any) {
      let s = "";
      if (stage == 0) {
        s = file.name + "が読み込めませんでした。";
      } else if (stage == 1) {
        s = file.name + "の内容に問題があるようです。";
      } else {
        s = file.name + "の中に入出金データがありませんでした。";
      }
      await myAlertAsync(s);
    }
  },
  exportCSV: async () => {
    const csv = writeDataToString();
    let action = "";
    try {
      if (await isVueRunnerAvailable() && Number(document.location.port) < 8000) {
        //  vueRunner が 3000 番台のポートを使用：WebView で動作している
        action = "書き出し";
        const path = await vSaveDialog({ defaultPath: 'kakeibo.csv' });
        if (path) {
          await vWriteTextFile(path, csv);
        } else {
          await myAlertAsync("ファイルの" + action + "をキャンセルしました。");
        }
      } else {
        action = "ダウンロード";
        const blob = new Blob([csv], { type: 'text/csv'});
        const url = URL.createObjectURL(blob);
        const a = Object.assign(document.createElement('a'), {
          href: url,
          download: 'kakeibo.csv'
        });
        a.click();
        URL.revokeObjectURL(url);
      }
    } catch (error: any) {
      await myAlertAsync("ファイルの" + action + "に失敗しました。");
    }
  }
};

/*  費目とカード設定  */
const defaultSettings = `{
  "incomeKinds": [
    "給与", "賞与", "年金", "雑収入", "その他収入"
  ],
  "paymentKinds": [
    "食費", "住居", "光熱水道", "被服", "保険医療",
    "教育", "教養娯楽", "交際", "交通通信", "貯蓄",
    "保険", "税金", "その他支出"
  ],
  "cards": [
  ]
}`;

const settings = ref<Settings>(JSON.parse(defaultSettings));

/*  設定変更のためのメソッド  */
const settingsMethods: SettingsMethods = {
  insertIncomeKind(kind: string, index: number): void {
    settings.value.incomeKinds.splice(index, 0, kind);
    settingsAreModified = true;
  },
  deleteIncomeKind(index: number): void {
    settings.value.incomeKinds.splice(index, 1);
    settingsAreModified = true;
  },
  replaceIncomeKind(kind: string, index: number): void {
    if (index >= 0 && index < settings.value.incomeKinds.length) {
      settings.value.incomeKinds[index] = kind;
      settingsAreModified = true;
    }
  },
  moveIncomeKind(fromIndex: number, toIndex: number): void {
    let kinds = settings.value.incomeKinds;
    if (fromIndex >= 0 && fromIndex < kinds.length
      && toIndex >= 0 && toIndex < kinds.length && fromIndex != toIndex) {
      let kind = kinds[fromIndex];
      kinds.splice(fromIndex, 1);
      kinds.splice(toIndex, 0, kind);
      settingsAreModified = true;
    }
  },
  isIncomeKindInUse(kind: string): boolean {
    let kinds = settings.value.incomeKinds;
    for (let key in data.value) {
      let rows = data.value[key];
      if (rows !== undefined && rows.find((row)=>row.kind === kind) !== undefined) {
        return true;
      }
    }
    return false;
  },
  insertPaymentKind(kind: string, index: number): void {
    settings.value.paymentKinds.splice(index, 0, kind);
    settingsAreModified = true;
  },
  deletePaymentKind(index: number): void {
    settings.value.paymentKinds.splice(index, 1);
    settingsAreModified = true;
  },
  replacePaymentKind(kind: string, index: number): void {
    if (index >= 0 && index < settings.value.paymentKinds.length) {
      settings.value.paymentKinds[index] = kind;
      settingsAreModified = true;
    }
  },
  movePaymentKind(fromIndex: number, toIndex: number): void {
    let kinds = settings.value.paymentKinds;
    if (fromIndex >= 0 && fromIndex < kinds.length
      && toIndex >= 0 && toIndex < kinds.length && fromIndex != toIndex) {
      let kind = kinds[fromIndex];
      kinds.splice(fromIndex, 1);
      kinds.splice(toIndex, 0, kind);
      settingsAreModified = true;
    }
  },
  isPaymentKindInUse(kind: string): boolean {
    let kinds = settings.value.paymentKinds;
    for (let key in data.value) {
      let rows = data.value[key];
      if (rows !== undefined && rows.find((row)=>row.kind === kind) !== undefined) {
        return true;
      }
    }
    return false;
  },
  insertCardEntry(name: string, closing: number, index: number): void {
    const entry: CardEntry = { name: name, closing: closing };
    settings.value.cards.splice(index, 0, entry);
    settingsAreModified = true;
  },
  changeCardEntry(name: string | undefined, closing: number | undefined, index: number): void {
    let cards = settings.value.cards;
    if (index >= 0 && index < cards.length) {
      if (name !== undefined) {
        cards[index].name = name;
        settingsAreModified = true;
      }
      if (closing !== undefined) {
        cards[index].closing = closing;
        settingsAreModified = true;
      }
    }
  },
  deleteCardEntry(index: number): void {
    settings.value.cards.splice(index, 1);
    settingsAreModified = true;
  },
  moveCardEntry(fromIndex: number, toIndex: number): void {
    let cards = settings.value.cards;
    if (fromIndex >= 0 && fromIndex < cards.length
      && toIndex >= 0 && toIndex < cards.length && fromIndex != toIndex) {
      let card = cards[fromIndex];
      cards.splice(fromIndex, 1);
      cards.splice(toIndex, 0, card);
      settingsAreModified = true;
    }
  },
  isCardEntryInUse(name: string): boolean {
    for (let key in data.value) {
      let rows = data.value[key];
      if (rows !== undefined && rows.find((row)=>row.card === name) !== undefined) {
        return true;
      }
    }
    return false;
  }
};

/*  現在の月（１秒ごとに更新する。変更があれば他のコンポーネントにも周知）  */
const thisMonth = ref(0);

/*  表示中の月  */
const pageMonth = ref(0);

/*  pageMonth を変更する（子コンポーネントから呼び出せる）  */
function setPageMonth(ym: number): void {
  pageMonth.value = ym;
}

/*  データとメソッドを公開  */
provide(dataKey, { data, methods });
/*  設定を公開  */
provide(settingsKey, { settings, settingsMethods } );
/*  現在の月を公開  */
provide(monthsKey, { thisMonth, pageMonth, setPageMonth } );

/*  「今日は○月○日」の表示用変数  */
const today = ref("");

/*  アクティブなタブ  */
const activeTab = ref(0);

/*  タブを選択する  */
function selectTab(tabIndex: number): void {
  if (tabIndex >= 0 && tabIndex <= 4) {
    activeTab.value = tabIndex;
  }
}

/*  データ数を求める  */
function countRows(d: DataType): number {
  return Object.keys(d).reduce((acc, cur) => acc + d[Number(cur)]?.length, 0);
}

/*  文字列のコンマ・二重引用符・制御文字を「%+16進数2桁」でエスケープ  */
function encodeHex(s: string) {
  const re = /[\x00-\x1f\%,"]/g;
  return s.replace(re, (match) => "%" + match.charCodeAt(0).toString(16).padStart(2, "0"));
}

/*  エスケープした16進数を元の文字に戻す  */
function decodeHex(s: string) {
  const re = /%([0-9a-fA-F]{2})/g;
  return s.replace(re, (_, p1) => String.fromCharCode(parseInt(p1, 16)));
}
/*  データ等をcsvに変換  */
function writeDataToString(): string {
  const ary: string[] = [];
  /*  incomeKinds, paymentKinds  */
  ary.push("[incomeKinds]\n");
  for (let item of settings.value.incomeKinds) {
    ary.push(encodeHex(item) + "\n");
  }
  ary.push("[paymentKinds]\n");
  for (let item of settings.value.paymentKinds) {
    ary.push(encodeHex(item) + "\n");
  }
  /*  cards  */
  ary.push("[cards]\n");
  for (let card of settings.value.cards) {
    ary.push(encodeHex(card.name) + "," + String(card.closing || 0) + "\n");
  }
  /*  data  */
  ary.push("[data]\n");
  let keys = Object.keys(data.value).map(v => Number(v));
  for (let key of keys) {
    let d = data.value[key];
    if (d !== undefined) {
      for (let row of d) {
        let s = String((row.date !== undefined ? row.date : 0) + key * 100);
        s = s + "," + encodeHex(row.item || "");
        s = s + "," + encodeHex(row.kind || "");
        s = s + "," + (row.isIncome ? "1" : "0");
        s = s + "," + String(row.amount || 0);
        s = s + "," + encodeHex(row.card || "");
        ary.push(s + "\n");
      }
    }
  }
  return ary.join("");
}

/*  csv をデータ等に変換  */
function readDataFromString(csv: string) {
  let ndata: {[index: number]: DataEntry[]} = {};
  let nincome: string[] = [];
  let npayment: string[] = [];
  let ncards: CardEntry[] = [];
  let stage = 0;
  for (let line of csv.split("\n")) {
    line = line.trim();
    if (line === "") {
      continue;
    }
    if (line === "[incomeKinds]") {
      stage = 1;
      continue;
    } else if (line === "[paymentKinds]") {
      stage = 2;
      continue;
    } else if (line === "[cards]") {
      stage = 3;
      continue;
    } else if (line === "[data]") {
      stage = 4;
      continue;
    }
    if (stage == 1) {
      nincome.push(decodeHex(line));
    } else if (stage == 2) {
      npayment.push(decodeHex(line));
    } else if (stage == 3) {
      let a = line.split(",");
      let name = decodeHex(a[0].trim());
      let closing = parseInt(a[1].trim());
      ncards.push({ name: name, closing: closing });
    } else if (stage == 4) {
      let a = line.split(",");
      let m = parseInt(a[0].trim());
      let date: number | undefined;
      date = m % 100;
      if (date == 0) {
        date = undefined;
      }
      m = Math.floor(m / 100);
      if (!(m in ndata)) {
        ndata[m] = [];
      }
      let item = decodeHex(a[1].trim());
      let kind = decodeHex(a[2].trim());
      let isIncome = (a[3].trim() === "1" ? true : false);
      let amount = parseInt(a[4].trim());
      let card = decodeHex(a[5].trim());
      ndata[m].push({date: date, item: item, kind: kind, isIncome: isIncome, amount: amount, card: card});
    } else {
      throw new Error("Bad CSV input");
    }
  }
  return {data: ndata, income: nincome, payment: npayment, cards: ncards};
}

async function initializeData() {
  let dataDir: string = "";
  let result;
  let stage = 0;
  let newFile = false;
  console.log("initializeData() invoked");
  console.log("VueRunner is" + (await isVueRunnerAvailable() ? "" : " not") + " running")
  try {
    if (await isVueRunnerAvailable()) {
      /*  home に kakeibo/${bookName} フォルダがなければ作成する  */
      console.log("465: kakeibo + bookName.value is " + "kakeibo/" + bookName.value);
      dataDir = await vJoin(await vHomeDir(), "kakeibo/" + bookName.value);
      console.log("467: dataDir is " + dataDir);
      if (!await vMkdir(dataDir, { recursive: true })) {
        throw new Error("cannot create " + dataDir);
      }
      /*  kakeibo.csv がなければ作成する  */
      stage = 1;
      let dataPath = await vJoin(dataDir, "kakeibo.csv");
      if (!await vExists(dataPath)) {
        await vCreate(dataPath);
        newFile = true;
      }
      /*  kakeibo.csv の内容を読む  */
      stage = 2;
      const dataText = await vReadTextFile(dataPath);
      /*  家計簿データとして解釈する  */
      stage = 3;
      result = readDataFromString(dataText);
    } else {
      if (testData !== "") {
        stage = 3;
        result = readDataFromString(testData);
      }
    }
  } catch (error: any) {
    let s: string;
    if (stage == 0) {
      s = "データ保存用のフォルダが用意できませんでした。\n(" + dataDir + ")";
    } else if (stage == 1) {
      s = "データファイル kakeibo.csv が用意できませんでした。";
    } else if (stage == 2) {
      s = "データファイル kakeibo.csv が読み込めませんでした。";
    } else if (stage == 3) {
      s = "データファイル kakeibo.csv の内容に問題があるようです。";
    } else {
      s = "エラーが発生しました。\n(" + error.name + ": " + error.message + ")";
    }
    await myAlertAsync(s);
  } finally {
    if (result !== undefined) {
      data.value = result.data;
      if (result.income.length > 0) {
        settings.value.incomeKinds = result.income;
      }
      if (result.payment.length > 0) {
        settings.value.paymentKinds = result.payment;
      }
      if (result.cards.length > 0) {
        settings.value.cards = result.cards;
      }
    }
  }
  /* データのある一番最近の月をpageMonthに設定 */
  let ym = endMonthInData(data.value);
  if (ym === undefined) {
    ym = thisMonth.value;
  }
  setPageMonth(ym);
  /* もし新規ファイルを作成したなら一度書き込みをしておく */
  if (newFile) {
    await writeData();
  }
}

/*  kakeibo ディレクトリを探索して、現在存在する家計簿の名前のリストを作る  */
async function initializeBookNames() {
  if (!(await isVueRunnerAvailable())) {
    return;
  }
  const rootDir = await vJoin(await vHomeDir(), "kakeibo");
  const dirs = await vReadDir(rootDir);
  let filteredDirs: string[] = [];
  for (let name of dirs) {
    if (name !== "default") {
      if (await vExists(await vJoin(rootDir, name + "/kakeibo.csv"))) {
        filteredDirs.push(name);
      }
    }
  }
  bookNames.value = filteredDirs;
  console.log("filteredDirs = " + filteredDirs);
}

/*  最後にバックアップを処理した日付  */
let lastBackupDate = 0;

/* バックアップファイルの管理 */
async function handleBackup(dataDir: string, file: string) {
  /* 最後にこの処理をした日が今日なら何もしない（１日に１回だけ処理する） */
  const date = new Date();
  let ymd = (date.getFullYear() * 10000 + (date.getMonth() + 1) * 100 + date.getDate());
  if (lastBackupDate === ymd) {
    return;
  }
  lastBackupDate = ymd;
  let stage = 1;
  if (!(await isVueRunnerAvailable())) {
    return;
  }
  try {
    /* ファイル名を本体と拡張子に分ける */
    const m = file.match(/(^.*)(\.\w*)$/);
    const bname = (m ? m[1] : file);
    const ext = (m ? m[2] : "");
    /* もし「ファイル名_今日.拡張子」が存在しなければ、現存のファイルをその名前に変更する */
    const fullPath = await vJoin(dataDir, bname + "_" + String(ymd) + ext);
    if (!await vExists(fullPath)) {
      await vRename(await vJoin(dataDir, file), fullPath);
    }
    stage = 2;
    /* 「ファイル名_dddddddd.拡張子」にマッチするファイルのリストを取得して、降順に並べる  */
    const re = new RegExp("^" + bname + "_\\d{8}" + (ext == "" ? "" : "\\" + ext) + "$");
    let entries;
    entries = (await vReadDir(dataDir))
      .filter((entry)=>entry.match(re))
      .sort().reverse();      
    /*  最初の10個は残す  */
    /*  その次の5個は、最後の1文字のみ違うものを１つずつ残す（10日に1個）  */
    /*  その次の5個は、最後の2文字のみ違うものを１つずつ残す（1ヶ月に1個）  */
    /*  そのあとは、最後の4文字のみ違うものを１つずつ残す（1年に1個）  */
    let count = 0;
    stage = 3;
    let last_entry = undefined;
    const blen = bname.length + 1;
    for (let i = 0; i < entries.length; i++) {
      const entry = entries[i];
      if (entry === undefined) {
        continue;
      }
      let remove_flag = false;
      if (stage == 3) {
        count += 1;
        if (count == 10) {
          stage = 4;
        }
      } else if (stage == 4) {
        if (last_entry !== undefined && last_entry.slice(0, blen + 7) === entry.slice(0, blen + 7)) {
          remove_flag = true;
        } else {
          count += 1;
          if (count == 15) {
            stage = 5;
          }
        }
      } else if (stage == 5) {
        if (last_entry !== undefined && last_entry.slice(0, blen + 6) === entry.slice(0, blen + 6)) {
          remove_flag = true;
        } else {
          count += 1;
          if (count == 20) {
            stage = 6;
          }
        }
      } else if (stage == 6) {
        if (last_entry !== undefined && last_entry.slice(0, blen + 4) === entry.slice(0, blen + 4)) {
          remove_flag = true;
        } else {
          count += 1;
        }
      }
      if (remove_flag) {
        await vRemove(await vJoin(dataDir, entry));
      } else {
        last_entry = entry;
      }
    }
  } catch (error: any) {
    let s: string;
    if (stage == 0) {
      s = "データファイルの最終更新日時を取得できませんでした。";
    } else if (stage == 1) {
      s = "直前のデータファイルの名称変更に失敗しました。";
    } else if (stage == 2) {
      s = "古いデータファイルの削除に失敗しました。";
    } else {
      s = "エラーが発生しました。\n(" + error.name + ": " + error.message + ")";
    }
    await myAlertAsync(s);
    throw new Error("バックアップ作成中にエラー（アラート表示済）");
  }
}

/* データ・設定をファイルに保存 */
async function writeData() {
  let stage = 0;
  try {
    if (await isVueRunnerAvailable()) {
      /*  データファイルを更新  */
      let dataDir = await vJoin(await vHomeDir(), "kakeibo/" + bookName.value);
      let dataPath = await vJoin(dataDir, "kakeibo.csv");
      const csv = writeDataToString();
      stage = 1;
      /*  バックアップを残す  */
      await handleBackup(dataDir, "kakeibo.csv");
      stage = 2;
      await vWriteTextFile(dataPath, csv);
      dataAreModified = false;
      settingsAreModified = false;
    }
  } catch (error: any) {
    let s: string;
    if (stage == 0) {
      s = "データの保存内容を作成できませんでした。";
    } else if (stage == 1) {
      return; /* alert 表示済み */
    } else if (stage == 2) {
      s = "データファイル kakeibo.csv に書き込みできませんでした。";
    } else {
      s = "エラーが発生しました。\n(" + error.name + ": " + error.message + ")";
    }
    await myAlertAsync(s);
  }
}

/* today, thisMonth を更新する */
function updateToday() {
  const date = new Date();
  let y = date.getFullYear();
  let m = date.getMonth() + 1;
  let d = date.getDate();
  today.value = "今日は" + String(m) + "月" + String(d) + "日"
  thisMonth.value = y * 100 + m;
}

/*  １秒ごとに実行するハンドラ  */
async function timerHandler() {
  if (await isVueRunnerAvailable() && (dataAreModified || settingsAreModified)) {
    await writeData();
  }
}

/*  家計簿を切り替え、または新規作成  */
async function onBookSelected(event: Event) {
  if (event.target === null) {
    return;
  }
  let newBookName = (event.target as HTMLSelectElement).value;
  if (newBookName === "new") {
    const result = await myAskAsync("新しい家計簿の名前：", (str: string): string => {
      if (str.match(/[\\/:*?"<>|]/)) {
        return "次の文字を使わないでください: \\ / : * ? \" < > |";
      } else if (str.match(/(^\.)|(\.$)/)) {
        return "ピリオドで始まる名前、終わる名前は使わないでください";
      } else if (str.match(/(^\s)|(\s$)/)) {
        return "空白文字で始まる名前、終わる名前は使わないでください";
      } else if (str.match(/[\0-\x1f]/)) {
        return "制御文字を使わないでください";
      } else if (bookNames.value?.includes(str)) {
        return "この名前はすでに使われています";
      } else {
        return "";
      }
    });
    if (result === "") {
      return;
    }
    newBookName = result;
    console.log(`${newBookName} is to be created`);
  } else {
    console.log(`${newBookName} is selected`);
  }
  /*  現在の家計簿を保存  */
  if (await isVueRunnerAvailable()) {
    await writeData();
  }
  /*  家計簿の名前を切り替えて初期化  */
  bookName.value = newBookName;
  data.value = {};
  settings.value = JSON.parse(defaultSettings);
  await initializeData();
  if (newBookName !== "default" && !bookNames.value.includes(newBookName)) {
    bookNames.value.push(newBookName);
  }
  nextTick(() => {
    if (selectBook.value !== undefined) {
      selectBook.value.value = bookName.value;
    }
  })
}

let intervalID: ReturnType<typeof setInterval>;
onMounted(async () => {
  updateToday();
  await initializeData();
  await initializeBookNames();
  intervalID = setInterval(timerHandler, 1000);
  if (await isVueRunnerAvailable()) {
    window.addEventListener('beforeunload', (event) => {
      event.stopImmediatePropagation();   /* 離脱防止アラートを抑制する */
      vTerminate();  /* サーバを終了する  */
    }, true);
    vListenToServer(async (event) => {
      if (event.data === "stop") {
        await myAlertAsync("アプリ本体が閉じられました。このタブを閉じてください。");
        window.close();
      }
    });
  }
});
onUnmounted(async () => {
  clearInterval(intervalID);
  if (await isVueRunnerAvailable()) {
    await writeData();
  }
});

/*  最初に thisMonth を初期化しておく  */
updateToday();

</script>

<template>
  <div class="main_window">
    <div id="today">
      {{ today }}
    </div>
    <div id="menu_box">
      <select id="whom" style="width:125px" @change="onBookSelected" ref="selectBook">
        <option value="default" selected>わたし</option>
        <option v-for="name in bookNames" :key="name" :value="name">
        {{ name }}
        </option>
        <option value="-" disabled>------</option>
        <option value="new">新しい家計簿を作成…</option>
      </select>
      <span id="of">の</span>
      <IconButton :label="'家計簿'" style="top:0px;left:150px;width:60px;height:24px;"
        :selected="activeTab == 0" @click="selectTab(0)" />
      <IconButton :label="'内訳別グラフ'" style="top:0px;left:210px;width:90px;height:24px;"
        :selected="activeTab == 1" @click="selectTab(1)" />
      <IconButton :label="'カード別一覧'" style="top:0px;left:300px;width:90px;height:24px;"
        :selected="activeTab == 2" @click="selectTab(2)" />
      <IconButton :label="'月別収支一覧'" style="top:0px;left:390px;width:90px;height:24px;"
        :selected="activeTab == 3" @click="selectTab(3)" />
      <IconButton :label="'設定'" style="top:0px;left:480px;width:60px;height:24px;"
        :selected="activeTab == 4" @click="selectTab(4)" />
    </div>
    <div class="tab_box">
      <MainTab v-if="activeTab == 0" />
      <GraphTab v-if="activeTab == 1" />
      <CardTab v-if="activeTab == 2" />
      <TableTab v-if="activeTab == 3" />
      <SettingsTab v-if="activeTab == 4" />
    </div>
  </div>
</template>

<style scoped>
.main_window {
    position: absolute;
    left: 0px;
    top: 0px;
    right: 0px;
    margin: 0 auto; /* 左右中央揃え　*/
    width: 720px;
    height: 520px;
    font-size: 11px;
    background-color: #ddf;
}
#today {
  position: absolute;
  left: 590px;
  top: 7px;
  width: 120px;
  height: 20px;
  border: 1px solid grey;
  text-align: center;
  background-color: white;
  font-size: 12px;
}
#whom {
  display: inline-block;
  width: 125px;
  height: 22px;
  text-align: center;
  background-color: white;
  border: 1px solid black;
  font-size: 14px;
}
#whom::after {
  display: block;
	content: "";
	position: absolute;
	top: 15px;
	left: 115px;
	width: 0;
	height: 0;
	margin-right: 0px;
	border: 5px solid transparent;        /*top right bottom を透明化 */ 
	border-top: 5px solid #404040;
}
#of {
  display: inline-block;
  width: 25px;
  height: 22px;
  text-align: center;
  font-size: 14px;
}
#menu_box {
  position: absolute;
  left: 10px;
  top: 7px;
}
.tab_box {
  position:absolute;
  left: 10px;
  top: 32px;
  width: 700px;
  height: 480px;
  border: 1px solid #888;
}
#new_book_dialog button {
  width: 7em;
  margin: 0 1em;
}
@media print {
  #today { display: none; }
  #menu_box { display: none; }
  .tab_box { top: 10px; height: auto; border: none; }
}
</style>
