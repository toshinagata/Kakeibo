<script setup lang="ts">
import { ref, inject, computed, onMounted, onUnmounted, nextTick } from "vue"
import type { Ref } from "vue"
import type { DataEntry, DataType, DataMethods, Settings, SettingsMethods } from "../types.ts"
import IconButton from "./IconButton.vue"
import DataTable from "./DataTable.vue"
import PrintButton from "./PrintButton.vue"
import type { DataTableSource } from "./DataTable.vue"
import { UndoManager } from "../undoManager.ts"
import { dataKey, settingsKey, monthsKey } from "./MainWindow.vue"
import { yearMonthToString, nextMonth, lastMonth, firstMonthInData, endMonthInData, formatAmount, numberFromString } from "../utils.ts"

/*  DataTable を保持するレファレンス  */
const dataTable = ref<typeof DataTable>();
/*  DataTable 内の情報にアクセスするためのショートカット  */
const selectedRows = computed<number[] | undefined>(() => dataTable.value?.selectedRows);

/*  MainWindow のデータとメソッドにアクセス  */
const { data, methods } = inject(dataKey) as { data: Ref<DataType>; methods: DataMethods; };
const { settings, settingsMethods } = inject(settingsKey) as { settings: Ref<Settings>; settingsMethods: SettingsMethods; };
const { thisMonth, pageMonth, setPageMonth } = inject(monthsKey) as {
  thisMonth: Ref<number>; pageMonth: Ref<number>; setPageMonth: (ym: number) => void;
};

/*  ファイル読み込み用の input 要素へのレファレンス  */
const fileInput = ref<HTMLInputElement>();
  
/*  pagemonth 月に対応するデータの配列  */
/*  (pageMonth の本体は MainWindow.vue にある)  */
const pageData = computed(() => data.value[pageMonth.value]);
/*  テーブルの行数。存在する行  */
const pageRows = computed(() => (pageData.value ? pageData.value.length : 0) + 1);

/*  表示可能な最初の月、最後の月  */
const firstPageMonth = computed(() => {
  let ym = firstMonthInData(data.value)
  if (ym === undefined) {
    ym = thisMonth.value;
  }
  return lastMonth(ym); /* １つ前の月まで表示可能 */
});
const endPageMonth = computed(() => {
  let ym = endMonthInData(data.value)
  if (ym === undefined || ym < thisMonth.value) {
    ym = thisMonth.value;
  }
  return ym;
});
const pageMonthList = computed(() => {
  /* 月のリスト：データのない月は負の数とし、２つ以上続く時は１つだけ残す */
  let ym = firstPageMonth.value;
  let ym_end = endPageMonth.value;
  let ls:number[] = [];
  while (ym <= ym_end) {
    let d = data.value[ym]
    if (ym > firstPageMonth.value && ym < ym_end && ym != pageMonth.value
     && (d === undefined || d.length == 0)) {
      if (ls.length == 0 || ls[ls.length - 1] > 0) {
        ls.push(-ym);
      }
    } else {
      ls.push(ym);
    }
    ym = nextMonth(ym);
  }
  return ls;
});

/*  「次の月」ボタンが有効かどうか  */
const nextMonthEnabled = computed(() => {
  return (pageMonth.value < endPageMonth.value);
});
/*  「前の月」ボタンが有効かどうか  */
const lastMonthEnabled = computed(() => {
  return (pageMonth.value > firstPageMonth.value);
})

/*  テーブル表示の定数  */
const columnNames = [ "・", "日付", "内容", "費目", "収入", "支出", "カード利用"];
const columnWidths = [ 16, 36, 144, 82, 84, 84, 82];
const columnTypes = [ "", "input", "input", "select", "input", "input", "select"];

