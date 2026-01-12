<script setup lang="ts">
import { ref, inject, computed, onMounted, nextTick } from "vue"
import type { Ref } from "vue"
import type { DataEntry, DataType, DataMethods, CardEntry, Settings, SettingsMethods } from "../types.ts"
import { settingsKey } from "./MainWindow.vue"
import IconButton from "./IconButton.vue"
import DataTable from "./DataTable.vue"
import type { DataTableSource } from "./DataTable.vue"
import { myAlertAsync } from "../utils.ts"

/*  DataTable を保持するレファレンス  */
const incomeTable = ref<typeof DataTable>();
const paymentTable = ref<typeof DataTable>();
const cardTable = ref<typeof DataTable>();

/*  新しい値を追加中かどうか（最後の行にだけ追加できる） */
const isAppendingIncome = ref(false);
const isAppendingPayment = ref(false);
const isAppendingCard = ref(false);

/*  MainWindow のデータとメソッドにアクセス  */
const { settings, settingsMethods } = inject(settingsKey) as { settings: Ref<Settings>; settingsMethods: SettingsMethods; };

/*  収入費目のデータソースオブジェクト  */
const incomeSource : DataTableSource = {
  numberOfColumns(): number {
    return 1;
  },
  columnName(_column: number): string {
    return "収入費目";
  },
  columnWidth(_column: number): number {
    return 80;
  },
  columnType(_column: number): string {
    return "input";
  },
  selectItems(_column: number): any[] {
    return [];
  },
  numberOfRows(): number {
    return (settings.value?.incomeKinds.length || 0);
  },
  valueAt(row: number, _column: number): string {
    return (settings.value?.incomeKinds[row] || "");
  },
  optionalStyleAt(row: number, _column: number): string {
    if (settingsMethods.isIncomeKindInUse(settings.value?.incomeKinds[row])) {
      return "font-style:italic;color:red;";
    } else {
      return "";
    }
  },
  setValueAt(value: string, row: number, column: number): void {
    let s = settings.value;
    if (s === undefined || s.incomeKinds === undefined) {
      return;
    }
    /*  value が空文字列の場合はそのエントリを削除  */
    if (value === "") {
      settingsMethods.deleteIncomeKind(row);
      return;
    }
    /*  value が変更されていない場合は何もしない  */
    if (s.incomeKinds[row] === value) {
      return;
    }
    if (s.incomeKinds.indexOf(value) >= 0 || 
      (s.paymentKinds !== undefined && s.paymentKinds.indexOf(value) >= 0)) {
      /*  値がダブっていた場合警告を出して更新拒否  */
      myAlertAsync("費目「" + value + "」はすでにあるので登録できません")
      .then(() => {
        /* 新しい費目の編集を再開 */
        incomeTable.value?.requestEdit(row, column);
      });
    } else {
      settingsMethods.replaceIncomeKind(value, row);
      isAppendingIncome.value = false;
    }
  },
  isEditable(_row: number, _column: number): boolean {
    return false;
  },
  isSelectable(_row: number): boolean {
    return true;
  },
  finalized(): void {
    return;
  },
  /*  データ本体  */
  data: computed(() => settings.value?.incomeKinds)
};

/*  支出費目のデータソースオブジェクト  */
const paymentSource : DataTableSource = {
  numberOfColumns(): number {
    return 1;
  },
  columnName(_column: number): string {
    return "支出費目";
  },
  columnWidth(_column: number): number {
    return 80;
  },
  columnType(_column: number): string {
    return "input";
  },
  selectItems(_column: number): any[] {
    return [];
  },
  numberOfRows(): number {
    return (settings.value?.paymentKinds.length || 0);
  },
  valueAt(row: number, _column: number): string {
    return (settings.value?.paymentKinds[row] || "");
  },
  optionalStyleAt(row: number, _column: number): string {
    if (settingsMethods.isPaymentKindInUse(settings.value?.paymentKinds[row])) {
      return "font-style:italic;color:red;";
    } else {
      return "";
    }
  },
  setValueAt(value: string, row: number, column: number): void {
    let s = settings.value;
    if (s === undefined || s.paymentKinds === undefined) {
      return;
    }
    /*  value が空文字列の場合はそのエントリを削除  */
    if (value === "") {
      settingsMethods.deletePaymentKind(row);
      return;
    }
    /*  value が変更されていない場合は何もしない  */
    if (s.paymentKinds[row] === value) {
      return;
    }
    if (s.paymentKinds.indexOf(value) >= 0 || 
      (s.incomeKinds !== undefined && s.incomeKinds.indexOf(value) >= 0)) {
      /*  値がダブっていた場合警告を出して更新拒否  */
      myAlertAsync("費目「" + value + "」はすでにあるので登録できません")
      .then(() => {
        /* 新しい費目の編集を再開 */
        paymentTable.value?.requestEdit(row, column);
      });
    } else {
      settingsMethods.replacePaymentKind(value, row);
      isAppendingPayment.value = false;
    }
  },
  isEditable(_row: number, _column: number): boolean {
    return false;
  },
  isSelectable(_row: number): boolean {
    return true;
  },
  finalized(): void {
    return;
  },
  /*  データ本体  */
  data: computed(() => settings.value?.paymentKinds)
};

