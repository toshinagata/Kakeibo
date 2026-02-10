<script setup lang="ts">
import { ref, computed, onMounted, onUpdated, nextTick } from 'vue'

/*  row, column は 0 から始まる  */
export interface DataTableSource {
  numberOfColumns(): number;             /* 列の数 */
  titleHeight?(): number;                 /* タイトル行の高さ：0 なら表示しない  */
  columnName(column: number): string;    /* 列のタイトル */
  columnWidth(column: number): number;   /* 列の幅 */
  columnType(column: number): string;    /* 列を編集するタイプ。"input", "select" など。編集できない列は "" を返す  */
  selectItems(column: number): any[];    /* select の子要素。単純な select の場合は文字列の配列、optgroup の場合は { opt:string, items:string[] } の配列  */
  numberOfRows(): number;                /* 行の数 */
  rowHeight?(): number;                   /* 行の高さ */
  valueAt(row: number, column: number): string;  /* あるセルの値 */
  optionalStyleAt?(row: number, column: number): string;  /* あるセルのスタイル */
  setValueAt(value: string, row: number, column: number): void; /* セルの値を変更 */
  isEditable(row: number, column: number): boolean;  /*  編集可能かどうか  */
  isSelectable(row: number): boolean;  /* 選択可能かどうか */
  finalized?(row: number, column: number): void;     /* 編集が終了した時に呼ばれる */
  data: any;  /*  データ本体  */
}

/*  props 定義  */
const props = defineProps<{
  source: DataTableSource;
  height?: number | string;
  noScroll?: boolean;
}>();
/*  タイトル行、データ行の高さ  */
const titleHeight = computed(() => props.source.titleHeight ? props.source.titleHeight() : 18);
const rowHeight = computed(() => props.source.rowHeight ? props.source.rowHeight() : 18);

/*  table へのレファレンス  */
const tableRef = ref<HTMLElement>();
const tableBox = ref<HTMLElement>();

/*  レンダリングするごとに行位置・カラム位置を記憶  */
let columnPositions: number[] = [];
let rowPositions: number[] = [];
function updateRowColumnPositions() {
  if (tableRef.value !== undefined) {
    let tableRect = tableRef.value.getBoundingClientRect();
    for (let column = 0; column < props.source.numberOfColumns(); column++) {
      let td = tableRef.value.querySelector("#r1c" + String(column+1));
      let rect = td?.getBoundingClientRect();
      if (rect !== undefined) {
        columnPositions[column] = rect.x - tableRect.x;
        /* 「次の列」がない場合に備えて特定しておく */
        columnPositions[column + 1] = rect.x + rect.width + 1 - tableRect.x;
      }
    }
    for (let row = 0; row < props.source.numberOfRows() + 1; row++) {
      let td = tableRef.value.querySelector("#r" + String(row+1) + "c1");
      let rect = td?.getBoundingClientRect();
      if (rect !== undefined) {
        rowPositions[row] = rect.y - tableRect.y;
        /* 「次の行」がない場合に備えて特定しておく */
        rowPositions[row + 1] = rect.y + rect.height + 1 - tableRect.y;
      }
    }
  }
}
onMounted(updateRowColumnPositions);
onUpdated(updateRowColumnPositions);

/*  表の幅を自動計算（カラムの間隔 5px とする）  */
function tableWidth() {
  let w = 0;
  for (let column = 0; column < props.source.numberOfColumns(); column++) {
    w += props.source.columnWidth(column) + 5;
  }
  return w;
}

/*  カラム位置をスタイル文字列に変換  */
function columnPosStyle(column: number) {
  return "left:" + String(columnPositions[column]) + "px;width:" + String(props.source.columnWidth(column)) + "px;";
}

/*  行位置をスタイル文字列に変換  */
function rowPosStyle(row: number) {
  return "top:" + String(rowPositions[row]) + "px;";
}