/*  家計簿テーブルのデータソースオブジェクト  */
const dataSource : DataTableSource = {
  numberOfColumns(): number {
    return 7;
  },
  columnName(column: number): string {
    return columnNames[column] ?? "";
  },
  columnWidth(column: number): number {
    return columnWidths[column] ?? 0;
  },
  columnType(column: number): string {
    return columnTypes[column] ?? "";
  },
  selectItems(column: number): any[] {
    if (column == 3) {
      return [ { opt:"収入", items:settings.value.incomeKinds },
               { opt:"支出", items:settings.value.paymentKinds } ];
    } else if (column == 6) {
      return [ "（なし）" ].concat(settings.value.cards.map((x) => x.name));
    } else {
      return [];
    }
  },
  numberOfRows(): number {
    return pageRows.value;
  },
  valueAt(row: number, column: number): string {
    if (pageData.value === undefined) {
      return "";
    }
    let r = pageData.value[row];
    let editing = (dataTable.value?.editRow == row && dataTable.value?.editColumn == column);
    let format = (editing ? String : formatAmount);
    if (r !== undefined) {
      switch (column) {
        case 0: return "・";
        case 1: return (r.date !== undefined ? String(r.date) : "");
        case 2: return r.item ?? "";
        case 3: return r.kind ?? "";
        case 4: return (r.isIncome && r.amount !== undefined ? format(r.amount) : "");
        case 5: return (!r.isIncome && r.amount !== undefined ? format(-r.amount) : "");
        case 6: return r.card ?? "";
        default: return "";
      }
    }
    return "";
  },
  setValueAt(value: string, row: number, column: number): void {
    /*  セルの値を変更  */
    if (row == pageRows.value - 1) {
      /* 最終行の編集：新しい行を挿入する  */
      if (value == "") {
        return;  /* 入力がなかった場合は新しい行は作らない */
      }
      if (pageData.value === undefined) {
        /*  新しいページを作る  */
        addNewPage(pageMonth.value);
      }
      insertOneRow(row, undefined);
    }
    if (pageData.value) {
      if (column == 1) {
        replaceData(row, "date",
          (value === "" ? undefined : numberFromString(value)));
      } else if (column == 2) {
        replaceData(row, "item", value);
      } else if (column == 3) {
        replaceData(row, "kind", value);
        replaceData(row, "isIncome",
          (settings.value.incomeKinds.indexOf(value) >= 0));
      } else if (column == 4) {
        replaceData(row, "amount",
          (value === "" ? undefined : numberFromString(value)));
      } else if (column == 5) {
        replaceData(row, "amount",
          (value === "" ? undefined : -numberFromString(value)));
      } else if (column == 6) {
        replaceData(row, "card",
          (value === "（なし）" ? "" : value));
      }
    }
  },
  isEditable(row: number, column: number): boolean {
    if (column == 4 || column == 5) {
      let r = pageData.value?.[row];
      if (r !== undefined) {
        return ((r.isIncome && column == 4) || (!r.isIncome && column == 5));
      } else {
        return false;
      }
    } else if (column == 0) {
      return false;
    } else {
      return true;
    }
  },
  isSelectable(row: number): boolean {
    if (row == pageRows.value - 1) {
      return false;
    } else {
      return true;
    }
  },
  /*  データ本体：pageData と紐づける  */
  data: computed(() => pageData.value)
};

