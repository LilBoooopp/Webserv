const nextLevelBtn = addButton("Next Level", [window.innerWidth * 0.6, window.innerHeight * 0.2], () => initMorpion(difficulty + 1));
const restartBtn = addButton("Restart", [window.innerWidth * 0.4, window.innerHeight * 0.2], () => initMorpion(difficulty));
const turnLabel = addDiv("TURN: X" + turn, [window.innerWidth * 0.5, window.innerHeight * 0.1], 1, null, null, true);
var turn;
var difficulty = 3;
var cells = [];
var cw, ch;
var gameWinner = null;
var Xwin = 0;
var Owin = 0;

function removeCells(cells) {
	for (const c of cells) {
		if (c.line) c.line.remove();
		c.remove();
	}
}

function minimax(board, depth, isMaximizing) {
	const result = checkWinner(board);
	if (result !== null) {
		if (result === "O") return 10 - depth;
		if (result === "X") return depth - 10;
		return 0;
	}

	if (isMaximizing) {
		let bestScore = -Infinity;
		for (let i = 0; i < board.length; i++) {
			if (board[i] === "") {
				board[i] = "O";
				const score = minimax(board, depth + 1, false);
				board[i] = "";
				bestScore = Math.max(score, bestScore);
			}
		}
		return bestScore;
	} else {
		let bestScore = Infinity;
		for (let i = 0; i < board.length; i++) {
			if (board[i] === "") {
				board[i] = "X";
				const score = minimax(board, depth + 1, true);
				board[i] = "";
				bestScore = Math.min(score, bestScore);
			}
		}
		return bestScore;
	}
}

function checkWinner(board) {
	for (let y = 0; y < ch; y++) {
		let first = board[y * cw];
		if (first === "") continue;
		let allSame = true;
		for (let x = 1; x < cw; x++) {
			if (board[y * cw + x] !== first) {
				allSame = false;
				break;
			}
		}
		if (allSame) return first;
	}

	for (let x = 0; x < cw; x++) {
		let first = board[x];
		if (first === "") continue;
		let allSame = true;
		for (let y = 1; y < ch; y++) {
			if (board[y * cw + x] !== first) {
				allSame = false;
				break;
			}
		}
		if (allSame) return first;
	}

	let first = board[0];
	if (first !== "") {
		let allSame = true;
		for (let i = cw + 1; i < board.length; i += cw + 1) {
			if (board[i] !== first) {
				allSame = false;
				break;
			}
		}
		if (allSame) return first;
	}

	first = board[cw - 1];
	if (first !== "") {
		let allSame = true;
		for (let i = cw - 1 + cw - 1; i < board.length - 1; i += cw - 1) {
			if (board[i] !== first) {
				allSame = false;
				break;
			}
		}
		if (allSame) return first;
	}

	if (board.every((cell) => cell !== "")) return "draw";
	return null;
}

function playMinMaxMove() {
	const board = cells.map((c) => c.textContent);
	let bestScore = -Infinity;
	let bestMove = -1;
	for (let i = 0; i < board.length; i++) {
		if (board[i] === "") {
			board[i] = "O";
			const score = minimax(board, 0, false);
			board[i] = "";
			if (score > bestScore) {
				bestScore = score;
				bestMove = i;
			}
		}
	}
	if (bestMove !== -1)
		tagCell(cells[bestMove]);
}

function playBotRandomMove() {
	var freeCells = cells.filter((c) => c.textContent === "");
	if (!freeCells.length) return;
	var randIndex = Math.floor(Math.random() * freeCells.length);
	tagCell(freeCells[randIndex]);
}

function playBotTurn() {
	if (Math.random() >= .8) playBotRandomMove();
	else playMinMaxMove();
	onTurnEnd();
}

function onTurnEnd() {
	var status = updateGame(cells, cw, ch);
	if (status) onGameEnd(status === 1);
	else switchTurn();
}

function switchTurn() {
	turn = turn === "X" ? "O" : "X";
	turnLabel.textContent = "TURN: " + turn;
}

function onGameEnd(hasWinner) {
	gameWinner = hasWinner ? turn : "none";
	if (hasWinner) {
		announce(turn + " WON!");
		nextLevelBtn.style.display = "block";
	} else announce("Game Ended: 1 | 1");
	restartBtn.style.display = "block";
}

function updateGame(cells, cw, ch) {
	for (let y = 0; y < ch; y++) {
		var prevSign;
		for (let x = 0; x < cw; x++) {
			var index = y * cw + x;
			var newSign = cells[index].textContent;
			if (newSign === "") break;
			if (x === 0) {
				prevSign = newSign;
				continue;
			}
			if (newSign != prevSign) break;
			prevSign = newSign;
			if (x == cw - 1) {
				return 1;
			}
		}
	}
	for (let x = 0; x < cw; x++) {
		var prevSign;
		for (let y = 0; y < ch; y++) {
			var index = y * cw + x;
			var newSign = cells[index].textContent;
			if (newSign === "") break;
			if (y === 0) {
				prevSign = newSign;
				continue;
			}
			if (newSign != prevSign) break;
			prevSign = newSign;
			if (y == ch - 1) {
				return 1;
			}
		}
	}

	var prevSign;
	for (let i = 0; i < cells.length; i += cw + 1) {
		var newSign = cells[i].textContent;
		if (i === 0) {
			prevSign = newSign;
			continue;
		}
		if (newSign === "" || newSign !== prevSign) break;
		if (i === cells.length - 1) {
			return 1;
		}
		prevSign = newSign;
	}
	for (const c of cells) if (c.textContent === "") return 0;
	return -1;
}

function tagCell(c) {
	c.style.color = turn === "X" ? "red" : "blue";
	c.textContent = turn;
}

function initMorpion(diff = 3) {
	cw = diff;
	ch = diff;
	difficulty = diff;
	turn = "X";
	gameWinner = null;
	turnLabel.textContent = "TURN: " + turn;

	const cellSz = (window.innerHeight * 0.5) / cw;
	const center = [window.innerWidth / 2, window.innerHeight * 0.6];
	restartBtn.style.display = "none";
	nextLevelBtn.style.display = "none";
	var p = [center[0] - cellSz * (cw / 2), center[1] + 50 - cellSz * (ch / 2)];

	removeCells(cells);
	cells = [];
	var i = 0;
	for (let y = 0; y < ch; y++) {
		for (let x = 0; x < cw; x++) {
			const c = writeBox(cellSz - 2, cellSz - 2, p[0] + 1 + x * cellSz, p[1] + 1 + y * cellSz, "rgba(0,0,0,0)");
			if (x > 0 && y === 0) {
				c.line = writeBox(2, ch * cellSz - 2, p[0] - 1 + x * cellSz, p[1] + 1 + y * cellSz, "black");
			} else if (y > 0 && x === 0) {
				c.line = writeBox(cw * cellSz - 2, 2, p[0] + 1 + x * cellSz, p[1] - 1 + y * cellSz, "black");
			}
			c.index = i++;
			c.classList.add("audio-button");
			c.textContent = "";
			c.style.scale = "1";
			c.style.display = "flex";
			c.style.alignItems = "center";
			c.style.justifyContent = "center";
			c.color = "white";
			c.style.fontSize = "40px";
			c.addEventListener("mousedown", () => {
				if (nextLevelBtn.style.display !== "none" || restartBtn.style.display !== "none") return;
				if (c.textContent === "") {
					tagCell(c);
					onTurnEnd();
					if (!gameWinner) playBotTurn();
				}
			});
			cells.push(c);
		}
	}
}