/*  編集中の行 (0から始まる)。編集中でなければ -1  */
const editRow = ref(-1);
/*  編集中の列 (0から始まる）。編集中でなければ無視される */
const editColumn = ref(-1);
/*  編集中の値（文字列） */
const editText = ref("");
/*  編集中のinputなどの要素  */
const editInput = ref();
/*  選択されている行（昇順とは限らない）  */
const selectedRows = ref<number[]>([]);

/*  編集がリクエストされているかどうか  */
/* （重複して nextTick を発行することを避ける） */
let editRequested = false;

/*  編集をリクエストする。row, column のいずれかが -1 なら編集リクエストをキャンセルする */
function requestEdit(row: number, column: number) {
  editRow.value = row;
  editColumn.value = column;
  if (!editRequested) {
    editRequested = true;
    nextTick(() => {
      if (editRow.value >= 0 && editColumn.value >= 0) {
        editText.value = (props.source.valueAt(editRow.value, editColumn.value));
        editInput.value.focus();
        if (props.source.columnType(column) == "input") {
          editInput.value.select();
        }
      }
      editRequested = false;
    });
  }
}

/*  エキスポート定義  */
defineExpose({
  editRow,
  editColumn,
  selectedRows,
  finalizeEditInput,
  requestEdit,
  updateHeight
});

function rowClass(row:number) {
  if (selectedRows.value.indexOf(row) >= 0) {
    return "selected ";
  } else {
    return "";
  }
}

/*  finalizeEditInput() の再帰呼び出しを避けるためのフラグ  */
let isFinalizing = false;

/*  編集中のinput要素があれば確定させる  */
function finalizeEditInput() {
  if (isFinalizing) {
    return;
  }
  if (editRow.value >= 0 && editInput.value) {
    if (true || editInput.value.tagName === "INPUT") {
      isFinalizing = true;
      editInput.value.blur();  /*  フォーカスを外す  */
      let value = editInput.value.value;
      props.source.setValueAt(value, editRow.value, editColumn.value);
      isFinalizing = false;
      //props.source.finalized?.();
    }
  }
  editRow.value = -1;
}

/*  command+クリックでそのセルをトグル  */
/*  shift+クリックで最後の選択からクリックした行までをすべて選択 */
function onClickNonEditableCell(event: MouseEvent, row: number) {
  editRow.value = -1;
  if (event.shiftKey) {
    if (selectedRows.value.length <= 0) {
      if (props.source.isSelectable(row)) {
        selectedRows.value.push(row);
      }
    } else {
      let last = selectedRows.value[selectedRows.value.length - 1] ?? 0; /* ?? 0 はダミー */
      if (row < last) {
        last = -last;
        row = -row;
      }
      for (let i = last; i <= row; i++) {
        let j = (i >= 0 ? i : -i);
        if (selectedRows.value.indexOf(j) < 0 && props.source.isSelectable(j)) {
          selectedRows.value.push(j);
        }
      }
    }
  } else if (event.metaKey) {
    let i = selectedRows.value.indexOf(row);
    if (i >= 0) {
      selectedRows.value.splice(i, 1);
    } else if (props.source.isSelectable(row)) {
      selectedRows.value.push(row);
    }
  } else {
    selectedRows.value.length = 0;
    if (props.source.isSelectable(row)) {
      selectedRows.value.push(row);
    }
  }
}

/*  テーブルセルをクリックした時の動作  */
function onClickCell(event: MouseEvent | undefined, row: number, column: number) {
  if (!props.source.isEditable(row, column)) {
    if (event) {
      onClickNonEditableCell(event, row);
    }
  } else {
    selectedRows.value.length = 0;
    if (props.source.isSelectable(row)) {
      selectedRows.value.push(row);
    }
    requestEdit(row, column);
  }
}