/*  合計を表示するテーブル  */
const netTableSource: DataTableSource = {
  numberOfColumns(): number {
    return 3;
  },
  titleHeight(): number {
    return 0;
  },
  columnName(_column: number): string {
    return "";
  },
  columnWidth(column: number): number {
    switch (column) {
      case 0: return 144;
      case 1: return 84;
      case 2: return 84;
      default: return 0;
    }
  },
  columnType(_column: number): string {
    return "";
  },
  selectItems(_column: number): any[] {
    return [];
  },
  numberOfRows(): number {
    return 3;
  },
  valueAt(row: number, column: number): string {
    if (column == 0) {
      switch (row) {
        case 0: return "選択行の合計";
        case 1: return "今月の合計";
        case 2: return "今月の収支";
      }
    } else if (column == 1 || column == 2) {
      if (pageData.value === undefined) {
        return "";
      } else if (row == 2 && column == 2) {
        return "---";
      } else {
        let sum = 0;
        for (let r = 0; r < pageRows.value - 1; r++) {
          if (row == 0 &&
            (selectedRows.value === undefined || selectedRows.value.indexOf(r) < 0)) {
            continue;
          }
          let v = pageData.value?.[r];
          if (v === undefined) {
            continue;
          }
          if ((row == 0 || row == 1) &&
            ((column == 1 && !v.isIncome) || (column == 2 && v.isIncome))) {
            continue;
          }
          sum += (v.amount || 0);
        }
        return formatAmount(column == 1 ? sum : -sum);
      }
    }
    return "";
  },
  setValueAt(_value: string, _row: number, _column: number): void {
  },
  isEditable(_row: number, _column: number): boolean {
    return false;
  },
  isSelectable(_row: number): boolean {
    return false;
  },
  data: undefined
}

/*  クリップボード（行削除、コピーの時に使う） */
const copiedRows = ref<DataEntry[]>([]);

/*  undoマネージャ  */
const undo = ref(new UndoManager());
undo.value.beforeUndoOrRedo = (_isUndoing: boolean) => {
  dataTable.value?.finalizeEditInput();
  if (selectedRows.value !== undefined) {
    selectedRows.value.length = 0;  /*  選択をクリア  */
  }
}

/*  行を選択する (undo用) */
function selectRowFromUndo(row: number) {
  if (selectedRows.value !== undefined && selectedRows.value.indexOf(row) < 0) {
    selectedRows.value.push(row);
  }
}

/*  row 行、key のデータを newValue に書き換える (undo対応)  */
function replaceData(row: number, key: keyof DataEntry, newValue: string | number | boolean | undefined) {
  if (pageData.value) {
    let r = pageData.value[row];
    if (r) {
      let v = r[key];
      if (v !== newValue) {
        undo.value.registerUndo(() => { replaceData(row, key, v); selectRowFromUndo(row) });
        methods.setValue(pageMonth.value, row, key, newValue);
      }
    }
  }
}
/*  row 行に新しいデータエントリを作成 (undo対応)  */
function insertOneRow(row: number, entry: DataEntry | undefined) {
  if (pageData.value && row >= 0 && row <= pageData.value.length) {
    undo.value.registerUndo(() => { deleteOneRow(row) });
    if (!entry) {
      entry = { date: undefined, item: "", kind: "", isIncome: false, amount: undefined, card: ""};
    }
    methods.insertRow(pageMonth.value, row, entry);
  }
}
/*  row 行を削除 (undo対応, row)  */
function deleteOneRow(row: number) {
  if (pageData.value && row >= 0 && row < pageData.value.length) {
    let entry = pageData.value[row];
    if (!entry) {
      entry = { date: undefined, item: "", kind: "", isIncome: false, amount: undefined, card: ""};
    }
    undo.value.registerUndo(() => { insertOneRow(row, entry); selectRowFromUndo(row); });
    methods.deleteRow(pageMonth.value, row);
  }
}
/*  ym 月のデータを追加 (undo対応)  */
function addNewPage(ym: number) {
  if (!data.value[ym]) {
    methods.insertPage(ym);
    undo.value.registerUndo(() => { removePage(ym) });
  }
}
/*  ym 月のデータを削除 (undo対応, ページが空のときのみ実行する)  */
function removePage(ym: number) {
  if (data.value[ym] && data.value[ym].length == 0) {
    methods.deletePage(ym);
    undo.value.registerUndo(() => { addNewPage(ym) });
  }
}
/*  現在の月を設定する (undo対応)  */
function changePageMonth(ym: number) {
  if (ym !== pageMonth.value) {
    let ym_old = pageMonth.value;
    /*  undoStack, redoStack の最後に積まれているエントリに changePageMonth() を追加する  */
    let actions = undo.value.undoStack[undo.value.undoStack.length - 1];
    if (actions) {
      actions.push(() => { changePageMonth(ym_old) });
    }
    actions = undo.value.redoStack[undo.value.redoStack.length - 1];
    if (actions) {
      actions.push(() => { changePageMonth(ym_old) });
    }
    /*  編集中であれば確定させる  */
    dataTable.value?.finalizeEditInput();
    /*  月の値を更新  */
    setPageMonth(ym);
    /*  未編集、行選択なし  */
    if (selectedRows.value !== undefined) {
      selectedRows.value.length = 0;
    }
  }
}