/*  カード情報のデータソースオブジェクト  */
const cardSource : DataTableSource = {
  numberOfColumns(): number {
    return 2;
  },
  columnName(column: number): string {
    if (column == 0) {
      return "カード名称";
    } else if (column == 1) {
      return "締め日";
    } else {
      return "";
    }
  },
  columnWidth(_column: number): number {
    return 80;
  },
  columnType(_column: number): string {
    return "input";
  },
  selectItems(_column: number): any[] {
    return [];
  },
  numberOfRows(): number {
    return (settings.value?.cards.length || 0);
  },
  valueAt(row: number, column: number): string {
    let c = settings.value?.cards?.[row];
    if (c !== undefined) {
      if (column == 0) {
        return c.name;
      } else if (column == 1) {
        if (c.closing === undefined || c.closing == 0) {
          return "";
        } else {
          return String(c.closing);
        }
      }
    }
    return "";
  },
  optionalStyleAt(row: number, column: number): string {
    if (column == 0 && settingsMethods.isCardEntryInUse(settings.value?.cards[row]?.name)) {
      return "font-style:italic;color:red;";
    } else {
      return "";
    }
  },
  setValueAt(value: string, row: number, column: number): void {
    let s = settings.value;
    if (s === undefined || s.cards === undefined) {
      return;
    }
    /*  カード名が空文字列の場合はそのエントリを削除  */
    if (column == 0 && value === "") {
      settingsMethods.deleteCardEntry(row);
      return;
    }
    /*  カード名が変更されていない場合は何もしない  */
    if (column == 0 && s.cards[row].name === value) {
      return;
    }
    if (column == 0 && s.cards.findIndex((c) => c.name === value) >= 0) {
      /*  値がダブっていた場合警告を出して更新拒否  */
      myAlertAsync("費目「" + value + "」はすでにあるので登録できません")
      .then(() => {
        /* カード名の編集を再開 */
        cardTable.value?.requestEdit(row, column);
      });
      return;
    }
    let name = (column == 0 ? value : undefined);
    let closing = (column == 1 ? Number(value) : undefined);
    settingsMethods.changeCardEntry(name, closing, row);
    isAppendingCard.value = false;
  },
  isEditable(_row: number, column: number): boolean {
    if (column == 0) {
      return false;
    } else {
      return true;
    }
  },
  isSelectable(_row: number): boolean {
    return true;
  },
  finalized(): void {
    return;
  },
  /*  データ本体  */
  data: computed(() => settings.value?.cards)
};

/*  新しい行を作成して編集開始  */
function newIncomeKind() {
  isAppendingIncome.value = true;
  const row = settings.value?.incomeKinds.length;
  settingsMethods.insertIncomeKind("", row);
  incomeTable.value?.requestEdit(row, 0);
}
function newPaymentKind() {
  isAppendingPayment.value = true;
  const row = settings.value?.paymentKinds.length;
  settingsMethods.insertPaymentKind("", row);
  paymentTable.value?.requestEdit(row, 0);
}
function newCardEntry() {
  isAppendingCard.value = true;
  const row = settings.value?.cards.length;
  settingsMethods.insertCardEntry("", 0, row);
  cardTable.value?.requestEdit(row, 0);
}