/* ヘッダーセルをクリックした時の動作：全選択／無選択のトグル  */
function onClickHeaderCell(_event: MouseEvent | undefined, _column: number) {
  if (editRow.value >= 0 && editColumn.value >= 0) {
    /*  編集中なら何もしない */
    return;
  } else {
    let nrows = props.source.numberOfRows();
    let selectable = 0;  /* 選択可能だがまだ選択されていない行の数 */
    let rowsToSelect = [];
    for (let r = 0; r < nrows; r++) {
      if (props.source.isSelectable(r)) {
        rowsToSelect.push(r);
        if (selectedRows.value.indexOf(r) < 0) {
          selectable += 1;
        }
      }
    }
    if (selectable == 0) {
      /* 無選択 */
      selectedRows.value.length = 0;
    } else {
      /* 全選択 */
      selectedRows.value = rowsToSelect;
    }
  }
}

/*  input要素で enter キーを押した時の動作：編集終了  */
/*  注意：keydown.enter は日本語入力中の確定enterも拾ってしまう。keypress.enter なら確定enterは拾わない */
/*  Cf: https://r17n.page/2020/04/04/vue-submit-on-enter-japanese/   */
function onKeydownEnter(_event: KeyboardEvent) {
  finalizeEditInput();
}
/*  input要素で esc キーを押した時の動作。編集中の値を確定して、編集モードを抜ける  */
function onKeydownEsc(_event: KeyboardEvent) {
  finalizeEditInput();
}

/*  input要素で tab キーを押した時の動作  */
function onKeydownTab(event: KeyboardEvent) {
  let row = editRow.value;
  let column = editColumn.value;
  finalizeEditInput();
  if (editRequested) {
    /* finalizeEditInput() 中で次の編集が指示されていたらそれを尊重 */
    return;
  }
  if (event.shiftKey) {
    /*  １つ前の要素に戻る  */
    while (true) {
      if (column == 0) {
        if (row == 0) {
          break;  /*  これ以上前には行けない  */
        } else {
          row -= 1;
          column = props.source.numberOfColumns() - 1;
        }
      } else {
        column -= 1;
      }
      if (props.source.isEditable(row, column)) {
        break;
      }
    }
  } else {
    /*  １つ先の要素に進む  */
    while (true) {
      if (column == props.source.numberOfColumns() - 1) {
        if (row == props.source.numberOfRows() - 1) {
          break;  /*  これ以上先には行けない  */
        } else {
          row += 1;
          column = 0;
        }
      } else {
        column += 1;
      }
      if (props.source.isEditable(row, column)) {
        break;
      }
    }
  }
  if (row != editRow.value || column != editColumn.value) {
    requestEdit(row, column);
  }
}

const tableBoxStyle = computed(() => {
  let s = 'width:' + String(tableWidth() + 18) + 'px;';
  if (props.noScroll) {
    s += 'overflow-y:hidden;background-color:transparent;';
  }
  if (props.height !== undefined && props.height !== '') {
    if (Number.isFinite(props.height)) {
      s += 'height:' + String(props.height) + 'px;';
    } else {
      s += 'height:' + String(props.height) + ';';
    }
  }
  return s;
});

function updateHeight(height: string) {
  if (tableBox.value !== undefined) {
    tableBox.value.style.height = height;
  }
}
</script>

