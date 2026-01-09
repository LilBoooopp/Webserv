var revealD = addDiv("toReveal: ", [window.innerWidth * 0.25, window.innerHeight * 0.6]);
// revealD.style.display = "none";
var cells;
var gameOver = false;
const colors = ["blue", "green", "orange", "red", "purple"];
var curLevel = 0;
var maxLevel = window.MAX_LEVEL ?? 1;
var k = "";
var toReveal = 0;
var hasStarted = false;
var cellW;
var cellH;

function toggleFlagMode() {
  clickFlag = !clickFlag;
  flagIcon.style.backgroundImage = "url(/ressources/img/mineSweeperIcons/" + (clickFlag ? "flag.png" : "flagOff.png") + ")";
}
var clickFlag = false;
var flagIcon = initIcon("/ressources/img/mineSweeperIcons/flagOff.png", [100, 100], [40, 40], "", 30, () => toggleFlagMode());

var levelsBtns = null;
updateLvlButtons();
function updateLvlButtons(amount = 11) {
  if (!levelsBtns) {
    levelsBtns = [];
    var spr = 35;
    for (let i = 0; i < amount; i++) {
      const idx = i + 1;
      var btn = addButton(idx, [window.innerWidth / 2 - 4.5 * spr + i * spr, window.innerHeight * 0.2], () => {
        if (idx <= maxLevel) initMinerGame(idx);
        else announce("Level " + idx + " is locked", 2000, "red");
      });
      btn.style.width = "9px";
      btn.classList.add("seven-seg");
      btn.style.fontSize = "15px";
      levelsBtns.push(btn);
    }
  }
  var baseBgr = "rgba(255, 255, 255, 0.2)";
  for (let i = 0; i < amount; i++) {
    var btn = levelsBtns[i];
    const idx = i + 1;
    btn.style.backgroundColor = idx === curLevel ? "green" : idx <= maxLevel ? baseBgr : "rgba(0,0,0,0)";
  }
}

window.addEventListener("contextmenu", (e) => e.preventDefault());

var keys = {};
window.addEventListener("keydown", (e) => {
  k = e.key.toLowerCase();
  keys[k] = true;
  if (k === "shift") toggleFlagMode();
});

window.addEventListener("keyup", (e) => {
  k = "";
  keys[e.key.toLowerCase()] = false;
});

function reveal(c, when = 0) {
  if (c.revealed) return;
  c.revealed = true;
  if (!c.hasMine) toReveal--;
  revealD.textContent = "To Reveal: " + toReveal;
  c.classList.remove("flagged");
  if (when === 0) {
    c.classList.remove("hidden");
    if (c.hasMine) c.classList.add("mine");
  } else
    setTimeout(() => {
      c.classList.remove("hidden");
      if (c.hasMine) c.classList.add("mine");
    }, when);
  if (c.hasMine || !c.numBombs) return;
  if (when === 0) c.textContent = c.numBombs;
  else
    setTimeout(() => {
      c.textContent = c.numBombs;
    }, when);
  c.style.color = colors[c.numBombs % (colors.length - 1)];
  c.style.display = "flex";
  c.style.alignItems = "center";
  c.style.justifyContent = "center";
  c.style.fontWeight = "bold";
  const cellSize = parseFloat(c.style.width);
  c.style.fontSize = cellSize * 0.6 + "px";
  c.style.fontWeight = "bold";
}

function floodFillMiner(cells, w, h, cord, revDur = 0) {
  var hw = Math.floor(w / 2);
  var hh = Math.floor(h / 2);
  if (cord[0] < -hw || cord[0] > hw + (w % 2) - 1 || cord[1] < -hh || cord[1] > hh + (h % 2) - 1) return;
  const idx = (cord[1] + hh) * w + (cord[0] + hw);

  const c = cells[idx];
  if (c.hasMine || c.revealed) return;
  reveal(c, revDur);
  if (revDur) {
    revDur += 25;
    if (revDur > 2000) revDur = 2000;
  }
  if (c.numBombs > 0) return;
  for (let dy = -1; dy <= 1; dy++) {
    for (let dx = -1; dx <= 1; dx++) {
      if (dx === 0 && dy === 0) continue;
      floodFillMiner(cells, w, h, [cord[0] + dx, cord[1] + dy], revDur);
    }
  }
}

