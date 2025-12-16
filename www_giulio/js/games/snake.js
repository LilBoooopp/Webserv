var plr = document.createElement("div");
var scoreDiv = addDiv("", [window.innerWidth / 2 - 200, window.innerHeight * 0.3]);
var highScoreDiv = addDiv("", [window.innerWidth / 2 + 200, window.innerHeight * 0.3]);

var score = 0;
var difficulty = 0;
var k = "";
var restartBtn = null;

window.addEventListener("keydown", (e) => {
  k = e.key.toLowerCase();
});

function initSnakeGame(diff = 0) {
  loadUsers((users) => {
    displayUsersScores(users, "Snake");
  });

  const cellSz = 10;
  const cellW = 40;
  const cellH = 40;
  const cells = [];

  if (!restartBtn)
    restartBtn = addButton("Restart", [window.innerWidth / 2, window.innerHeight * 0.2], () => {
      initSnakeGame();
    });
  restartBtn.style.display = "none";

  difficulty = diff;
  score = 0;
  scoreDiv.textContent = "Score " + 0;
  restartBtn.style.display = "none";

  highScoreDiv.textContent = "High Score " + (window.HIGH_SCORE === undefined ? 0 : window.HIGH_SCORE);

  plr.snakeDir = "";
  plr.body = [];

  const c = [window.innerWidth / 2, window.innerHeight * 0.65];
  for (let y = -Math.round(cellH / 2); y < Math.round(cellH / 2); y++) {
    for (let x = -Math.round(cellW / 2); x < Math.round(cellW / 2); x++) {
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
  const am = Math.round(cells.length * (diff * 0.01));
  for (let i = 0; i < am; i++) {
    var wall = cells[Math.round(Math.random() * (cells.length - 1))];
    while (wall.style.backgroundColor !== "white") wall = cells[Math.round(Math.random() * (cells.length - 1))];
    wall.style.backgroundColor = "grey";
  }

  var food = cells[Math.round(Math.random() * cells.length)];
  food.style.backgroundColor = "green";
  food.isFood = true;

  setTimeout(() => {
    loop(cells, cellW, cellH, plr);
  }, 300);
}

var overload = 3;

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

function frame(cells, w, h, plr) {
  const curCrd = plr.body[plr.body.length - 1].cord;
  const pd = plr.snakeDir;
  plr.snakeDir = k === "a" && pd !== "right" ? "left" : k === "d" && pd !== "left" ? "right" : k === "w" && pd !== "down" ? "up" : k === "s" && pd !== "up" ? "down" : pd;
  if (plr.snakeDir === "") return 1;
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
    const wasMega = newCell.isMega;
    score += 50 * (difficulty <= 0 ? 1 : difficulty);
    if (wasMega) {
      score += 150 * (difficulty <= 0 ? 1 : difficulty);
      overload += 10;
    }
    scoreDiv.textContent = "Score " + score;
    newCell.isFood = false;
    newCell.isMega = false;
    if (!wasMega) spawnFruit(cells, false);
  } else if (newCell.style.backgroundColor !== "white") return 0;
  else if (overload > 0) overload--;
  else {
    plr.body[0].style.backgroundColor = "white";
    plr.body.splice(0, 1);
  }
  plr.body.push(newCell);
  newCell.style.backgroundColor = "black";
  return 1;
}

function onGameOver() {
  if (window.HIGH_SCORE === undefined || score > window.HIGH_SCORE) {
    window.HIGH_SCORE = score;
    const body = new URLSearchParams({ snakeHighScore: score });
    fetch("/cgi/auth/setters/setScore.php", {
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
}

function loop(cells, w, h, plr) {
  if (k && !frame(cells, w, h, plr)) onGameOver();
  else
    setTimeout(() => {
      requestAnimationFrame(() => loop(cells, w, h, plr));
    }, 50);
}

var usersDivs = null;
function displayUsersScores(users, type) {
  if (usersDivs) usersDivs.forEach((div) => div.remove());
  usersDivs = [];
  users.sort((a, b) => b.snakeHighScore - a.snakeHighScore);
  var cp = [window.innerWidth * 0.1, window.innerHeight * 0.5];
  users.forEach((user) => {
    usersDivs.push(addDiv(`${user.username}:`, cp));
    usersDivs.push(addDiv(`${user.snakeHighScore} points`, [cp[0] + 150, cp[1]]));
    cp[1] += 20;
  });
}
