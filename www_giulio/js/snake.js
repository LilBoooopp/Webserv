var plr = document.createElement("div");
var scoreDiv = addDiv("", [window.innerWidth / 2 - 200, window.innerHeight * 0.3]);
var highScoreDiv = addDiv("", [window.innerWidth / 2 + 200, window.innerHeight * 0.3]);

var score = 0;
const restartBtn = addButton("Restart", [window.innerWidth / 2, window.innerHeight * 0.2], () => {
  initSnakeGame();
});

window.addEventListener("keydown", (e) => {
  const k = e.key.toLowerCase();
  const curDir = plr.snakeDir;
  plr.snakeDir = k === "a" && curDir !== "right" ? "left" : k === "d" && curDir !== "left" ? "right" : k === "w" && curDir !== "down" ? "up" : k === "s" && curDir !== "up" ? "down" : curDir;
});

function initSnakeGame() {
  const cellSz = 10,
    cellW = 40,
    cellH = 40;
  const cells = [];

  score = 0;
  scoreDiv.textContent = "Score " + 0;
  restartBtn.style.display = "none";

  highScoreDiv.textContent = "High Score " + (window.HIGH_SCORE === undefined ? 0 : window.HIGH_SCORE);

  plr.snakeDir = "down";
  plr.body = [];

  const c = [window.innerWidth / 2, window.innerHeight * 0.65];
  for (let y = -cellH / 2; y < cellH / 2; y++) {
    for (let x = -cellW / 2; x < cellW / 2; x++) {
      const p = [c[0] + x * cellSz, c[1] + y * cellSz];
      var cell = writeBox(cellSz - 1, cellSz - 1, p[0], p[1], "white");
      cells.push(cell);
      cell.cord = [x, y];
      if (!x && !y) {
        cell.style.backgroundColor = "black";
        plr.body.push(cell);
      }
    }
  }

  var food = cells[Math.round(Math.random() * cells.length)];
  food.style.backgroundColor = "green";
  food.isFood = true;

  setTimeout(() => {
    loop(cells, cellW, cellH, plr);
  }, 300);
}

function spawnFruit(cells, isMega = false) {
  var food = cells[Math.round(Math.random() * cells.length)];
  while (food.style.backgroundColor !== "white") food = cells[Math.round(Math.random() * cells.length)];
  food.style.backgroundColor = isMega ? "red" : "green";
  food.isFood = true;
  food.isMega = isMega;
  if (isMega) {
    setTimeout(() => {
      food.isMega = false;
      food.isFood = false;
      food.style.backgroundColor = "white";
    }, 20000);
  } else if (Math.round(Math.random() * 10) <= 2) spawnFruit(cells, true);
}

function loop(cells, w, h, plr) {
  const curCrd = plr.body[plr.body.length - 1].cord;
  var newX = curCrd[0] + (plr.snakeDir === "left" ? -1 : plr.snakeDir === "right" ? 1 : 0);
  var newY = curCrd[1] + (plr.snakeDir === "up" ? -1 : plr.snakeDir === "down" ? 1 : 0);
  const halfW = w / 2;
  const halfH = h / 2;

  if (newX < -halfW) newX = halfW - 1;
  else if (newX >= halfW) newX = -halfW;
  if (newY < -halfH) newY = halfH - 1;
  else if (newY >= halfH) newY = -halfH;

  const idx = (newY + halfH) * w + (newX + halfW);

  const newCell = cells[idx];
  if (newCell.isFood) {
    score += newCell.isMega ? 200 : 50;
    scoreDiv.textContent = "Score " + score;
    newCell.isFood = false;
    newCell.isMega = false;
    spawnFruit(cells, false);
  } else if (newCell.style.backgroundColor === "black") {
    if (window.HIGH_SCORE === undefined || score > window.HIGH_SCORE) {
      window.HIGH_SCORE = score;
      const body = new URLSearchParams({ snakeHighScore: score });
      fetch("/cgi/auth/setScore.php", {
        method: "POST",
        credentials: "same-origin",
        headers: {
          "Content-Type": "application/x-www-form-urlencoded",
          "X-snakeHighScore": score.toString(),
        },
        body: body.toString(),
      }).catch(() => announce("Can't save score"));
    }
    restartBtn.style.display = "block";
    return;
  } else {
    plr.body[0].style.backgroundColor = "white";
    plr.body.splice(0, 1);
  }
  plr.body.push(newCell);
  newCell.style.backgroundColor = "black";
  if (newCell)
    setTimeout(() => {
      requestAnimationFrame(() => loop(cells, w, h, plr));
    }, 50);
}
