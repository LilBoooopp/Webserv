var toReveal_solver = 0;

function noBombsAround(cell) {
  return cell.numBombs === 0 && cell.revealed;
}

function floodFillSolver(cells, w, h, cord) {
  var hw = Math.floor(w / 2);
  var hh = Math.floor(h / 2);
  if (cord[0] < -hw || cord[0] > hw + (w % 2) - 1 || cord[1] < -hh || cord[1] > hh + (h % 2) - 1) return;
  const idx = (cord[1] + hh) * w + (cord[0] + hw);
  const c = cells[idx];
  if (c.hasBomb || c.revealed) return;
  reveal_solver(c);
  if (c.numBombs > 0) return;
  for (let dy = -1; dy <= 1; dy++) {
    for (let dx = -1; dx <= 1; dx++) {
      if (dx === 0 && dy === 0) continue;
      floodFillSolver(cells, w, h, [cord[0] + dx, cord[1] + dy]);
    }
  }
}

function setDbgs(cd) {
  for (const c of cd) {
    if (c.dbg === undefined) {
      var div = addDiv("", [c.x + c.w * 0.3, c.y + c.w * 0.2], 0.6, "green", null, true);
      div.style.userSelect = "none";
      div.style.pointerEvents = "none";
      c.dbg = div;
    }
    if (c.revealed) {
      c.dbg.textContent = "";
      continue;
    }
    c.dbg.textContent = c.bombChance;
  }
}

function solveObvious(cd, cw, ch) {
  for (let y = 0; y < ch; y++) {
    for (let x = 0; x < cw; x++) {
      var i = y * cw + x;
      var c = cd[i];
      if (!c.revealed || c.numBombs === 0) continue;
      var neighbors = getNeighbors(cd, x, y, cw, ch);
      var hiddenN = neighbors.filter((n) => !n.revealed);
      var bombsAround = c.numBombs;
      if (hiddenN.length != bombsAround) continue;
      for (const n of hiddenN) n.bombChance = 100;
    }
  }
}

function solvePart2(cd, cw, ch) {
  for (let y = 0; y < ch; y++) {
    for (let x = 0; x < cw; x++) {
      var i = y * cw + x;
      var c = cd[i];
      if (!c.revealed) continue;
      if (!c.numBombs) {
        var neighbors = getNeighbors(cd, x, y, cw, ch);
        for (const n of neighbors) if (!n.revealed) n.bombChance = 0;
        continue;
      }
      var neighbors = getNeighbors(cd, x, y, cw, ch);
      var hiddenN = neighbors.filter((n) => !n.revealed);
      var knownBombs = hiddenN.filter((n) => n.bombChance === 100);
      var unkown = hiddenN.filter((n) => n.bombChance !== 100);
      var bombsAround = c.numBombs;
      if (knownBombs.length === bombsAround) for (const u of unkown) u.bombChance = 0;
    }
  }
}

function solve(cd, cw, ch, showDbg = true) {
  for (const c of cd) c.bombChance = "-";
  solveObvious(cd, cw, ch);
  solvePart2(cd, cw, ch);

  var solutions = cd.filter((c) => !c.revealed && c.bombChance === 0);
  //   if (showDbg) setDbgs(cd);
  if (!solutions.length) return null;
  return solutions[Math.floor(Math.random() * solutions.length)];
}

var step = 0;
var minSteps = 0;
function recursiveSolve(cd, cw, ch, showDbg = true) {
  if (toReveal_solver <= 0) return step >= minSteps;
  var safeCell = solve(cd, cw, ch, showDbg);
  if (!safeCell) return 0;
  step++;
  floodFillSolver(cd, cw, ch, safeCell.cord);
  reveal_solver(safeCell);
  return recursiveSolve(cd, cw, ch, showDbg);
}

function countBombs(cd, cw, ch, cell) {
  const crd = cell.cord;
  let numBombs = 0;
  var hw = Math.floor(cw / 2);
  var hh = Math.floor(ch / 2);
  for (let y = -1; y < 2; y++) {
    for (let x = -1; x < 2; x++) {
      if (!x && !y) continue;
      const c = [crd[0] + x, crd[1] + y];
      if (c[0] < -hw || c[0] > hw + (cw % 2) - 1 || c[1] < -hh || c[1] > hh + (ch % 2) - 1) continue;
      const idx = (c[1] + hh) * cw + (c[0] + hw);
      const n = cd[idx];
      if (n.hasBomb) numBombs++;
    }
  }
  return numBombs;
}

function reveal_solver(c) {
  if (c.revealed) return;
  c.revealed = true;
  if (!c.hasBomb) toReveal_solver--;
}

function isSolvable(cells, cw, ch, startCell, minTries = 0) {
  toReveal_solver = toReveal;
  minSteps = minTries;
  var cd = [];
  for (const c of cells) {
    const copy = {
      hasBomb: c.hasBomb,
      revealed: c.revealed,
      cord: [c.cord[0], c.cord[1]],
      numBombs: c.numBombs,
    };
    if (!copy.revealed && !copy.hasBomb) toReveal_solver++;
    cd.push(copy);
  }
  var startCd = cd[cells.indexOf(startCell)];
  reveal_solver(startCd);
  step = 0;
  var solvable = recursiveSolve(cd, cw, ch, false);
//   console.warn("Solver ended - steps: " + step + ", status: " + (solvable ? "SOLVABLE" : "UNSOLVABLE"));
  return solvable;
}

function getNeighbors(cd, x, y, cw, ch) {
  const lists = getNeighborIdxCache(cw, ch);
  return lists[y * cw + x].map((idx) => cd[idx]);
}

var neighborIdxCache = { cw: null, ch: null, lists: [] };
function getNeighborIdxCache(cw, ch) {
  if (neighborIdxCache.cw === cw && neighborIdxCache.ch === ch && neighborIdxCache.lists.length) return neighborIdxCache.lists;

  const lists = new Array(cw * ch);
  for (let y = 0; y < ch; y++) {
    for (let x = 0; x < cw; x++) {
      const acc = [];
      for (let yy = y - 1; yy <= y + 1; yy++) {
        if (yy < 0 || yy >= ch) continue;
        for (let xx = x - 1; xx <= x + 1; xx++) {
          if (xx < 0 || xx >= cw) continue;
          if (xx === x && yy === y) continue;
          acc.push(yy * cw + xx);
        }
      }
      lists[y * cw + x] = acc;
    }
  }

  neighborIdxCache = { cw, ch, lists };
  return lists;
}

function generatePlayableGrid() {}
