export interface DataEntry {
  date: number | undefined;
  item: string;
  kind: string;
  isIncome: boolean;
  amount: number | undefined;
  card: string;
}

export type DataType = {
  [index: number]: DataEntry[];
}

export interface DataMethods {
  setValue(page: number, row: number, key: keyof DataEntry,
    value: string | number | boolean | undefined): void;
  insertRow(page: number, row: number,
    entry: DataEntry | undefined): void;
  deleteRow(page: number, row: number): void;
  insertPage(page: number): void;
  deletePage(page: number): void;
  importCSV(file: File): Promise<any>;
  exportCSV(): Promise<any>;
}

export interface CardEntry {
  name: string;
  closing: number;
}

export interface Settings {
  incomeKinds: string[];
  paymentKinds: string[];
  cards: CardEntry[];
}

export interface SettingsMethods {
  insertIncomeKind(kind: string, index: number): void;
  deleteIncomeKind(index: number): void;
  replaceIncomeKind(kind: string, index: number): void;
  moveIncomeKind(fromIndex: number, toIndex: number): void;
  isIncomeKindInUse(kind: string): boolean;
  insertPaymentKind(kind: string, index: number): void;
  deletePaymentKind(index: number): void;
  replacePaymentKind(kind: string, index: number): void;
  movePaymentKind(fromIndex: number, toIndex: number): void;
  isPaymentKindInUse(kind: string): boolean;
  insertCardEntry(name: string, closing: number, index: number): void;
  changeCardEntry(name: string | undefined, closing: number | undefined, index: number): void;
  deleteCardEntry(index: number): void;
  moveCardEntry(fromIndex: number, toIndex: number): void;
  isCardEntryInUse(name: string): boolean;
}
