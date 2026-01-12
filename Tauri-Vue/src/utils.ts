import type { DataEntry, DataType} from "./types.ts"

import { isTauri } from "@tauri-apps/api/core";
import { vueRunner } from "./vueRunner.ts";

/*  Tauri上で動いているかどうか：結果を windows.tauriIsAvailable にキャッシュ  */
export function isTauriAvailable() {
  if ((window as any).tauriIsAvailable === undefined) {
    const b = ((isTauri && isTauri()) ? true : false);
    (window as any).tauriIsAvailable = b;
    console.log("window.tauriIsAvailable is set to " + b);
  }
  return ((window as any).tauriIsAvailable);
}

/*  VueRunner上で動いているかどうか：結果を windows.vueRunnerIsAvailable にキャッシュ  */
export async function isVueRunnerAvailable() {
  if ((window as any).vueRunnerIsAvailable === undefined) {
    const b = ((vueRunner && await vueRunner()) ? true : false);
    (window as any).vueRunnerIsAvailable = b;
    console.log("window.vueRunnerIsAvailable is set to " + b);
  }
  return ((window as any).vueRunnerIsAvailable);
}

/*  アラートダイアログ  */
function myDialogAsync(mes: string, type: string, options?: any) {
  /*  dialog 要素を作成して main_window に挿入  */
  const dialog = document.createElement("dialog");
  dialog.innerHTML = `<div class="dialog-msg">
    </div>
    <div class="dialog-input">
      <input class="dialog-text" type="text" width="80" />
      <div class="dialog-comment"></div>
    </div>
    <div class="dialog-footer">
      <button type="button" class="cancelButton">キャンセル</button>
      <button type="button" class="okButton">OK</button>
    </div>
  `;
  if (options === undefined) {
    options = {};
  }
  const msg = dialog.getElementsByClassName("dialog-msg")[0] as HTMLDivElement;
  msg.innerText = mes;
  msg.style.cssText = `font-size: 14px; padding: 1rem 2rem; white-space: pre-wrap;`;
  const inputDiv = dialog.getElementsByClassName("dialog-input")[0] as HTMLDivElement;
  const input = inputDiv.getElementsByClassName("dialog-text")[0] as HTMLInputElement;
  inputDiv.style.cssText = `font-size: 14px; margin: 0 2rem;`;
  const comment = inputDiv.getElementsByClassName("dialog-comment")[0] as HTMLDivElement;
  input.style.cssText = `font-size: 14px;`;
  comment.style.cssText = `font-size: 11px; color: red;`
  const footer = dialog.getElementsByClassName("dialog-footer")[0] as HTMLDivElement;
  footer.style.cssText = `padding: 1rem 2rem;`;
  const cancelButton = footer.children[0] as HTMLButtonElement;
  const okButton = footer.children[1] as HTMLButtonElement;
  cancelButton.style.cssText = `width: 7em; margin: 0 1em`;
  okButton.style.cssText = `width: 7em; margin: 0 1em`;
  if (type === "alert") {
    cancelButton.remove();
  }
  if (type === "ask") {
    if (options.validator !== undefined) {
      input.oninput = (_event) => {
        const err = options.validator(input.value);
        comment.innerText = err;
      }
    } else {
      comment.remove();
    }
  } else {
    inputDiv.remove();
  }
  const mainWindow = document.getElementsByClassName("main_window")[0] as HTMLElement;
  mainWindow.appendChild(dialog);
  const focusButton = (options.danger && type === "confirm" ? cancelButton : okButton);
  return new Promise<string>((resolve, _reject) => {
    cancelButton.onclick = (_event) => {
      if (type === "ask" && comment.innerText !== "") {
        return;
      }
      dialog.close();
      dialog.remove();
      if (type === "ask") {
        resolve("");
      } else {
        resolve("cancel");
      }
    };
    okButton.onclick = (_event) => {
      dialog.close();
      dialog.remove();
      if (type === "ask") {
        resolve(input.value);
      } else {
        resolve("ok");
      }
    };
    dialog.onclose = (_event) => resolve(dialog.returnValue || "");
    dialog.showModal();
    focusButton.focus();
  });
}

export function myAskAsync(mes: string, validator?: (str: string) => string ) {
  return myDialogAsync(mes, "ask", { validator: validator });
}

export function myAlertAsync(mes: string) {
  return myDialogAsync(mes, "alert");
}

export function myConfirmAsync(mes: string, danger?: boolean) {
  return myDialogAsync(mes, "confirm", { danger: danger });
}

/*  年月を「〜年〜月」という文字列に変換  */
export function yearMonthToString(ym: string | number): string {
  ym = Number(ym);
  return `${Math.floor(ym / 100)}年${ym % 100}月`;
}

/*  次の月  */
export function nextMonth(ym: number): number {
  return (ym % 100 == 12 ? ym + 89 : ym + 1);
}
/*  前の月  */
export function lastMonth(ym: number): number {
  return (ym % 100 == 1 ? ym - 89 : ym - 1);
}

/*  データ中の最初の月、最後の月  */
export function firstMonthInData(data: DataType): number | undefined {
  let keys = Object.keys(data);
  if (keys !== undefined && keys.length > 0) {
    return Number(keys.reduce((a, b) => (a < b ? a : b)));
  } else {
    return undefined;
  }
}

export function endMonthInData(data: DataType): number | undefined {
  let keys = Object.keys(data);
  if (keys !== undefined && keys.length > 0) {
    return Number(keys.reduce((a, b) => (a > b ? a : b)));
  } else {
    return undefined;
  }
}

/*  金額を３桁ごとにコンマで区切る  */
export function formatAmount(amount: number): string {
  let sign = "";
  if (amount < 0) {
    sign = "-";
    amount = -amount;
  }
  let s = String(amount);
  let r = "";
  while (s.length > 3) {
    r = "," + s.slice(-3) + r;
    s = s.slice(0, -3);
  }
  return sign + s + r;
}

/*  入力された数値を正しい数値形式に変換（コンマを削除し、全角を半角に変換） */
export function numberFromString(str: string): number {
  str = str.replace(/、，,/g, "");
  str = str.replace(/ー/g, "-");
  str = str.replace(/[Ａ-Ｚａ-ｚ０-９]/g,
    (s) => String.fromCharCode(s.charCodeAt(0) - 0xFEE0));
  return Number(str);
}