/*  「行追加」が有効になる条件（最終行を除いて１行以上が連続して選択されていること）  */
const canAddRows = computed(() => {
  if (selectedRows.value === undefined || selectedRows.value.length == 0) {
    return false;
  } else if (selectedRows.value.length == 1 && selectedRows.value[0] == pageRows.value - 1) {
    return false;
  } else {
    let minRow = selectedRows.value.reduce((a, b) => (a < b ? a : b));
    let maxRow = selectedRows.value.reduce((a, b) => (a > b ? a : b));
    return (maxRow - minRow + 1) == selectedRows.value.length;
  }
});
/*  「行削除」「コピー」が有効になる条件（選択されている行があり、それが最終行だけではないこと）  */
const canRemoveRows = computed(() => {
  if (selectedRows.value === undefined) {
    return false;
  } else if (selectedRows.value.length == 0) {
    return false;
  } else if (selectedRows.value.length == 1 && selectedRows.value[0] == pageRows.value - 1) {
    return false;
  } else {
    return true;
  }
})

/*  「ペースト」が有効になる条件（１行以上が連続して選択されていて、かつクリップボードにデータがあること） */
const canPasteRows = computed(() => {
  return (canAddRows.value && copiedRows.value.length > 0);
})

/*  行追加：現在選択されている行の位置に、選択されているのと同じ数の行を挿入  */
function addRows() {
  dataTable.value?.finalizeEditInput();
  if (selectedRows.value !== undefined && selectedRows.value.length > 0) {
    let first = selectedRows.value.reduce((a, b) => (a < b ? a : b));
    let last = selectedRows.value.reduce((a, b) => (a > b ? a : b));
    if (last == pageRows.value - 1) {
      last = last - 1;
    }
    for (let i = first; i <= last; i++) {
      insertOneRow(first, undefined);
    }
  }
}

/*  行削除またはコピー  */
function deleteOrCopySelectedRows(isCopy: boolean) {
  dataTable.value?.finalizeEditInput();
  if (pageData.value !== undefined && selectedRows.value !== undefined && selectedRows.value.length > 0) {
    /*  降順に並べ替える  */
    /*  注：引数なしの sort() は「文字列の順で」並べ替えるので注意！  */
    let rows = selectedRows.value.slice().sort((a, b) => b - a);
    /*  クリップボードをクリア  */
    copiedRows.value.length = 0;
    for (let i = 0; i < rows.length; i++) {
      let row = rows[i];
      if (row !== undefined) {
        let rowData = pageData.value[row];
        if (rowData) {
          let copiedData: DataEntry = {
            date: rowData.date,
            item: rowData.item,
            kind: rowData.kind,
            isIncome: rowData.isIncome,
            amount: rowData.amount,
            card: rowData.card
          }
          copiedRows.value.unshift(copiedData);
        }
        if (!isCopy) {
          deleteOneRow(row);
          selectedRows.value.length = 0;  /*  行選択をクリア  */
        }
      }
    }
  }
}

/*  行を選択位置にペースト  */
function pasteRows() {
  dataTable.value?.finalizeEditInput();
  if (selectedRows.value !== undefined && selectedRows.value.length == 1 && copiedRows.value.length > 0) {
    let row = selectedRows.value[0];
    if (row !== undefined) {
      for (let i = copiedRows.value.length - 1; i >= 0; i--) {
        /*  後ろの行から順に挿入  */
        insertOneRow(row, copiedRows.value[i]);
      }
      /*  新しく挿入した行を選択  */
      for (let j = 0; j < copiedRows.value.length; j++) {
        selectedRows.value.push(row + j);
      }
    }
  }
}

