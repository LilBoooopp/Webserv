var CURRENT_DIR = "";
var infoMenu = null;

function clearInfoMenu() {
  if (infoMenu) {
    for (const d of infoMenu.divs) d.remove();
    infoMenu.remove();
  }
  infoMenu = null;
}
function toggleInfoMenu() {
  infoMenu = writeBox(50, 50, 0, 0, "rgba(0,0,0,0)");
  infoMenu.divs = [];
  var spreadY = 20;
  for (const f of selFiles) {
    var p = [f.posX, f.posY + 80];
    var y = 0;
    infoMenu.divs.push(writeBox(100, 50, p[0] - 50, p[1] - 15, "rgba(0,0,0,.05)"));
    infoMenu.divs.push(addDiv(f.name, [p[0], p[1] + spreadY * y++], 1, "white"));
    infoMenu.divs.push(addDiv((f.size / 1024).toFixed(2) + " MB", [p[0], p[1] + spreadY * y++], 0.8, "white"));
  }
}

function toggleMenu(p, active = true) {
  if (!active) {
    clearOptMenu();
    return;
  }
  if (selFiles.length) {
    var options = ["View", "Data", "Duplicate", "Delete"];
    var functions = [inspectFiles, toggleInfoMenu, duplicate, deleteFiles];
    var infos = [`GET /ressources/uploads/${selFiles[0].path} HTTP/1.1`, "ToggleInfoMenu", `POST /cgi-bin/upload.py HTTP/1.1`, `DELETE ${selFiles[0].path} HTTP/1.1`];
    if (selFiles[0].isDir) {
      for (let x = 0; x < 3; x++) {
        options.pop();
        functions.pop();
        infos.pop();
      }
      infos[0] = "GET /cgi-bin/gdrive.php?dir=... " + selFiles[0].path + " HTTP/1.1";
    }
    initOptMenu([p[0] - 100, p[1]], options, functions, infos);
  } else initOptMenu(p, ["Upload", "Repo"], [() => openFileDialog(uploadFiles), uploadDirectory], ["OpenFileDialog", "uploadDirectory"]);
}

var selBoxStart = null;
var selBox = null;
var selBoxRect = null;
var selBoxBase = [];
var gFiles = [];
var keys = {};
window.addEventListener("keydown", (e) => {
  const k = e.key.toLowerCase();
  keys[k] = true;
  if (e.code === "Backspace" || k === "x") {
    deleteFiles();
  }
});

window.addEventListener("keyup", (e) => {
  keys[e.key.toLowerCase()] = false;
});

window.addEventListener("contextmenu", (e) => {
  e.preventDefault();
  if (hovFile && !selFiles.includes(hovFile)) {
    if (!keys["shift"]) clearSelFiles();
    hovFile.iconBgr.style.display = "block";
    selFiles.push(hovFile);
  }
  toggleMenu([e.clientX, e.clientY], true);
});

window.addEventListener("mousedown", (e) => {
  if (!optMenu) clearInfoMenu();

  clearOptMenu();
  if (e.button !== 0) return;
  if (!hovFile && !keys["shift"]) clearSelFiles();
  else if (hovFile) {
    playClick();
    if (selFiles.includes(hovFile) && !keys["shift"]) {
      if (hovFile.extension === "folder" || hovFile.isDir) window.location.href = "/cgi-bin/gdrive.php?dir=" + encodeURIComponent(hovFile.relativePath);
      else window.location.href = "/ressources/uploads/" + hovFile.relativePath;
      return;
    }
    if (!keys["shift"] && !keys["meta"]) clearSelFiles();
    selFiles.push(hovFile);
    hovFile.iconBgr.style.display = "block";
  }
  if (!hovFile) {
    selBoxStart = [e.clientX, e.clientY];
    selBoxBase = keys["shift"] ? selFiles.slice() : [];
  }
});

window.addEventListener("mousemove", (e) => {
  if (!selBoxStart) return;
  var x = Math.min(e.clientX, selBoxStart[0]);
  var y = Math.min(e.clientY, selBoxStart[1]);
  var w = Math.abs(e.clientX - selBoxStart[0]);
  var h = Math.abs(e.clientY - selBoxStart[1]);
  selBoxRect = { x, y, w, h };
  if (selBox) selBox.remove();
  selBox = writeBox(w, h, x, y, window.DARKMODE ? "rgba(255, 255, 255, 0.05)" : "rgba(0, 0, 0, 0.05)");
  const newSel = selBoxBase.slice();
  for (const f of gFiles) {
    const cx = f.posX;
    const cy = f.posY;
    const fw = 50 / 2;
    if (cx >= x - fw && cx <= x + fw + w && cy >= y - fw && cy <= y + h + fw) {
      if (!newSel.includes(f)) {
        if (!selFiles.includes(f)) playClick();
        newSel.push(f);
      }
    }
  }
  selFiles = newSel;
  for (const f of gFiles) {
    if (selFiles.includes(f) && f.iconBgr) f.iconBgr.style.display = "block";
    else if (f.iconBgr) f.iconBgr.style.display = "none";
  }
});

