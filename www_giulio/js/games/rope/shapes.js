class AirPusher {
  constructor(pos, angle = 0, force = 20, radius = 400) {
    this.pos = pos;
    this.angle = angle;
    this.baseForce = force;
    this.force = this.baseForce;
    this.radius = radius;
    this.airLines = [];
  }

  remove() {
    AirPusher.remove(this);
  }
  duplicate() {
    var newAirPusher = new AirPusher(new Vec2(this.pos.x + 5, this.pos.y + 5), this.angle, this.force, this.radius);
    airPushers.push(newAirPusher);
  }
  place(pos = new Vec2(mouse.pos.x, mouse.pos.y)) {
    this.pos = pos;
  }

  getWindForceAtPos(pos, mass = 0.1) {
    var dx = pos.x - this.pos.x;
    var dy = pos.y - this.pos.y;
    var dist = Math.sqrt(dx * dx + dy * dy);
    if (dist > this.radius) return pos;
    var angleToPoint = Math.atan2(dy, dx);
    var angleDiff = Math.abs(((angleToPoint - this.angle + Math.PI) % (2 * Math.PI)) - Math.PI);
    if (angleDiff > Math.PI / 6) return pos;
    var strength = (1 - dist / this.radius) * this.force * Math.random();
    // Use direction from airPusher to pos for wind direction
    var dirX = dx / (dist || 1);
    var dirY = dy / (dist || 1);
    var massFactor = Math.max(0.1, 1 - mass); // Clamp to prevent negative push
    var pushX = dirX * strength * massFactor;
    var pushY = dirY * strength * massFactor;
    return new Vec2(pos.x + pushX, pos.y + pushY);
  }

  render_rad() {
    var size = this.radius;
    var baseWidth = Math.max(20, size);
    var dx = Math.cos(this.angle),
      dy = Math.sin(this.angle);
    var px = -dy,
      py = dx;
    var baseCenter = [this.pos.x + dx * 20, this.pos.y + dy * 20];
    var tip = [baseCenter[0] + dx * size, baseCenter[1] + dy * size];
    var left = [tip[0] + px * baseWidth * 0.5, tip[1] + py * baseWidth * 0.5];
    var right = [tip[0] - px * baseWidth * 0.5, tip[1] - py * baseWidth * 0.5];
    if (!hovSegment && !selShape && !selDirPusher && !selAirPusher && !selSegment && pointInTriangle(mouse.pos.x, mouse.pos.y, baseCenter[0], baseCenter[1], left[0], left[1], right[0], right[1])) hovDirPusher = this;
    var clr = player === this || hovDirPusher === this ? "rgba(255, 255, 255, 0.2)" : "rgba(255, 255, 255, 0.1)";
    if (hovDirPusher === this && mouse.pressed) selDirPusher = this;
    drawTriangleBorder(ctx, baseCenter, left, right, clr, 2);
  }

  control() {
    player = this;
  }

  render() {
    if (player === this) {
      var dir = scale_v2(input.wasd, 4 * dt * 100);
      var newP = add_v2(this.pos, dir);
      var cp = clamp_v2(newP, new Vec2(window.innerWidth, window.innerHeight));
      this.place(cp);
      this.angle -= input.arrows.x * 0.05;
      if (this.angle < Math.PI) this.angle += Math.PI * 2;
      else if (this.angle > Math.PI) this.angle -= Math.PI * 2;
      this.radius = clamp(this.radius + input.arrows.y * 10, 1, window.innerWidth / 2);
    }
    var size = 20;
    this.force = clamp(this.force + r_range(-4, 4), this.baseForce * 0.5, this.baseForce * 1.5);
    var dx = Math.cos(this.angle);
    var dy = Math.sin(this.angle);
    var px = -dy;
    var py = dx;
    var tip = [this.pos.x + dx * size, this.pos.y + dy * size];
    var left = [this.pos.x + px * size * 0.5, this.pos.y + py * size * 0.5];
    var right = [this.pos.x - px * size * 0.5, this.pos.y - py * size * 0.5];
    if (!selDirPusher && !selAirPusher && !hovAirPusher && !hovDirPusher && pointInTriangle(mouse.pos.x, mouse.pos.y, tip[0], tip[1], left[0], left[1], right[0], right[1])) hovAirPusher = this;
    this.render_rad();

    if (!mouse.pressed || hovSegment) {
      if (selAirPusher === this) selAirPusher = null;
      if (selDirPusher === this) selDirPusher = null;
    } else {
      if (hovAirPusher === this) {
        if (mouse.clicked && input.keys["shift"]) {
          if (this === player) player = null;
          else this.control();
        } else selAirPusher = this;
      }
      if (selAirPusher === this) this.place();
      else {
        if (selDirPusher === this) {
          var diff = magnitude_v2(this.pos, mouse.pos);
          if (input.keys["shift"]) {
            var dx = mouse.pos.x - this.pos.x;
            var dy = mouse.pos.y - this.pos.y;
            var forward = Math.cos(this.angle) * dx + Math.sin(this.angle) * dy;
            var clampedDiff = clamp(diff, -20, 20);
            this.baseForce = forward >= 0 ? clampedDiff : -clampedDiff;
          } else {
            this.angle = Math.atan2(mouse.pos.y - this.pos.y, mouse.pos.x - this.pos.x);
            this.radius = Math.floor(diff);
          }
        }
      }
    }
    var clr = hovAirPusher === this || selAirPusher === this ? (this === player ? "blue" : "white") : this === player ? "rgba(0, 208, 255, 1)" : "grey";
    drawTriangleBorder(ctx, tip, left, right, clr, 2);
  }
  static remove(airPusher) {
    var idx = airPushers.indexOf(airPusher);
    if (idx === -1) return;
    if (selAirPusher === airPusher) selAirPusher = null;
    if (selDirPusher === airPusher) selDirPusher = null;
    airPushers.splice(idx, 1);
  }
  static instantiate(pos, angle = 0) {
    var airPusher = new AirPusher(new Vec2(pos.x, pos.y), angle);
    airPushers.push(airPusher);
    return airPusher;
  }
}