/*  ファイル読み込み  */
async function fileSelected() {
  if (fileInput.value !== undefined) {
    let file = fileInput.value.files?.[0];
    if (file !== undefined) {
      await methods.importCSV(file);
    }
  }
}

/*  表の高さ：印刷時は auto にする  */
const dataTableHeight = ref('');

/*  data URL にエンコードする  */
import leftTriangleURL from "../../src/assets/left-triangle.svg?inline";
import rightTriangleURL from "../../src/assets/right-triangle.svg?inline";

onMounted(() => {
  dataTableHeight.value = '314px';
  window.onbeforeprint = () => {
    dataTableHeight.value = 'auto';
    dataTable.value?.updateHeight(dataTableHeight.value);
  };
  window.onafterprint = () => {
    dataTableHeight.value = '314px';
    dataTable.value?.updateHeight(dataTableHeight.value);
  };
});
onUnmounted(() => {
  dataTable.value?.finalizeEditInput();
  window.onbeforeprint = null;
  window.onafterprint = null;
});

</script>

<template>
  <div id="button_box" ref="buttonBox">
    <IconButton :label="'行追加'" style="top:0px;left:0px;width:58px;height:38px;"
      :enabled="canAddRows" @click="addRows" />
    <IconButton :label="'行削除'" style="top:40px;left:0px;width:58px;height:38px;"
      :enabled="canRemoveRows" @click="deleteOrCopySelectedRows(false)" />
    <IconButton :label="'コピー'" style="top:80px;left:0px;width:58px;height:38px;"
      :enabled="canRemoveRows" @click="deleteOrCopySelectedRows(true)" />
    <IconButton :label="'貼り付け'" style="top:120px;left:0px;width:58px;height:38px;"
      :enabled="canPasteRows" @click="pasteRows" />
    <IconButton :label="'取り消し'" style="top:160px;left:0px;width:58px;height:38px;"
      :enabled="undo.canUndo()" @click="undo.doUndo()" />
    <IconButton :label="'やり直し'" style="top:200px;left:0px;width:58px;height:38px;"
      :enabled="undo.canRedo()" @click="undo.doRedo()" />
    <IconButton :label="'CSV\n読み込み'" style="top:260px;left:0px;width:58px;height:38px;"
      :enabled="true" @click="fileInput?.click()" />
    <input style="display:none;" ref="fileInput" type="file" @change="fileSelected()" />
    <IconButton :label="'CSV\n書き出し'" style="top:300px;left:0px;width:58px;height:38px;"
      :enabled="true" @click="methods.exportCSV()" />
  </div>
  <PrintButton />
  <div id="header_box">
    <IconButton :source="leftTriangleURL" style="top:3px;left:180px;"
      :enabled="lastMonthEnabled" @click="lastMonthEnabled && changePageMonth(lastMonth(pageMonth)) " />
    <select id="this_month" style="left:200px;width:160px" v-model="pageMonth">
      <option v-for="(ym, index) in pageMonthList" :key="ym" :value="ym" :disabled="ym < 0">
        {{ (ym > 0 ? yearMonthToString(ym) : "••••••") }}
      </option>
    </select>
    <IconButton :source="rightTriangleURL" style="top:3px;left:362px;"
      :enabled="nextMonthEnabled" @click="nextMonthEnabled && changePageMonth(nextMonth(pageMonth))" />
  </div>
  <div id="dataTable" style="margin-left:90px;margin-top:50px;">
    <DataTable :source="dataSource" ref="dataTable"
      :height="dataTableHeight" />
  </div>
  <div id="netTable" style="margin-left:239px;margin-top:18px">
    <DataTable :source="netTableSource" 
      :no-scroll="true" :height="90" />
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
@media print {
  #button_box { display: none; }
}
</style>