window.addEventListener("mouseup", (e) => {
  if (!selBoxStart) return;
  selBoxStart = null;
  selBoxRect = null;
  selBoxBase = [];
  if (selBox) selBox.remove();
  selBox = null;
});

var hovFile = null;
var selFiles = [];
var optMenu = null;

function clearSelFiles() {
  for (const f of selFiles) {
    f.iconBgr.style.display = "none";
  }
  selFiles = [];
}

function clearOptMenu() {
  if (!optMenu) return;
  for (const b of optMenu.buttons) {
    if (b.line) b.line.remove();
    b.remove();
  }
  optMenu.remove();
  optMenu = null;
}

function initOptMenu(p, options, effects, infos = null) {
  clearOptMenu();
  optMenu = writeBox(80, options.length * 30, p[0] - 40, p[1] - 15, "rgba(204, 154, 206, 1)");
  optMenu.style.borderRadius = "20%";
  optMenu.buttons = [];
  for (let i = 0; i < options.length; i++) {
    var b = addButton(options[i], p, effects[i] || (() => {}), null, "rgba(0,0,0,0)", infos ? infos[i] : null);
    b.style.height = "20px";
    b.line = null;
    p[1] += 30;
    if (i < options.length - 1) b.line = writeBox(80, 2, p[0] - 40, p[1] - 15, "rgba(0, 0, 0, 0.05)");
    optMenu.buttons.push(b);
  }
}

function handleGDrive(files) {
  // currentDir is passed from PHP; default empty string
  CURRENT_DIR = arguments.length > 1 ? arguments[1] || "" : "";
  gFiles = files;
  window.PAGE_NAME = "GDRIVE";
  addTitle();
  addHomeButton();
  if (CURRENT_DIR !== "") addBackButton([window.innerWidth / 2, window.innerHeight / 2 - 80]);
  var filePath = "/ressources/img/fileIcons/";
  var extensions = ["file", "folder", "mp3", "pdf", "png", "txt", "wav"];
  var imageData = {};
  extensions.forEach((ext) => {
    var img = new Image();
    img.src = `${filePath}${ext}.png`;
    imageData[ext] = img;
  });

  const cols = 12;
  const cellWidth = window.innerWidth / cols;
  const cellHeight = 150;
  const startY = 400;
  const startX = cellWidth / 2;
  const totalRows = Math.ceil(files.length / cols);

  function layoutFiles() {
    const cols = 12;
    const cellWidth = window.innerWidth / cols;
    const cellHeight = 150;
    const startY = 400;
    const startX = cellWidth / 2;

    for (const f of gFiles) {
      if (f.labelDiv) {
        f.labelDiv.remove();
        f.labelDiv = null;
      }
      if (f.iconBgr) {
        f.iconBgr.remove();
        f.iconBgr = null;
      }
      if (f.iconImg) {
        f.iconImg.remove();
        f.iconImg = null;
      }
    }

    gFiles.forEach((file, i) => {
      const row = Math.floor(i / cols);
      const rowStartIndex = row * cols;
      const itemsInRow = Math.min(cols, gFiles.length - rowStartIndex);
      const colInRow = i - rowStartIndex;
      const offsetCol = Math.floor((cols - itemsInRow) / 2);
      var x = startX + (offsetCol + colInRow) * cellWidth;
      const y = startY + row * cellHeight;

      if (files.length % 2 !== 0) x += cellWidth / 2;
      file.posX = x;
      file.posY = y;

      const label = `${file.name}`;
      const labelDiv = addDiv(label, [x - cellWidth * 0.45, y + 40]);
      labelDiv.style.textAlign = "center";
      labelDiv.style.width = cellWidth * 0.9 + "px";
      labelDiv.style.whiteSpace = "normal";
      labelDiv.style.wordWrap = "break-word";
      labelDiv.style.fontSize = "12px";
      file.labelDiv = labelDiv;

      const iconBgr = writeBox(cellWidth * 0.8, cellHeight * 0.5, x - cellWidth * 0.4, y - 40, "white");
      iconBgr.style.opacity = ".04";
      iconBgr.style.borderRadius = "20%";
      iconBgr.style.display = selFiles.includes(file) ? "block" : "none";
      file.iconBgr = iconBgr;

      const iconExt = imageData[file.extension] ? file.extension : "file";
      const iconImg = imageData[iconExt].cloneNode(true);
      iconImg.style.userSelect = "none";
      file.iconExt = iconExt;
      file.iconImg = iconImg;
      iconImg.addEventListener("mouseenter", () => {
        hovFile = file;
        iconImg.style.scale = 1.2;
      });
      iconImg.addEventListener("mouseleave", () => {
        hovFile = null;
        iconImg.style.scale = 1;
      });
      iconImg.style.width = "40px";
      iconImg.style.height = "40px";
      iconImg.style.position = "absolute";
      iconImg.style.left = x - 20 + "px";
      iconImg.style.top = y - 20 + "px";
      document.body.appendChild(iconImg);
    });
  }

  // Initial layout
  layoutFiles();

  // Re-render layout on window resize
  window.addEventListener("resize", () => {
    layoutFiles();
  });
}