class Shape {
  constructor(pos, size, type = "SQUARE", color = getRandomColor(), angle = 0, _gravity = gravity) {
    this.gravity = _gravity;
    this.pos = pos;
    this.newPos;
    this.attachedSegments = [];
    this.size = size;
    this.type = type;
    this.color = color;
    this.angle = angle;
    this.angVel = 0;
    this.center = new Vec2(0, 0);
    this.vel = new Vec2(0, 0);
    this.bounceFactor = type === "CIRCLE" ? 0.8 : 0.5;
    this.dragFactor = type === "CIRCLE" ? 0.99 : 0.5;
    this.updateMass();
  }

  remove() {
    Shape.remove(this);
  }
  duplicate() {
    var newShape = new Shape(new Vec2(this.pos.x + 5, this.pos.y + 5), new Vec2(this.size.x, this.size.y), this.type, this.color, this.angle, new Vec2(this.gravity.x, this.gravity.y));
    shapes.push(newShape);
  }

  place(pos) {
    this.pos = pos;
  }

  rotate(deltaAngle) {
    this.angle += deltaAngle;
    while (this.angle <= -Math.PI) this.angle += Math.PI * 2;
    while (this.angle > Math.PI) this.angle -= Math.PI * 2;
  }

  resize(newSize) {
    if (typeof newSize === Vec2) this.size = new Vec2(newSize);
    else this.size = new Vec2(newSize, newSize);
    this.updateMass();
  }

  resolveCircleCollision(dotB) {
    let newP = this.newPos;
    let dotA = this;
    let radA = dotA.size.x;
    let radB = dotB.size.x;
    var massA = dotA.size.x;
    var massB = dotB.size.x;
    var posA = dotA.pos;
    var posB = dotB.pos;
    let dx = newP.x - posB.x;
    let dy = newP.y - posB.y;
    let distSq = dx * dx + dy * dy;
    let minDist = radA + radB + 0.1;
    if (distSq >= minDist * minDist) return false;
    let xDist = posB.x - posA.x;
    let yDist = posB.y - posA.y;
    let dist = Math.sqrt(xDist * xDist + yDist * yDist);
    const overlap = minDist - dist;
    const totalMass = massA + massB;
    let pushA = (massB / totalMass) * overlap;
    let pushB = (massA / totalMass) * overlap;
    newP.x -= xDist * (pushA / dist);
    newP.y -= yDist * (pushA / dist);
    dotB.pos.x += xDist * (pushB / dist);
    dotB.pos.y += yDist * (pushB / dist);

    const xVelocityDiff = dotA.vel.x - dotB.vel.x;
    const yVelocityDiff = dotA.vel.y - dotB.vel.y;
    if (xVelocityDiff * xDist + yVelocityDiff * yDist >= 0) {
      const angle = -Math.atan2(yDist, xDist);
      const u1 = rotate({ x: dotA.vel.x, y: dotA.vel.y }, angle);
      const u2 = rotate({ x: dotB.vel.x, y: dotB.vel.y }, angle);
      const m1 = massA;
      const m2 = massB;
      const v1 = { x: (u1.x * (m1 - m2) + 2 * m2 * u2.x) / (m1 + m2), y: u1.y };
      const v2 = { x: (u2.x * (m2 - m1) + 2 * m1 * u1.x) / (m1 + m2), y: u2.y };
      const vFinal1 = rotate(v1, -angle);
      const vFinal2 = rotate(v2, -angle);
      dotA.vel.x = vFinal1.x;
      dotA.vel.y = vFinal1.y;
      dotB.vel.x = vFinal2.x;
      dotB.vel.y = vFinal2.y;
    }
    this.newPos = newP;
    return true;
  }