<template>
  <div id="table_box" ref="tableBox"
    :style="tableBoxStyle">
    <table ref="tableRef" :style="'width:'+String(tableWidth())+'px;'">
      <thead v-if="titleHeight > 0">
        <tr>
          <th v-for="i in props.source.numberOfColumns()" :key="i" class="sticky"
            :style="'height:'+String(titleHeight)+'px;'"
            :width="props.source.columnWidth(i-1)"
            @click.stop="onClickHeaderCell($event, i-1)">
            {{ props.source.columnName(i-1) }}
          </th>
        </tr>
      </thead>
      <tbody>
        <tr v-for="i in props.source.numberOfRows()" :key="i" :class="rowClass(i-1)" >
          <td v-for="j in props.source.numberOfColumns()" :key="j"
            :id="'r'+String(i)+'c'+String(j)"
            :style="'height:'+String(rowHeight)+'px;'+(props.source.optionalStyleAt?.(i-1, j-1)||'')"
            :width="props.source.columnWidth(j-1)"
            @click.stop="onClickCell($event, i-1, j-1)">
            {{  props.source.valueAt(i-1, j-1) }}
          </td>
        </tr>
      </tbody>
    </table>
    <div class="table_input" v-if="editRow >= 0 && editColumn >= 0"
      :style="rowPosStyle(editRow)">
      <div v-if="props.source.columnType(editColumn) === 'input'">
        <input :style="columnPosStyle(editColumn)"
          v-model="editText" ref="editInput"
          @keypress.enter.prevent = "onKeydownEnter"
          @keydown.esc.prevent="onKeydownEsc"
          @keydown.tab.prevent="onKeydownTab"
          @focusout="finalizeEditInput" />
      </div>
      <div v-if="props.source.columnType(editColumn) === 'select'">
        <select :style="columnPosStyle(editColumn)"
          v-model="editText" ref="editInput" tabindex="1"
          @keypress.enter.prevent="onKeydownEnter"
          @keydown.esc.prevent="onKeydownEsc"
          @keydown.tab.prevent="onKeydownTab"
          @focusout="finalizeEditInput" >
          <template v-if="typeof props.source.selectItems(editColumn)[0] === 'string'">
            <option v-for="(option,index) in props.source.selectItems(editColumn)"
              :key="index" :value="option">
              {{ option }}
            </option>
          </template>
          <template v-else>
            <optgroup v-for="(option,index) in props.source.selectItems(editColumn)"
              :key="index" :label="option.opt">
              <option v-for="(item,index2) in option.items" :key="index*100+Number(index2)" :value="item">
                {{ item }}
              </option>
            </optgroup>
          </template>
        </select>
      </div>
    </div>
  </div>
</template>

<style scoped>
#table_box {
  background-color: white;
  position: relative;
  /*border: 2px solid #dea6f5;*/
  overflow-y: scroll;
  overflow-x: hidden;
  -webkit-overflow-scrolling: touch;
}
table {
  border-collapse: collapse;
  border-spacing: 0;
  /* width: 565px; */
  table-layout: fixed;
  background-color: white;
}
tr.selected {
  background-color: #88ffff;
}
th, td {
  vertical-align: middle;
  padding: 2px;
  border: 1px solid #000;
  color: #000;
  font-size: 14px;
  height: 18px;
  text-align: center;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  user-select: none;
  line-height: 1;
}
.sticky {
  position: sticky;
  top: 0;
  left: 0;
  border-top: none;
}
.sticky::before {
  content: "";
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  border-top: 1px solid #000;
  border-bottom: 1px solid #000;
  background: #ccc;
  z-index: -1;
}
.table_input {
  position: absolute;
  left: 0px;
  top: 26px;
}
.table_input input {
  position: absolute;
  top: 0px;
  height: 18px;
  width: 68px;
  background: #ffffcc;
  font-size: 14px;
}
.table_input select {
  position: absolute;
  top: 0px;
  height: 24px;
  width: 68px;
  background: #ffffcc;
  font-size: 14px;
}
.table_header {
  position: absolute;
  display: table;
  top: -1px;
  height: 34px;
  border: 1px solid #dea6f5;
  background-color: #ffffaa;
  text-align: center;
}
.header_content {
  display: table-cell;
  vertical-align:middle;
}
.table_row {
  position: absolute;
  left: 0px;
  width: 488px;
  height: 20px;
  text-align: center;
}
.table_cell {
  position: absolute;
  display: table;
  top: 0px;
  height: 19px;
  border: 1px solid #dea6f5;
}
.table_cell.odd {
  background-color: #ffffee;
}
.table_cell.dark {
  background-color: #eee;
}

</style>