function setNeighCountAroundCell(centerCell, w, h) {
  var hw = Math.floor(w / 2);
  var hh = Math.floor(h / 2);
  for (let y = -1; y < 2; y++) {
    for (let x = -1; x < 2; x++) {
      if (!x && !y) continue;
      const c = [centerCell.cord[0] + x, centerCell.cord[1] + y];
      if (c[0] < -hw || c[0] > hw + (w % 2) - 1 || c[1] < -hh || c[1] > hh + (h % 2) - 1) continue;
      const idx = (c[1] + hh) * w + (c[0] + hw);
      const cell = cells[idx];
      if (cell.hasMine) continue;
      cell.numBombs += centerCell.hasMine ? 1 : -1;
    }
  }
}

function getNeighCount(cells, cell, w, h) {
  const crd = cell.cord;
  let numBombs = 0;
  var hw = Math.floor(w / 2);
  var hh = Math.floor(h / 2);

  for (let y = -1; y < 2; y++) {
    for (let x = -1; x < 2; x++) {
      if (!x && !y) continue;
      const c = [crd[0] + x, crd[1] + y];
      if (c[0] < -hw || c[0] > hw + (w % 2) - 1 || c[1] < -hh || c[1] > hh + (h % 2) - 1) continue;
      const idx = (c[1] + hh) * w + (c[0] + hw);
      const cell = cells[idx];
      if (cell.hasMine) numBombs++;
    }
  }
  cell.numBombs = numBombs;
  return numBombs;
}

function getRevealCount(cells) {
  var revCount = 0;
  for (const c of cells) if (!c.hasMine) revCount++;
  return revCount;
}

function setRevealCount(count = getRevealCount(cells)) {
  toReveal = count;
  revealD.textContent = "To Reveal: " + toReveal;
  return toReveal;
}

function validateMineIndex(startCell, rIndex, margins = 2) {
  if (cells[rIndex] == startCell || cells[rIndex].hasMine) return false;
  if (margins <= 0) return true;
  if (Math.abs(cells[rIndex].cord[0] - startCell.cord[0]) >= margins) return true;
  return Math.abs(cells[rIndex].cord[1] - startCell.cord[1]) >= margins;
}

function placeRandomMine(startCell) {
  var rIndex = Math.floor(Math.random() * cells.length);
  while (!validateMineIndex(startCell, rIndex, 2)) {
    var rIndex = Math.floor(Math.random() * cells.length);
  }
  cells[rIndex].hasMine = true;
  setNeighCountAroundCell(cells[rIndex], cellW, cellH);
}

function resetCell(c) {
  c.numBombs = 0;
  c.textContent = "";
  c.hasMine = false;
}

function placeMinesRandomly(startCell, amount) {
  for (const c of cells) resetCell(c);
  for (let i = 0; i < amount; i++) placeRandomMine(startCell, amount);
}

function loadValidMap(cells, w, h) {
  var name = w + "x" + h + ".txt";
  fetch("/ressources/uploads/mineSweeperMaps/" + name)
    .then((r) => {
      if (!r.ok) throw new Error("Map not found");
      return r.text();
    })
    .then((mapText) => {
      console.log("Loaded map:", mapText);
      for (const c of cells) resetCell(c);
      setRevealCount(0);
      let idx = 0;
      for (let i = 0; i < mapText.length; i++) {
        var ch = mapText[i];
        if (ch === "\n") continue;
        if (ch === "1") {
          cells[idx].hasMine = true;
          setNeighCountAroundCell(cells[idx], cellW, cellH);
        }
        idx++;
      }
      setRevealCount();
      console.warn("REV COUNT IS " + toReveal + " bombs " + (cells.length - toReveal));
      return true;
    })
    .catch((err) => announce("Load map failed: " + err));
  return false;
}

function writeValidMap(cells) {
  var map = "";
  for (let y = 0; y < cellH; y++) {
    for (let x = 0; x < cellW; x++) map += cells[y * cellW + x].hasMine ? "1" : "0";
    map += "\n";
  }

  const blob = new Blob([map], { type: "text/plain" });
  const file = new File([blob], "map.txt", { type: "text/plain" });
  var name = cellW + "x" + cellH + ".txt";
  fetch("/upload", {
    method: "POST",
    headers: {
      "Content-Type": "text/plain",
      "X-Upload-Path": "mineSweeperMaps/" + name,
      "X-Mode": "Create",
    },
    body: file,
  })
    .then((r) => r.text())
    .then((t) => console.log("Map uploaded:", t))
    .catch((err) => announce("Upload failed: " + err));
}