  resolveBoxCollision(boxB) {
    let newP = this.newPos;
    let boxA = this;
    const aRight = newP.x + boxA.size.x;
    const aBottom = newP.y + boxA.size.y;
    const bRight = boxB.pos.x + boxB.size.x;
    const bBottom = boxB.pos.y + boxB.size.y;
    if (newP.x >= bRight || aRight <= boxB.pos.x || newP.y >= bBottom || aBottom <= boxB.pos.y) {
      return false;
    }
    const overlapLeft = aRight - boxB.pos.x;
    const overlapRight = bRight - newP.x;
    const overlapTop = aBottom - boxB.pos.y;
    const overlapBottom = bBottom - newP.y;
    // Find minimum penetration axis
    const minOverlapX = Math.min(overlapLeft, overlapRight);
    const minOverlapY = Math.min(overlapTop, overlapBottom);
    // Mass-based separation
    const massA = boxA.size.x * boxA.size.y;
    const massB = boxB.size.x * boxB.size.y;
    const totalMass = massA + massB;
    const pushRatioA = massB / totalMass;
    const pushRatioB = massA / totalMass;

    // Separate along axis of minimum penetration
    if (minOverlapX < minOverlapY) {
      // Horizontal collision
      if (overlapLeft < overlapRight) {
        // Hit from left
        newP.x -= minOverlapX * pushRatioA;
        if (!boxB.isAtBorder(boxB.pos, "right")) boxB.pos.x += minOverlapX * pushRatioB;
      } else {
        // Hit from right
        newP.x += minOverlapX * pushRatioA;
        if (!boxB.isAtBorder(boxB.pos, "left")) boxB.pos.x -= minOverlapX * pushRatioB;
      }

      // Apply elastic collision on X axis
      const relVelX = boxA.vel.x - boxB.vel.x;
      if ((overlapLeft < overlapRight && relVelX > 0) || (overlapLeft >= overlapRight && relVelX < 0)) {
        const m1 = massA;
        const m2 = massB;
        const v1 = boxA.vel.x;
        const v2 = boxB.vel.x;
        boxA.vel.x = ((m1 - m2) * v1 + 2 * m2 * v2) / (m1 + m2);
        boxB.vel.x = ((m2 - m1) * v2 + 2 * m1 * v1) / (m1 + m2);
      }
    } else {
      // Vertical collision
      if (overlapTop < overlapBottom) {
        // Hit from top
        newP.y -= minOverlapY * pushRatioA;
        if (!boxB.isAtBorder(boxB.pos, "down")) boxB.pos.y += minOverlapY * pushRatioB;
        boxA.vel.x *= this.dragFactor;
      } else {
        // Hit from bottom
        newP.y += minOverlapY * pushRatioA;
        if (!boxB.isAtBorder(boxB.pos, "up")) boxB.pos.y -= minOverlapY * pushRatioB;
      }
      const relVelY = boxA.vel.y - boxB.vel.y;
      if ((overlapTop < overlapBottom && relVelY > 0) || (overlapTop >= overlapBottom && relVelY < 0)) {
        const m1 = massA;
        const m2 = massB;
        const v1 = boxA.vel.y;
        const v2 = boxB.vel.y;
        boxA.vel.y = ((m1 - m2) * v1 + 2 * m2 * v2) / (m1 + m2);
        boxB.vel.y = ((m2 - m1) * v2 + 2 * m1 * v1) / (m1 + m2);
      }
    }
    this.newPos = newP;
    return true;
  }

