import { nextTick } from "vue"
export class UndoManager {
  /*  undoスタック: 引数なし、戻り値なしの関数の配列の配列  */
  undoStack: (()=>void)[][];
  /*  redoスタック: 引数なし、戻り値なしの関数の配列の配列  */
  redoStack: (()=>void)[][];
  /*  undo実行中か？ 0:どちらでもない、1:undo中、2:redo中  */
  undoOrRedo: number = 0;
  /*  一時的な undo/redo スタック：nextTick 時に redoStack または undoStack に乗せる  */
  temporaryUndos: (()=>void)[];
  temporaryRedos: (()=>void)[];
  /*  コンストラクタ  */
  constructor() {
    this.undoStack = [];
    this.redoStack = [];
    this.temporaryUndos = [];
    this.temporaryRedos = [];
  }
  /*  undo動作を登録  */
  registerUndo(action: ()=>void) {
    if (this.temporaryUndos.length == 0 && this.temporaryRedos.length == 0) {
      let tempUndoOrRedo = this.undoOrRedo;  /* 一時変数にコピーしてアロー関数に渡す */
      nextTick(() => {
        if (tempUndoOrRedo == 1) {
          this.redoStack.push(this.temporaryRedos);
          this.temporaryRedos = [];
        } else {
          this.undoStack.push(this.temporaryUndos);
          this.temporaryUndos = [];
          if (tempUndoOrRedo == 0) {
            /* 通常の動作(undo中でもredo中でもない)なら redoStack をクリアする */
            this.redoStack.length = 0;
          }
        }
      });
    }
    if (this.undoOrRedo == 1) {
      this.temporaryRedos.push(action);
    } else {
      this.temporaryUndos.push(action);
    }
  }

  /*  undo, redo 時に事前・事後に実行する内容  */
  beforeUndoOrRedo: ((isUndoing: boolean) => void) | undefined = undefined;
  afterUndoOrRedo: ((isUndoing: boolean) => void) | undefined = undefined;

  /*  undo を１段階実行  */
  doUndo() {
    this.undoOrRedo = 1;
    let actions = this.undoStack.pop();
    if (actions) {
      if (this.beforeUndoOrRedo !== undefined) {
        this.beforeUndoOrRedo(true);
      }
      while (actions.length > 0) {
        let action = actions.pop();
        if (action) {
          action();
        }
      }
      if (this.afterUndoOrRedo !== undefined) {
        this.afterUndoOrRedo(true);
      }
    }
    this.undoOrRedo = 0;
  }

  /*  redo を１段階実行  */
  doRedo() {
    this.undoOrRedo = 2;
    let actions = this.redoStack.pop();
    if (actions) {
      if (this.beforeUndoOrRedo !== undefined) {
        this.beforeUndoOrRedo(false);
      }
      while (actions.length > 0) {
        let action = actions.pop();
        if (action) {
          action();
        }
      }
      if (this.afterUndoOrRedo !== undefined) {
        this.afterUndoOrRedo(false);
      }
    }
    this.undoOrRedo = 0;
  }

  /*  undo可能か？  */
  canUndo(): boolean {
    return (this.undoStack.length > 0);
  }

  /*  redo可能か？  */
  canRedo(): boolean {
    return (this.redoStack.length > 0);
  }
}