function startPlayableGrid(startCell) {
  var perc = curLevel + 10;
  var numMines = Math.round((cells.length / 100) * perc);
  placeMinesRandomly(startCell, numMines);
  var maxTries = 100;
  var curTry = 0;
  while (curTry++ < maxTries && !isSolvable(cells, cellW, cellH, startCell)) placeMinesRandomly(startCell, numMines);
  var solved = curTry < maxTries;
  if (solved) console.warn("Couldn't find a solvable grid in " + maxTries);
  else console.warn("Solvable grid generated in " + curTry + " tries");
  if (!solved) announce("NEEDS GUESSES!", 2000, "red");
  console.warn("REV COUNT IS " + toReveal + " bombs " + (cells.length - toReveal));
  setRevealCount();
}

function initGrid(diff) {
  var n = 5 + diff;
  const cellSz = (window.innerHeight * 0.5) / n;
  cellW = n * 2;
  cellH = n;
  const halfW = Math.floor(cellW / 2);
  const halfH = Math.floor(cellH / 2);
  var offset = n % 2 === 0 ? 0 : -cellSz / 2;
  const c = [window.innerWidth / 2, window.innerHeight * 0.65 + offset];
  if (cells) for (const c of cells) c.remove();
  cells = [];
  for (let y = -halfH; y < halfH + (cellH % 2); y++) {
    for (let x = -halfW; x < halfW + (cellW % 2); x++) {
      const p = [c[0] + x * cellSz, c[1] + y * cellSz];
      const cell = writeBox(cellSz, cellSz, p[0], p[1], "");
      cells.push(cell);
      cell.cord = [x, y];
      cell.style.textContent = "";
      cell.classList.add("mineSweeperCell", "hidden");
      cell.style.userSelect = "none";
      cell.hasMine = false;
      cell.revealed = false;
      cell.x = parseFloat(cell.style.left);
      cell.y = parseFloat(cell.style.top);
      cell.addEventListener("mouseenter", () => {
        if (cell.revealed || cell.flagged || gameOver) return;
        cell.style.left = cell.x - 5 + "px";
        cell.style.top = cell.y - 5 + "px";
      });
      cell.addEventListener("mouseleave", () => {
        if (cell.revealed || cell.flagged || gameOver) return;
        cell.style.left = cell.x + "px";
        cell.style.top = cell.y + "px";
      });
      cell.addEventListener("mouseup", (e) => {
        cell.style.left = cell.x + "px";
        cell.style.top = cell.y + "px";
        if (cell.revealed || gameOver) return;
        if (e.button === 2 || clickFlag) {
          playClick();
          cell.classList.toggle("flagged");
          cell.flagged = !cell.flagged;
          return;
        }
        if (cell.flagged) return;
        playClick();
        if (!hasStarted) {
          startPlayableGrid(cell);
          hasStarted = true;
        }
        floodFillMiner(cells, cellW, cellH, cell.cord, 0);
        reveal(cell, cells.length);
        if (!cell.hasMine) {
          getNeighCount(cells, cell, cellW, cellH);
        } else {
          for (const c of cells) {
            if (c.hasMine) reveal(c);
          }
          cell.style.backgroundColor = "red";
          gameOver = true;
          announce("Game Over", 2000, "red");
          return;
        }
        if (!toReveal) {
          announce("Bravo!", 2000, "green");
          diff++;
          if (diff > maxLevel) {
            maxLevel = diff;
            updateLvlButtons();
            saveMaxLevel();
          }
        }
      });
    }
  }
}

function initMinerGame(level = 1) {
  if (level >= maxLevel) maxLevel = level;
  curLevel = level;
  hasStarted = false;
  gameOver = false;
  setRevealCount(0);
  updateLvlButtons();
  initGrid(level);
}

function saveMaxLevel() {
  if (window.MAX_LEVEL === undefined || maxLevel > window.MAX_LEVEL) {
    window.MAX_LEVEL = maxLevel;
    const body = new URLSearchParams({ mineSweeperMaxLevel: maxLevel });
    fetch("/cgi-bin/auth/setters/setScore.php", {
      method: "POST",
      credentials: "same-origin",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
        "X-minesweeperMaxLevel": maxLevel.toString(),
      },
      body: body.toString(),
    }).catch(() => announce("Can't save max level"));
  }
  announce("Max level saved - " + maxLevel);
}