  resolveCircleBoxCollision(b) {
    let newP = this.newPos;
    var a = this;
    var circle = a.type === "CIRCLE" ? a : b.type === "CIRCLE" ? b : null;
    var box = a.type === "SQUARE" ? a : b.type === "SQUARE" ? b : null;
    if (!circle || !box) return false;

    // Find closest point on box to circle center
    const boxLeft = box.pos.x;
    const boxRight = box.pos.x + box.size.x;
    const boxTop = box.pos.y;
    const boxBottom = box.pos.y + box.size.y;

    const closestX = Math.max(boxLeft, Math.min(circle.pos.x, boxRight));
    const closestY = Math.max(boxTop, Math.min(circle.pos.y, boxBottom));

    // Calculate separation vector
    const dx = circle.pos.x - closestX;
    const dy = circle.pos.y - closestY;
    const distSq = dx * dx + dy * dy;
    const radius = circle.size.x;
    const radSq = radius * radius;

    // No collision if outside sphere
    if (distSq >= radSq) return false;

    const dist = Math.sqrt(distSq) || 0.0001;
    const overlap = radius - dist;

    // Normalize separation vector
    const nx = dx / dist;
    const ny = dy / dist;

    const totalMass = circle.mass + box.mass;
    const pushRatioCircle = box.mass / totalMass;
    const pushRatioBox = circle.mass / totalMass;

    // Separate circle and box (check borders)
    const pushDirX = nx > 0 ? "right" : "left";
    const pushDirY = ny > 0 ? "down" : "up";
    if (circle === a) {
      newP.x += nx * overlap * pushRatioCircle;
      newP.y += ny * overlap * pushRatioCircle;
      if (!box.isAtBorder(box.pos, pushDirX)) box.pos.x -= nx * overlap * pushRatioBox;
      if (!box.isAtBorder(box.pos, pushDirY)) box.pos.y -= ny * overlap * pushRatioBox;
    } else {
      circle.pos.x += nx * overlap * pushRatioCircle;
      circle.pos.y += ny * overlap * pushRatioCircle;
      newP.x -= nx * overlap * pushRatioBox;
      newP.y -= ny * overlap * pushRatioBox;
    }

    // Apply elastic collision along normal
    const relVelX = circle.vel.x - box.vel.x;
    const relVelY = circle.vel.y - box.vel.y;
    const velAlongNormal = relVelX * nx + relVelY * ny;

    if (velAlongNormal < 0) {
      // Moving towards each other
      const m1 = circle.mass;
      const m2 = box.mass;
      const impulse = (2 * velAlongNormal) / (m1 + m2);

      circle.vel.x -= impulse * m2 * nx;
      circle.vel.y -= impulse * m2 * ny;
      box.vel.x += impulse * m1 * nx;
      box.vel.y += impulse * m1 * ny;
    }
    this.newPos = newP;
    return true;
  }

  updateShapeCollisions() {
    var cs = colGrid.getAroundPos(this.center.x, this.center.y, 1);
    var colAmount = 0;
    for (const s of cs) {
      if (s === this || s.rope) continue;
      var collided;
      if (this.type === "CIRCLE" && s.type === "CIRCLE") {
        collided = this.resolveCircleCollision(s);
      } else if (this.type === "SQUARE" && s.type === "SQUARE") collided = this.resolveBoxCollision(s);
      else collided = this.resolveCircleBoxCollision(s);
      colAmount += collided;
    }
    return colAmount;
  }

  getRotatedCorners() {}