function inspectFiles() {
  for (let i = 0; i < selFiles.length; i++) {
    const f = selFiles[i];
    if (f.extension === "folder" || f.isDir) {
      window.location.href = "/cgi-bin/gdrive.php?dir=" + encodeURIComponent(f.relativePath);
    } else {
      window.location.href = "/ressources/uploads/" + f.relativePath;
    }
  }
}

function deleteFiles() {
  //   const filesToDelete = selFiles.filter((f) => f.extension !== "folder" && !f.isDir);
  const filesToDelete = selFiles;
  if (filesToDelete.length === 0) {
    selFiles = [];
    return;
  }

  let completed = 0;
  for (const f of filesToDelete) {
    fetch(f.path, {
      method: "DELETE",
    })
      .then((res) => {
        if (res.ok) {
          completed++;
          if (completed === filesToDelete.length) window.location.reload();
        } else announce("Delete failed: " + res.statusText);
      })
      .catch((err) => announce("Delete failed: " + err));
  }
  selFiles = [];
}

function duplicate() {
  for (let i = 0; i < selFiles.length; i++) {
    const file = selFiles[i];
    const name = file.name;
    const dot = name.lastIndexOf(".");
    if (dot <= 0) {
      announce("Can't upload " + name + " - not a file");
      continue;
    }
    const ext = name.slice(dot + 1).toLowerCase();
    const start = performance.now();

    const targetPath = (CURRENT_DIR ? CURRENT_DIR + "/" : "") + name;
    fetch("/cgi-bin/upload.py", {
      method: "POST",
      headers: {
        "Content-Type": file.type || "application/octet-stream",
        "X-Upload-Path": targetPath,
        "X-mode": "Create",
      },
      body: file,
    })
      .then((r) => r.text())
      .then((t) => {
        window.location.reload();
      })
      .catch((err) => announce("Upload failed: " + err));
  }
}

function uploadDirectory() {
  // Prompt for directory name
  var dirName = prompt("Directory name:");
  if (!dirName) return;

  var path = (CURRENT_DIR ? CURRENT_DIR + "/" : "") + dirName + "/";
  fetch("/cgi-bin/upload.py", {
    method: "POST",
    headers: {
      "X-Upload-Path": path,
    },
  })
    .then((r) => r.text())
    .then((t) => {
      window.location.reload();
    })
    .catch((err) => announce("Upload failed: " + err));
}

function postFiles(files) {
  for (let i = 0; i < files.length; i++) {
    const file = files[i];
    const name = file.name;
    const dot = name.lastIndexOf(".");
    if (dot <= 0) {
      announce("Can't post " + name + " - not a file");
      continue;
    }

    fetch(file.path, {
      method: "POST",
      headers: {
        "Content-Type": file.type || "application/octet-stream",
      },
      body: file,
    })
      .then((r) => {
        if (r.ok) window.location.reload();
        else announce("Post failed: " + res.statusText);
      })
      .catch((err) => announce("Post failed: " + err));
  }
}

function uploadFiles(files) {
  for (let i = 0; i < files.length; i++) {
    const file = files[i];
    const name = file.name;
    const dot = name.lastIndexOf(".");
    if (dot <= 0) {
      announce("Can't upload " + name + " - not a file");
      continue;
    }
    const ext = name.slice(dot + 1).toLowerCase();
    const start = performance.now();

    const targetPath = (CURRENT_DIR ? CURRENT_DIR + "/" : "") + name;
    fetch("/cgi-bin/upload.py", {
      method: "POST",
      headers: {
        "Content-Type": file.type || "application/octet-stream",
        "X-Upload-Path": targetPath,
      },
      body: file,
    })
      .then((r) => r.text())
      .then((t) => {
        window.location.reload();
      })
      .catch((err) => announce("Upload failed: " + err));
  }
}
