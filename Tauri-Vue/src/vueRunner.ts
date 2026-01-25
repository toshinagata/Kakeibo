//import { homeDir, join } from "@tauri-apps/api/path";
//import { mkdir, exists, create, rename, remove, readTextFile, writeTextFile, readDir }
//  from "@tauri-apps/plugin-fs";
//import { save } from "@tauri-apps/plugin-dialog";

let vueRunnerId: string | null;

//  If id is specified in the document URL, then update vueRunnerId
let uri = decodeURIComponent(document.location.href);
let url = new URL(uri);
let id = url?.searchParams?.get("id");
if (id !== undefined) {
  vueRunnerId = id;
}

function fetchVueRunner(args: object) {
  if (vueRunnerId !== undefined) {
    args = {...args, id: vueRunnerId};
  }
  return fetch("/@vueRunner/", {
    method: "POST",
    body: JSON.stringify(args),
    headers: {
      "Content-Type": "application/json",
    }
  })
}

export async function vueRunner(): Promise<boolean> {
  const res = await fetchVueRunner({ cmd: "isAvailable" });
  if (res.ok) {
    return await res.text() === "ok";
  } else {
    return false;
  }
}

export async function vHomeDir(): Promise<string> {
  const res = await fetchVueRunner({ cmd: "homeDir" });
  if (res.ok) {
    return await res.text();
  } else {
    return "";
  }
}

export async function vJoin(dirPath: string, file: string): Promise<string> {
  const res = await fetchVueRunner({ cmd: "join", dirPath: dirPath, file: file });
  if (res.ok) {
    return await res.text();
  } else {
    return "";
  }
}

export async function vMkdir(path: string, options?: object): Promise<boolean> {
  const res = await fetchVueRunner({ cmd: "mkdir", path: path, options: options });
  if (res.ok) {
    return (await res.text() === "ok");
  } else {
    return false;
  }
}

export async function vExists(path: string): Promise<boolean> {
  const res = await fetchVueRunner({ cmd: "exists", path: path });
  if (res.ok) {
    return (await res.text() === "ok");
  } else {
    return false;
  }
}

export async function vCreate(path: string): Promise<boolean> {
  const res = await fetchVueRunner({ cmd: "create", path: path });
  if (res.ok) {
    return (await res.text() === "ok");
  } else {
    return false;
  }
}

export async function vRename(oldPath: string, newPath: string): Promise<boolean> {
  const res = await fetchVueRunner({ cmd: "rename", oldPath: oldPath, newPath: newPath });
  if (res.ok) {
    return (await res.text() === "ok");
  } else {
    return false;
  }
}

export async function vRemove(path: string): Promise<boolean> {
  const res = await fetchVueRunner({ cmd: "remove", path: path });
  if (res.ok) {
    return (await res.text() === "ok");
  } else {
    return false;
  }
}

export async function vReadTextFile(path: string): Promise<string> {
  const res = await fetchVueRunner({ cmd: "readTextFile", path: path });
  if (res.ok) {
    return await res.text();
  } else {
    return "";
  }
}

export async function vWriteTextFile(path: string, text: string): Promise<boolean> {
  const res = await fetchVueRunner({ cmd: "writeTextFile", path: path, text: text });
  if (res.ok) {
    return (await res.text() === "ok");
  } else {
    return false;
  }
}

export async function vReadDir(path: string): Promise<string[]> {
  const res = await fetchVueRunner({ cmd: "readDir", path: path });
  if (res.ok) {
    const text = await res.text();
    return await JSON.parse(text);
  } else {
    return [];
  }
}

export async function vSaveDialog(options?: object): Promise<string> {
  const res = await fetchVueRunner({ cmd: "saveDialog", options: options });
  //  res は event-stream
  const reader = res.body?.getReader();
  const decoder = new TextDecoder();
  return new Promise<string>(async (resolve, reject) => {
    if (reader !== undefined) {
      while (true) {
        const {done, value} = await reader.read();
        console.log(`vSaveDialog: done=${done}, value=${value}\n`);
        if (done)
          break;
        if (!value)
          continue;
        const lines = decoder.decode(value);
        let [ type, text ] = lines.split(": ");
        if (type === "data") {
          //  text may be an empty string (the save dialog was canceled)
          text = text.trim();
          console.log(`vSaveDialog: resolve(${text})\n`);
          resolve(text);
          return;
        }
      }
    }
    reject();
    return;
  });
}

export function vTerminate() {
  fetchVueRunner({ cmd: "terminate" });
}

export function vListenToServer(handler: (event: MessageEvent) => void): void {
  const url = document.location.origin + "/@vueRunner/event?id=" + vueRunnerId;
  const evtSource = new EventSource(url);
  evtSource.onmessage = handler;
}