  updateBorderCollisions() {
    var pos = this.newPos;
    var size = this.size;
    const bounceMult = Math.max(0.3, this.bounceFactor * (1 - this.mass / 10000));

    if (this.type === "CIRCLE") {
      var minX = this.size.x;
      var maxX = window.innerWidth - this.size.x;

      var minY = this.size.x;
      var maxY = window.innerHeight - this.size.x;

      if (pos.x < minX) {
        if (this.vel.x < 0) this.vel.x *= -bounceMult;
        pos.x = minX + 1;
      } else if (pos.x > maxX) {
        if (this.vel.x > 0) this.vel.x *= -bounceMult;
        pos.x = maxX;
        this.vel.x *= this.dragFactor;
      }
      if (pos.y < minY) {
        if (this.vel.y < 0) this.vel.y *= -bounceMult;
        pos.y = minY + 1;
      } else if (pos.y > maxY) {
        if (this.vel.y > 0) this.vel.y *= -bounceMult;
        pos.y = maxY;
        this.vel.x *= this.dragFactor;
      }
    } else {
      const corners = [new Vec2(pos.x, pos.y), new Vec2(pos.x + size.x, pos.y), new Vec2(pos.x, pos.y + size.y), new Vec2(pos.x + size.x, pos.y + size.y)];
      const rotatedCorners = corners.map((c) => rotate_v2(c, this.center, this.angle));
      const maxX = Math.max(...rotatedCorners.map((c) => c.x));
      const minX = Math.min(...rotatedCorners.map((c) => c.x));
      const maxY = Math.max(...rotatedCorners.map((c) => c.y));
      const minY = Math.min(...rotatedCorners.map((c) => c.y));
      if (minX < 0) {
        pos.x += Math.abs(minX) + 2;
      } else if (maxX > window.innerWidth) {
        pos.x -= maxX - window.innerWidth + 2;
      }
      if (minY < 0) {
        pos.y += Math.abs(minY) + 2;
      } else if (maxY > window.innerHeight) {
        pos.y -= maxY - window.innerHeight + 2;
      }

      const finalCenter = new Vec2(pos.x + size.x / 2, pos.y + size.y / 2);
      const finalCorners = corners.map((c) => rotate_v2(c, finalCenter, this.angle));
      const finalMaxX = Math.max(...finalCorners.map((c) => c.x));
      const finalMinX = Math.min(...finalCorners.map((c) => c.x));
      const finalMaxY = Math.max(...finalCorners.map((c) => c.y));
      const finalMinY = Math.min(...finalCorners.map((c) => c.y));

      const touchingLeft = finalMinX < 0;
      const touchingRight = finalMaxX > window.innerWidth;
      const touchingTop = finalMinY < 0;
      const touchingBottom = finalMaxY > window.innerHeight;

      if ((touchingLeft && this.vel.x < 0) || (touchingRight && this.vel.x > 0)) {
        if (Math.sign(this.vel.x) < 0.1) this.grounded = true;
        this.vel.x *= -bounceMult;
        this.vel = scale_v2(this.vel, this.dragFactor);
      }
      if ((touchingTop && this.vel.y < 0) || (touchingBottom && this.vel.y > 0)) {
        if (Math.sign(this.vel.y) <= this.gravity.y) {
          this.grounded = true;
          this.vel.y = 0;
        } else this.vel.y *= -bounceMult;
        this.vel = scale_v2(this.vel, this.dragFactor);
      }
    }
  }

  isAtBorder(pos, direction) {
    const minX = this.type === "CIRCLE" ? this.size.x : 0;
    const maxX = window.innerWidth - (this.type === "CIRCLE" ? this.size.x : this.size.x);
    const minY = this.type === "CIRCLE" ? this.size.x : 0;
    const maxY = window.innerHeight - (this.type === "CIRCLE" ? this.size.x : this.size.y);

    if (direction === "left") return pos.x <= minX;
    if (direction === "right") return pos.x >= maxX;
    if (direction === "up") return pos.y <= minY;
    if (direction === "down") return pos.y >= maxY;
    return false;
  }

  updateMass() {
    this.mass = this.type === "CIRCLE" ? this.size.x * this.size.x : this.size.x * this.size.y;
  }

  updateVelocity() {
    // If grounded and velocities are near zero, suppress gravity entirely
    this.vel.x += this.gravity.x * dt * 10;
    this.vel.y += this.gravity.y * dt * 10;

    for (const a of airPushers) {
      var oldPos = new Vec2(this.pos.x, this.pos.y);
      var pushedPos = a.getWindForceAtPos(oldPos, this.mass / 100);
      var windPush = sub_v2(pushedPos, oldPos);
      this.vel.x += windPush.x / dt;
      this.vel.y += windPush.y / dt;
    }
    this.newPos = add_v2(this.newPos, scale_v2(this.vel, dt));
  }

  updateAngle() {
    if (this.type === "CIRCLE") {
      this.rotate((this.vel.x * dt) / this.size.x);
    } else {
    }
  }

