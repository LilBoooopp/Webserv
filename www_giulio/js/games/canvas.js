let showStripes = false;
function drawLine(ctx, start, end, color = "white", width = 2, handleSize = 0) {
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.beginPath();
  ctx.moveTo(start[0], start[1]);
  ctx.lineTo(end[0], end[1]);
  ctx.stroke();
  if (handleSize > 0) drawCircle2(ctx, end, handleSize * _scale);
}

function drawStripedLine(p0, p1, color1, color2 = "rgba(0, 0, 0, 0)") {
  const dx = p1[0] - p0[0];
  const dy = p1[1] - p0[1];
  const len = Math.hypot(dx, dy);
  if (!len) return;
  const stripe = 10;
  const speed = 3;
  const ux = dx / len;
  const uy = dy / len;
  let offset = (FRAME * speed) % (stripe * 2);
  if (len <= stripe) offset = 0;
  let curX = p0[0] + ux * offset;
  let curY = p0[1] + uy * offset;
  let remaining = len - offset;
  if (remaining <= 0) {
    curX = p0[0];
    curY = p0[1];
    remaining = len;
    offset = 0;
  }
  ctx.lineWidth = 5;
  const steps = Math.ceil(remaining / stripe);
  for (let s = 0; s < steps; s++) {
    const segLen = Math.min(stripe, remaining - s * stripe);
    const nextX = curX + ux * segLen;
    const nextY = curY + uy * segLen;
    ctx.beginPath();
    ctx.moveTo(curX, curY);
    ctx.lineTo(nextX, nextY);
    ctx.strokeStyle = s % 2 === 0 ? color1 : color2;
    ctx.stroke();
    curX = nextX;
    curY = nextY;
  }
}

function drawCircle2(ctx, pos, radius = 2, color = "white", strokeColor = "black", lineWidth = 4) {
  ctx.beginPath();
  ctx.arc(pos[0], pos[1], radius, 0, 2 * Math.PI);
  ctx.strokeStyle = strokeColor;
  ctx.lineWidth = lineWidth;
  if (color) {
    ctx.fillStyle = color;
    ctx.fill();
  }
  ctx.stroke();
  ctx.closePath();
}

function drawBezierLine(start, end, ctrl, color = "white", width = 8) {
  const step = 0.001;
  ctx.fillStyle = color;

  for (let t = 0; t <= 1; t += step) {
    const mt = 1 - t;

    const x = mt * mt * start[0] + 2 * mt * t * ctrl[0] + t * t * end[0];
    const y = mt * mt * start[1] + 2 * mt * t * ctrl[1] + t * t * end[1];

    ctx.fillRect(x, y, width, width);
  }
}

var _canvas = null;
var ctx = null;
function initCanvas(size = [window.innerWidth, window.innerHeight], pos = [0, 0]) {
  _canvas = document.createElement("canvas");
  _canvas.style.backgroundColor = "black";
  ctx = _canvas.getContext("2d");
  _canvas.width = size[0];
  _canvas.height = size[1];
  _canvas.style.left = pos[0];
  _canvas.style.top = pos[1];
  document.body.appendChild(_canvas);
}

function drawText(ctx, pos, text, color = "white", backgroundColor = null, size = 25, centered = true) {
  if (!size) size = 25;
  size *= 1;
  ctx.font = size + "px MyPixelFont";
  const metrics = ctx.measureText(text);
  const w = metrics.width + 12;
  const h = size + 8;

  let x = pos[0],
    y = pos[1];
  if (centered) {
    x -= w / 2;
    y -= h / 2;
  }

  if (backgroundColor) {
    ctx.fillStyle = backgroundColor;
    if (!centered) ctx.fillRect(x - 5, y - h / 2, w, h);
    else ctx.fillRect(x, y, w, h);
  }
  ctx.fillStyle = color;
  ctx.textAlign = centered ? "center" : "left";
  ctx.textBaseline = "middle";
  ctx.fillText(text, pos[0], pos[1] + 15);
  return w;
}

function sameSide(px, py, ax, ay, bx, by, cx, cy) {
  const cross1 = (bx - ax) * (py - ay) - (by - ay) * (px - ax);
  const cross2 = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
  return cross1 * cross2 >= 0;
}

function pointInTriangle(px, py, ax, ay, bx, by, cx, cy) {
  return sameSide(px, py, ax, ay, bx, by, cx, cy) && sameSide(px, py, bx, by, cx, cy, ax, ay) && sameSide(px, py, cx, cy, ax, ay, bx, by);
}

function drawTriangle(ctx, p0, p1, p2, color = "white", width = 2) {
  ctx.beginPath();
  ctx.moveTo(p0[0], p0[1]);
  ctx.lineTo(p1[0], p1[1]);
  ctx.lineTo(p2[0], p2[1]);
  ctx.closePath();

  ctx.fillStyle = color;
  ctx.fill();

  ctx.lineWidth = width;
  ctx.strokeStyle = color;
  ctx.stroke();
}

function drawTriangleBorder(ctx, p0, p1, p2, color = "white", width = 2) {
  drawLine(ctx, p0, p1, color, width);
  drawLine(ctx, p1, p2, color, width);
  drawLine(ctx, p2, p0, color, width);
}

function drawRect(x, y, width, height, color, strokeColor, lineWidth) {
  var prev = ctx.lineWidth;
  ctx.lineWidth = lineWidth || 1;

  if (color) {
    ctx.fillStyle = color;
    ctx.fillRect(x, y, width, height);
    if (strokeColor) {
      ctx.strokeStyle = strokeColor;
      ctx.strokeRect(x, y, width, height);
    }
  } else if (strokeColor) {
    ctx.strokeStyle = strokeColor;
    ctx.strokeRect(x, y, width, height);
  }
  ctx.lineWidth = prev;
}

function drawSlider(ctx, pos, size, value, min, max, fillColor = "rgba(255, 255, 255, 1)", backgroundColor = "rgba(255, 255, 255, 0.23)") {
  drawRect(pos[0], pos[1], size[0], size[1], backgroundColor);

  var normalizedValue = (value - min) / (max - min);
  var x = size[0] * normalizedValue;
  drawRect(pos[0], pos[1], x, size[1], fillColor);
  var fixedValue = value;
  if (value !== Math.floor(value)) {
    fixedValue = Number(value).toFixed(2);
    if (fixedValue !== value) fixedValue += "...";
  }
  drawText(ctx, [pos[0] - 20, pos[1] - 12], fixedValue, "white", null, 10, true);
}
