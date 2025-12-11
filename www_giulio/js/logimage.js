var scoreDiv = addDiv("", [window.innerWidth / 2 - 200, window.innerHeight * 0.3]);
var highScoreDiv = addDiv("", [window.innerWidth / 2 + 200, window.innerHeight * 0.3]);

const colors = ["blue", "green", "orange", "red", "purple"];
var score = 0;
var difficulty = 0;
var k = "";
const restartBtn = addButton("Restart", [window.innerWidth / 2, window.innerHeight * 0.2], () => {
  initMinerGame();
});

window.addEventListener("keydown", (e) => {
  k = e.key.toLowerCase();
});

function reveal(c) {
  c.revealed = true;
  c.style.backgroundColor = c.hasBomb ? "red" : "rgba(183, 183, 183, 1)";
}

function pappillon() {
	const w = 15, h = 15;
	var pap = ["   .       .   ",
		".   .     .   .",
		""
	]
}

function initLogimage(diff = 3) {
  const cellSz = 20;
  const cellW = 20;
  const cellH = 20;
	const cells = [];
	
	const seed = [""]

  score = 0;
  scoreDiv.textContent = "Score " + 0;
  restartBtn.style.display = "none";

  highScoreDiv.textContent = "High Score " + (window.HIGH_SCORE === undefined ? 0 : window.HIGH_SCORE);

  const c = [window.innerWidth / 2, window.innerHeight * 0.65];
  for (let y = -Math.round(cellH / 2); y < Math.round(cellH / 2); y++) {
    for (let x = -Math.round(cellW / 2); x < Math.round(cellW / 2); x++) {
      const p = [c[0] + x * cellSz, c[1] + y * cellSz];
      const cell = writeBox(cellSz - 1, cellSz - 1, p[0], p[1], "grey");
      cells.push(cell);
      cell.cord = [x, y];
      cell.style.userSelect = "none";
      cell.revealed = false;

      if (Math.random() > 0.2) cell.style.backgroundColor = "white";

      cell.addEventListener("mouseenter", () => {
        if (cell.revealed) return;
        cell.style.scale = 1.2;
      });
      cell.addEventListener("mouseleave", () => {
        if (cell.revealed) return;
        cell.style.scale = 1;
      });
      cell.addEventListener("mousedown", (e) => {
        if (cell.revealed) return;
        floodFillMiner(cells, cellW, cellH, cell.cord);
        reveal(cell);
        if (!cell.hasBomb) getNeighCount(cells, cell, cellW, cellH);
      });
    }
  }
}