/*  選択された行を削除  */
function deleteSelectedIncomeKinds() {
  if (incomeTable.value === undefined) {
    return;
  }
  let selection = Array.from(incomeTable.value.selectedRows) as number[];
  if (selection !== undefined && selection.length > 0) {
    selection = selection.sort().reverse();
    for (let row of selection) {
      if (!settingsMethods.isIncomeKindInUse(settings.value?.incomeKinds[row])) {
        settingsMethods.deleteIncomeKind(row);
      }
    }
    incomeTable.value.selectedRows.length = 0;
  }
}
function deleteSelectedPaymentKinds() {
  if (paymentTable.value === undefined) {
    return;
  }
  let selection = Array.from(paymentTable.value.selectedRows) as number[];
  if (selection !== undefined && selection.length > 0) {
    selection = selection.sort().reverse();
    for (let row of selection) {
      if (!settingsMethods.isPaymentKindInUse(settings.value?.paymentKinds[row])) {
        settingsMethods.deletePaymentKind(row);
      }
    }
    paymentTable.value.selectedRows.length = 0;
  }
}
function deleteSelectedCardEntries() {
  if (cardTable.value === undefined) {
    return;
  }
  let selection = Array.from(cardTable.value.selectedRows) as number[];
  if (selection !== undefined && selection.length > 0) {
    selection = selection.sort().reverse();
    for (let row of selection) {
      if (!settingsMethods.isCardEntryInUse(settings.value?.cards[row].name)) {
        settingsMethods.deleteCardEntry(row);
      }
    }
    cardTable.value.selectedRows.length = 0;
  }
}

/*  選択された行を１つずつ移動  */
function moveIncomeRows(up: boolean) {
  let selection = Array.from(incomeTable.value?.selectedRows) as number[];
  if (selection !== undefined && selection.length > 0) {
    let dr = -1;
    selection = selection.sort();
    if (up) {
      if (selection[0] == 0) {
        return;
      }
    } else {
      selection = selection.reverse();
      if (selection[0] == incomeSource.numberOfRows() - 1) {
        return;
      }
      dr = 1;
    }
    for (let row of selection) {
      settingsMethods.moveIncomeKind(row, row + dr);
    }
    if (incomeTable.value !== undefined) {
      for (let i = 0; i < incomeTable.value.selectedRows.length; i++) {
        incomeTable.value.selectedRows[i] += dr;
      }
    }
  }
}
function movePaymentRows(up: boolean) {
  let selection = Array.from(paymentTable.value?.selectedRows) as number[];
  if (selection !== undefined && selection.length > 0) {
    let dr = -1;
    selection = selection.sort();
    if (up) {
      if (selection[0] == 0) {
        return;
      }
    } else {
      selection = selection.reverse();
      if (selection[0] == paymentSource.numberOfRows() - 1) {
        return;
      }
      dr = 1;
    }
    for (let row of selection) {
      settingsMethods.movePaymentKind(row, row + dr);
    }
    if (paymentTable.value !== undefined) {
      for (let i = 0; i < paymentTable.value.selectedRows.length; i++) {
        paymentTable.value.selectedRows[i] += dr;
      }
    }
  }
}
function moveCardRows(up: boolean) {
  let selection = Array.from(cardTable.value?.selectedRows) as number[];
  if (selection !== undefined && selection.length > 0) {
    let dr = -1;
    selection = selection.sort();
    if (up) {
      if (selection[0] == 0) {
        return;
      }
    } else {
      selection = selection.reverse();
      if (selection[0] == cardSource.numberOfRows() - 1) {
        return;
      }
      dr = 1;
    }
    for (let row of selection) {
      settingsMethods.moveCardEntry(row, row + dr);
    }
    if (cardTable.value !== undefined) {
      for (let i = 0; i < cardTable.value.selectedRows.length; i++) {
        cardTable.value.selectedRows[i] += dr;
      }
    }
  }
}

