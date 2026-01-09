class Vec2 {
  constructor(x, y) {
    this.x = x;
    this.y = y;
  }
  debugOnCanvas(ctx, pos = [50, 50], color = "red") {
    drawText(ctx, pos, `x${this.x} y${this.y}`, color);
  }

  debug(label = null) {
    if (label) console.warn(`${label} x${this.x} y${this.y}`);
    else console.warn(`x${this.x} y${this.y}`);
  }
}

function magnitude_v2(a, b) {
  return Math.sqrt(Math.pow(a.x - b.x, 2) + Math.pow(a.y - b.y, 2));
}
function sub_v2(a, b) {
  return new Vec2(a.x - b.x, a.y - b.y);
}
function normalize_v2(v) {
  var mag = Math.sqrt(v.x * v.x + v.y * v.y);
  if (mag === 0) return new Vec2(0, 0);
  return new Vec2(v.x / mag, v.y / mag);
}
function mult_v2(a, b) {
  return new Vec2(a.x * b.x, a.y * b.y);
}
function add_v2(a, b) {
  return new Vec2(a.x + b.x, a.y + b.y);
}

function scale_v2(v, scalar) {
  return new Vec2(v.x * scalar, v.y * scalar);
}

function clamp_v2(v, limits) {
  return new Vec2(clamp(v.x, -limits.x, limits.x), clamp(v.y, -limits.y, limits.y));
}

function clamp(val, min, max) {
  return val < min ? min : val > max ? max : val;
}

function lerp(a, b, t) {
  return a + (b - a) * t;
}

function rand_v2(min = new Vec2(0, 0), max = new Vec2(window.innerWidth, window.innerHeight)) {
  return new Vec2(r_range(min.x, max.x), r_range(min.y, max.y));
}

function compare_v2(a, b, scalar) {
  return magnitude_v2(a, b) < scalar;
}

function pointInRect(point, rPos, rSize) {
  return point.x >= rPos.x && point.x <= rPos.x + rSize.x && point.y >= rPos.y && point.y <= rPos.y + rSize.y;
}

function circleInRect(circle, radius, rPos, rSize) {
  // Find closest point on rectangle to circle center
  var closestX = Math.max(rPos.x, Math.min(circle.x, rPos.x + rSize.x));
  var closestY = Math.max(rPos.y, Math.min(circle.y, rPos.y + rSize.y));
  // Calculate distance from circle center to closest point
  var dx = circle.x - closestX;
  var dy = circle.y - closestY;
  var distSq = dx * dx + dy * dy;
  return distSq < radius * radius;
}

function pointInCircle(point, circlePos, circleRadius) {
  var dx = point.x - circlePos.x;
  var dy = point.y - circlePos.y;
  return dx * dx + dy * dy <= circleRadius * circleRadius;
}

function circleInCircle(innerPos, innerRadius, outerPos, outerRadius) {
  if (outerRadius < innerRadius) return false;
  var dx = innerPos.x - outerPos.x;
  var dy = innerPos.y - outerPos.y;
  var distSq = dx * dx + dy * dy;
  var allowed = (outerRadius - innerRadius) * (outerRadius - innerRadius);
  return distSq <= allowed;
}

function circleOverlap(pos1, radius1, pos2, radius2) {
  var dx = pos1.x - pos2.x;
  var dy = pos1.y - pos2.y;
  var distSq = dx * dx + dy * dy;
  var minDist = radius1 + radius2;
  return distSq < minDist * minDist;
}

function r_range(min, max) {
  return Math.random() * (max - min) + min;
}

function r_range_int(min, max) {
  return Math.floor(Math.random() * (max - min) + min);
}

function rotate(velocity, angle) {
  return {
    x: velocity.x * Math.cos(angle) - velocity.y * Math.sin(angle),
    y: velocity.x * Math.sin(angle) + velocity.y * Math.cos(angle),
  };
}

function rotate_v2(pos, center, angle) {
  const x = pos.x - center.x;
  const y = pos.y - center.y;
  const cos = Math.cos(angle);
  const sin = Math.sin(angle);
  const newX = x * cos - y * sin;
  const newY = x * sin + y * cos;
  return new Vec2(newX + center.x, newY + center.y);
}

function clampAngle(dir, minA, maxA) {
  let a = Math.atan2(dir.y, dir.x);
  if (a < minA) a = minA;
  if (a > maxA) a = maxA;
  return new Vec2(Math.cos(a), Math.sin(a));
}
