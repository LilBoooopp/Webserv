var scoreDiv = addDiv("", [window.innerWidth * 0.25, window.innerHeight * 0.5]);
var highScoreDiv = addDiv("", [window.innerWidth * 0.25, window.innerHeight * 0.55]);
var revealD = addDiv("toReveal: ", [window.innerWidth * 0.25, window.innerHeight * 0.6]);
var diffDiv = addDiv("Diff: ", [window.innerWidth * 0.25, window.innerHeight * 0.65]);
var cells;
var gameOver = false;
const colors = ["blue", "green", "orange", "red", "purple"];
var score = 0;
var difficulty = 0;
var k = "";
var toReveal = 0;
var hasStarted = false;
const restartBtn = addButton("Restart", [window.innerWidth / 2, window.innerHeight * 0.2], () => {
  initMinerGame(difficulty);
});

const nextLevelBtn = addButton("Next Level", [window.innerWidth / 2, window.innerHeight * 0.2], () => {
  initMinerGame(difficulty + 1);
});

window.addEventListener("keydown", (e) => {
  k = e.key.toLowerCase();
});

function reveal(c, when = 0) {
  if (c.revealed) return;
  c.revealed = true;
  if (!c.hasBomb) toReveal--;
  revealD.textContent = "To Reveal: " + toReveal;

  if (when === 0) {
    c.classList.remove("hidden");
    if (c.hasBomb) c.classList.add("bomb");
  } else
    setTimeout(() => {
      c.classList.remove("hidden");
      if (c.hasBomb) c.classList.add("bomb");
    }, when);
  if (c.hasBomb || !c.numBombs) return;
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
  var hw = Math.round(w / 2);
  var hh = Math.round(h / 2);
  if (cord[0] < -hw || cord[0] >= hw || cord[1] < -hh || cord[1] >= hh) return;
  const idx = (cord[1] + hh) * w + (cord[0] + hw);

  const c = cells[idx];
  if (c.hasBomb || c.revealed) return;
  reveal(c, revDur);
  revDur += 25;
  if (revDur > 2000) revDur = 2000;
  if (c.numBombs > 0) return;
  floodFillMiner(cells, w, h, [cord[0] - 1, cord[1]], revDur);
  floodFillMiner(cells, w, h, [cord[0] + 1, cord[1]], revDur);
  floodFillMiner(cells, w, h, [cord[0], cord[1] - 1], revDur);
  floodFillMiner(cells, w, h, [cord[0], cord[1] + 1], revDur);
}

function getNeighCount(cells, cell, w, h) {
  const crd = cell.cord;
  let numBombs = 0;
  var hw = Math.round(w / 2);
  var hh = Math.round(h / 2);

  for (let y = -1; y < 2; y++) {
    for (let x = -1; x < 2; x++) {
      if (!x && !y) continue;
      const c = [crd[0] + x, crd[1] + y];
      if (c[0] < -hw || c[0] >= hw || c[1] < -hh || c[1] >= hh) continue;
      const idx = (c[1] + Math.round(hw)) * w + (c[0] + hw);
      const cell = cells[idx];
      if (cell.hasBomb) numBombs++;
    }
  }
  cell.numBombs = numBombs;
  return numBombs;
}

function initGrid(diff) {
  var n = diff * 5;
  if (n % 2 !== 0) n++;
  const cellSz = (window.innerHeight * 0.5) / n;
  const cellW = n;
  const cellH = n;
  const c = [window.innerWidth / 2, window.innerHeight * 0.65];
  if (cells) for (const c of cells) c.remove();
  cells = [];
  for (let y = -Math.round(cellH / 2); y < Math.round(cellH / 2); y++) {
    for (let x = -Math.round(cellW / 2); x < Math.round(cellW / 2); x++) {
      const p = [c[0] + x * cellSz, c[1] + y * cellSz];
      const cell = writeBox(cellSz, cellSz, p[0], p[1], "");
      cells.push(cell);
      cell.cord = [x, y];
      cell.classList.add("mineSweeperCell", "hidden");
      cell.style.userSelect = "none";
      cell.hasBomb = false;
      cell.revealed = false;
      cell.x = parseFloat(cell.style.left);
      cell.y = parseFloat(cell.style.top);
      cell.addEventListener("mouseenter", () => {
        if (cell.revealed || gameOver) return;
        cell.style.left = cell.x - 5 + "px";
        cell.style.top = cell.y - 5 + "px";
      });
      cell.addEventListener("mouseleave", () => {
        if (cell.revealed || gameOver) return;
        cell.style.left = cell.x + "px";
        cell.style.top = cell.y + "px";
      });
      cell.addEventListener("mouseup", (e) => {
        cell.style.left = cell.x + "px";
        cell.style.top = cell.y + "px";
        if (cell.revealed || gameOver) return;
        if (!hasStarted) {
          initMines(cell);
          hasStarted = true;
        }
        floodFillMiner(cells, cellW, cellH, cell.cord);
        reveal(cell);
        if (!toReveal) nextLevelBtn.style.display = "block";
        if (!cell.hasBomb) {
          getNeighCount(cells, cell, cellW, cellH);
        } else {
          for (const c of cells) reveal(c);
          gameOver = true;
          restartBtn.style.display = "block";
          return;
        }
      });
    }
  }

  function initMines(cellWithNoBomb) {
    toReveal = 0;
    for (const c of cells) {
      if (c === cellWithNoBomb) continue;
      c.hasBomb = Math.floor(Math.random() < diff * 0.03);
    }
    // Count non-bomb cells that need to be revealed
    for (const c of cells) {
      if (!c.hasBomb) toReveal++;
    }
    revealD.textContent = "To Reveal: " + toReveal;
    for (const c of cells) getNeighCount(cells, c, cellW, cellH);
  }
}

function initMinerGame(diff = 2) {
  revealD.textContent = "To Reveal: " + 0;

  diffDiv.textContent = "Diff: " + diff;
  hasStarted = false;
  gameOver = false;
  difficulty = diff;
  score = 0;
  scoreDiv.textContent = "Score " + 0;
  restartBtn.style.display = "none";
  nextLevelBtn.style.display = "none";
  highScoreDiv.textContent = "High Score " + (window.HIGH_SCORE === undefined ? 0 : window.HIGH_SCORE);
  initGrid(diff);
}