/*  ボタンが有効になる条件  */
const canDeleteIncomeRows = computed(() => {
  let selectedRows = incomeTable.value?.selectedRows;
  if (selectedRows !== undefined && selectedRows.length > 0) {
    if (selectedRows.findIndex((row: number) => 
      !settingsMethods.isIncomeKindInUse(settings.value?.incomeKinds[row])) >= 0) {
      return true;
    }
  }
  return false;
});
const canMoveUpIncomeRows = computed(() => {
  let selectedRows = incomeTable.value?.selectedRows;
  return (selectedRows !== undefined && selectedRows.length > 0 && selectedRows.indexOf(0) < 0);
});
const canMoveDownIncomeRows = computed(() => {
  let selectedRows = incomeTable.value?.selectedRows;
  return (selectedRows !== undefined && selectedRows.length > 0 && selectedRows.indexOf(incomeSource.numberOfRows() - 1) < 0);
});
const canDeletePaymentRows = computed(() => {
  let selectedRows = paymentTable.value?.selectedRows;
  if (selectedRows !== undefined && selectedRows.length > 0) {
    if (selectedRows.findIndex((row: number) =>
      !settingsMethods.isPaymentKindInUse(settings.value?.paymentKinds[row])) >= 0) {
      return true;
    }
  }
  return false;
});
const canMoveUpPaymentRows = computed(() => {
  let selectedRows = paymentTable.value?.selectedRows;
  return (selectedRows !== undefined && selectedRows.length > 0 && selectedRows.indexOf(0) < 0);
});
const canMoveDownPaymentRows = computed(() => {
  let selectedRows = paymentTable.value?.selectedRows;
  return (selectedRows !== undefined && selectedRows.length > 0 && selectedRows.indexOf(paymentSource.numberOfRows() - 1) < 0);
});
const canDeleteCardRows = computed(() => {
  let selectedRows = cardTable.value?.selectedRows;
  if (selectedRows !== undefined && selectedRows.length > 0) {
    if (selectedRows.findIndex((row: number) => 
      !settingsMethods.isCardEntryInUse(settings.value?.cards[row]?.name)) >= 0) {
      return true;
    }
  }
  return false;
});
const canMoveUpCardRows = computed(() => {
  let selectedRows = cardTable.value?.selectedRows;
  return (selectedRows !== undefined && selectedRows.length > 0 && selectedRows.indexOf(0) < 0);
});
const canMoveDownCardRows = computed(() => {
  let selectedRows = cardTable.value?.selectedRows;
  return (selectedRows !== undefined && selectedRows.length > 0 && selectedRows.indexOf(cardSource.numberOfRows() - 1) < 0);
});

/*  data URL にエンコードする  */
import plusURL from "../../src/assets/plus.svg?inline";
import minusURL from "../../src/assets/minus.svg?inline";
import downArrowURL from "../../src/assets/down-arrow.svg?inline";
import upArrowURL from "../../src/assets/up-arrow.svg?inline";

</script>

<template>

  <DataTable :source="incomeSource" ref="incomeTable"
    style="position:absolute;left:90px;top:50px;height:314px" />
  <IconButton :source="plusURL" style="top:369px;left:90px;" 
    @click="newIncomeKind" />
  <IconButton :source="minusURL" style="top:369px;left:112px;"
    :enabled="canDeleteIncomeRows"
    @click="deleteSelectedIncomeKinds" />
  <IconButton :source="upArrowURL" style="top:369px;left:134px;"
    :enabled="canMoveUpIncomeRows"
    @click="moveIncomeRows(true)" />
  <IconButton :source="downArrowURL" style="top:369px;left:156px;"
    :enabled="canMoveDownIncomeRows"
    @click="moveIncomeRows(false)" />

  <DataTable :source="paymentSource" ref="paymentTable"
    style="position:absolute;left:220px;top:50px;height:314px;" />
  <IconButton :source="plusURL" style="top:369px;left:220px;"
    @click="newPaymentKind" />
  <IconButton :source="minusURL" style="top:369px;left:242px;" 
    :enabled="canDeletePaymentRows"
    @click="deleteSelectedPaymentKinds" />
  <IconButton :source="upArrowURL" style="top:369px;left:264px;" 
    :enabled="canMoveUpPaymentRows"
    @click="movePaymentRows(true)" />
  <IconButton :source="downArrowURL" style="top:369px;left:286px;" 
    :enabled="canMoveDownPaymentRows"
    @click="movePaymentRows(false)" />

  <DataTable :source="cardSource" ref="cardTable"
    style="position:absolute;left:350px;top:50px;height:314px;" />
  <IconButton :source="plusURL" style="top:369px;left:350px;"
    @click="newCardEntry" />
  <IconButton :source="minusURL" style="top:369px;left:372px;" 
    :enabled="canDeleteCardRows"
    @click="deleteSelectedCardEntries" />
  <IconButton :source="upArrowURL" style="top:369px;left:394px;" 
    :enabled="canMoveUpCardRows"
    @click="moveCardRows(true)" />
  <IconButton :source="downArrowURL" style="top:369px;left:416px;" 
    :enabled="canMoveDownCardRows"
    @click="moveCardRows(false)" />

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

</style>