  updateSelected() {
    if (mouse.pressed) this.newPos = new Vec2(mouse.pos.x - this.size.x / 2, mouse.pos.y - this.size.y / 2);
    else {
      this.vel = new Vec2(-mouse.delta.x * 40, -mouse.delta.y * 40);
      selShape = null;
    }
  }

  updateCenter() {
    if (this.type === "CIRCLE") {
      this.center.x = this.pos.x;
      this.center.y = this.pos.y;
    } else if (this.type === "SQUARE") {
      this.center.x = this.pos.x + this.size.x / 2;
      this.center.y = this.pos.y + this.size.y / 2;
    }
  }

  updateHover() {
    if (this.type === "SQUARE" && pointInRect(mouse.pos, this.pos, this.size)) hovShape = this;
    else if (this.type === "CIRCLE" && pointInCircle(mouse.pos, new Vec2(this.pos.x, this.pos.y), this.size.x)) hovShape = this;
  }

  update() {
    this.grounded = false;
    this.newPos = new Vec2(this.pos.x, this.pos.y);
    if (selShape === this) this.updateSelected();
    else {
      this.updateVelocity();
      this.updateAngle();
    }
    this.updateBorderCollisions();
    this.updateShapeCollisions();
    this.pos = this.newPos;
    if (!hovShape && !selShape) this.updateHover();
    this.updateCenter();
  }

  render() {
    var isSel = contextMenu.shape === this || hovShape === this || selShape === this;
    var curClr = isSel ? this.color : addColor(this.color, "black", 0.3);
    switch (this.type) {
      case "SQUARE":
        ctx.save();
        const centerX = this.pos.x + this.size.x / 2;
        const centerY = this.pos.y + this.size.y / 2;
        ctx.translate(centerX, centerY);
        ctx.rotate(this.angle);
        drawRect(-this.size.x / 2, -this.size.y / 2, this.size.x, this.size.y, curClr);
        ctx.restore();
        break;
      case "CIRCLE":
        drawCircle2(ctx, [this.pos.x, this.pos.y], this.size.x, curClr, isSel ? "white" : "rgba(0,0,0,0)", 2);
        var color = addColor(curClr, "black", 0.4);
        for (let i = 0; i < 2; i++) {
          const angle = this.angle + (i * Math.PI) / 2;
          const dir = new Vec2(Math.cos(angle), Math.sin(angle));
          const start = add_v2(this.pos, scale_v2(dir, this.size.x));
          const end = add_v2(this.pos, scale_v2(dir, -this.size.x));
          drawLine(ctx, [start.x, start.y], [end.x, end.y], color, 4);
        }
        drawCircle2(ctx, [this.pos.x, this.pos.y], this.size.x * 0.25, color, isSel ? "white" : "rgba(0,0,0,0)", 2);
        break;
      case "TRIANGLE":
        ctx.save();
        ctx.translate(this.pos.x + this.size.x / 2, this.pos.y + this.size.y / 2);
        ctx.rotate(this.angle);
        const h = (this.size.y * Math.sqrt(3)) / 2;
        const p0 = [0, (-2 * h) / 3];
        const p1 = [-this.size.x / 2, h / 3];
        const p2 = [this.size.x / 2, h / 3];
        drawTriangle(ctx, p0, p1, p2, curClr, 2);
        ctx.restore();
        break;
    }
  }
  static resize(g_size) {
    for (const s of shapes) {
      var newSize = new Vec2(g_size.x || s.size.x, g_size.y || s.size.y);
      s.resize(newSize);
    }
    ShapeScale = g_size.x;
  }
  static removeAll() {
    for (const s of shapes) s.remove();
  }

  static remove(shape) {
    var idx = shapes.indexOf(shape);
    if (idx === -1) return;
    if (shapes[idx].attach) {
      shapes[idx].attach.attachToShape(null);
      shapes[idx].isAnchor = false;
    }
    if (selShape === shape) selShape = null;
    shapes.splice(idx, 1);
  }

  static setGlobalGravity(newGravity) {
    for (const s of shapes) s.gravity = newGravity;
  }

  static instantiate(type = "CIRCLE", pos = mouse.pos, size = new Vec2(r_range(20, 80), r_range(20, 80))) {
    var shape = new Shape(new Vec2(pos.x, pos.y), new Vec2(size.x, size.y), type);
    shapes.push(shape);
    return shape;
  }
}
