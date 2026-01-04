class RopeSegment {
  constructor(pos, rope) {
    this.pos = pos;
    this.prevPos = pos;
    this.rope = rope;
    this.isAnchor = false;
    this.anchorPos = null;
    this.anchorObject = null;
    this.anchorOffset = new Vec2(0, 0);
  }

  getAnchorOffset(anchorObject) {
    if (!anchorObject) return null;
    var angle = Math.atan2(this.pos.y - anchorObject.pos.y, this.pos.x - anchorObject.pos.x);
    var dir = new Vec2(Math.cos(angle), Math.sin(angle));
    if (anchorObject.type === "CIRCLE") {
      return scale_v2(dir, anchorObject.size.x * 1.2);
    } else if (anchorObject.type === "SQUARE") {
      const hw = anchorObject.size.x / 2;
      const hh = anchorObject.size.y / 2;
      if (Math.abs(dir.x) > Math.abs(dir.y)) {
        return new Vec2(dir.x > 0 ? hw : -hw, dir.y * hh);
      } else {
        return new Vec2(dir.x * hw, dir.y > 0 ? hh : -hh);
      }
    }
    return null;
  }

  attachToShape(anchorObject = null, anchorOffset = this.getAnchorOffset(anchorObject)) {
    if (anchorObject.child) return;
    this.anchorObject = anchorObject;
    this.anchorObject.child = this;
    if (anchorOffset) this.anchorOffset = anchorOffset;
  }

  setAnchor(pos = new Vec2(this.pos), _anchorObject = null) {
    if (_anchorObject) {
      this.anchorObject = _anchorObject;
      this.isAnchor = true;
    } else if (!pos) {
      this.isAnchor = false;
      this.anchorPos = null;
    } else {
      this.isAnchor = true;
      this.anchorPos = pos;
    }
  }

  place(newPos) {
    this.pos = newPos;
    this.anchorPos = newPos;
  }

  applyAnchorPullForces() {
    if (!this.anchorObject) return;

    // Calculate where segment should be attached to anchor
    this.anchorPos = add_v2(this.anchorObject.pos, this.anchorOffset);

    // Check if this segment is being overstretched by neighboring segments
    const neighbors = this.rope.segments;
    const myIndex = neighbors.indexOf(this);
    let totalPull = new Vec2(0, 0);

    if (myIndex > 0) {
      const prev = neighbors[myIndex - 1];
      const dx = prev.pos.x - this.anchorPos.x;
      const dy = prev.pos.y - this.anchorPos.y;
      const dist = Math.sqrt(dx * dx + dy * dy);
      const overstretch = Math.max(0, dist - this.rope.segSpace);
      if (overstretch > 0) {
        totalPull.x += (dx / dist) * overstretch;
        totalPull.y += (dy / dist) * overstretch;
      }
    }

    if (myIndex < neighbors.length - 1) {
      const next = neighbors[myIndex + 1];
      const dx = next.pos.x - this.anchorPos.x;
      const dy = next.pos.y - this.anchorPos.y;
      const dist = Math.sqrt(dx * dx + dy * dy);
      const overstretch = Math.max(0, dist - this.rope.segSpace);
      if (overstretch > 0) {
        totalPull.x += (dx / dist) * overstretch;
        totalPull.y += (dy / dist) * overstretch;
      }
    }

    const pullMagnitude = Math.sqrt(totalPull.x * totalPull.x + totalPull.y * totalPull.y);
    if (pullMagnitude > 0) {
      const stiffness = 0.1;
      this.anchorObject.pos.x += totalPull.x * stiffness;
      this.anchorObject.pos.y += totalPull.y * stiffness;

      this.anchorPos = add_v2(this.anchorObject.pos, this.anchorOffset);
    }

    // Segment follows final anchor position
    this.pos = this.anchorPos;
  }

  update() {
    if (this.isAnchor || this === selSegment) return;

    if (this.anchorObject) {
      this.applyAnchorPullForces();
      return; // Anchored segments don't do regular physics
    }

    var rope = this.rope;
    var movement = sub_v2(this.pos, this.prevPos);
    var vel = scale_v2(movement, rope.damp * r_range(0.8, 1.2));
    this.prevPos = new Vec2(this.pos.x, this.pos.y);
    var newPos = add_v2(this.pos, vel);
    var grav = rope.gravity || new Vec2(0, 0);
    newPos.x += (grav.x || 0) * dt;
    newPos.y += (grav.y || 0) * dt;
    for (const a of airPushers) newPos = a.getWindForceAtPos(newPos, rope.thick / 100);

    const floorY = window.innerHeight - rope.thick / 2;
    if (newPos.y >= floorY) {
      const velX = newPos.x - this.prevPos.x;
      if (rope.groundFriction) newPos.x = this.pos.x + velX * rope.groundFriction;
      newPos.y = floorY;
      rope.grounded = true;
    }
    const ceilingY = rope.thick / 2;
    if (newPos.y <= ceilingY) {
      const velX = newPos.x - this.prevPos.x;
      newPos.x = this.pos.x + velX * rope.groundFriction;
      newPos.y = ceilingY;
    }

    newPos.x = clamp(newPos.x, rope.thick, window.innerWidth - rope.thick);
    this.pos = newPos;
  }
}

class Rope {
  constructor(start, end = null, color = getRandomColor(), thick = r_range(1, 20), _segAmount = 50, _segSpace = segSpace, _damp = dampingFactor) {
    this.segAmount = _segAmount;
    this.segSpace = _segSpace;
    this.damp = _damp;
    this.gravity = new Vec2(gravity.x, gravity.y);
    this.color = color;
    this.groundFriction = ropeGroundFriction;
    this.color2 = getRandomColor();
    this.thick = thick;
    this.init(start, end);
  }

  duplicate() {
    var start = new Vec2(this.segments[0].pos.x + 5, this.segments[0].pos.y + 5);
    var last = this.segments[this.segments.length - 1];
    var end = last.isAnchor ? null : new Vec2(last.pos.x + 5, last.pos.y + 5);
    var newRope = new Rope(start, end, this.color, this.thick, this.segAmount, this.segSpace, this.damp);
    ropes.push(newRope);
  }
  setAnchor(indices) {
    for (const index of indices) {
      this.segments[index].setAnchor();
    }
  }
  remove() {
    Rope.remove(this);
  }

  setNewSegAmount(newSegAmount) {
    if (newSegAmount === this.segAmount) return;
    var start = this.segments[0].pos;
    var end = null;
    var last = this.segments[this.segments.length - 1];
    if (last.isAnchor) end = new Vec2(last.pos.x + this.thick / 2, last.pos.y);
    var prevAmount = this.segAmount;
    var oldPositions = [];
    for (let i = 0; i < prevAmount; i++) {
      let p = this.segments[i].pos;
      oldPositions.push([p[0], p[1]]);
    }

    this.segAmount = newSegAmount;
    this.init(start, end);
    if (oldPositions.length === 0) return;
    return;
    var min = Math.min(oldPositions.length, newSegAmount);
    for (let i = 0; i < min; i++) this.segments[i].pos = new Vec2(oldPositions[i].x, oldPositions[i].y);
    for (let i = 0; i >= min && i < newSegAmount; i++) this.segments[i].pos = new Vec2(oldPositions[min].x, oldPositions[min].y);
  }

  getSegmentOverstretchAmount(seg, x, y, tolerance = 0.1) {
    var maxStretch = this.segSpace * (1 + tolerance);
    var idx = this.segments.indexOf(seg);
    var maxOverstretch = 0;
    if (idx > 0) {
      var prev = this.segments[idx - 1];
      if (!prev.isAnchor) {
        var dPrev = Math.hypot(x - prev.pos.x, y - prev.pos.y);
        if (dPrev > maxStretch) maxOverstretch = Math.max(maxOverstretch, dPrev - maxStretch);
      }
    }
    if (idx < this.segments.length - 1) {
      var next = this.segments[idx + 1];
      if (!next.isAnchor) {
        var dNext = Math.hypot(x - next.pos.x, y - next.pos.y);
        if (dNext > maxStretch) maxOverstretch = Math.max(maxOverstretch, dNext - maxStretch);
      }
    }
    return maxOverstretch;
  }

  // Returns the normal vector (as {x, y}) of a segment, perpendicular to the direction between its neighbors
  getSegmentNormal(seg) {
    const idx = this.segments.indexOf(seg);
    let dir = null;
    if (idx > 0 && idx < this.segments.length - 1) {
      const prev = this.segments[idx - 1].pos;
      const next = this.segments[idx + 1].pos;
      dir = { x: next.x - prev.x, y: next.y - prev.y };
    } else if (idx < this.segments.length - 1) {
      const next = this.segments[idx + 1].pos;
      dir = { x: next.x - seg.pos.x, y: next.y - seg.pos.y };
    } else if (idx > 0) {
      const prev = this.segments[idx - 1].pos;
      dir = { x: seg.pos.x - prev.x, y: seg.pos.y - prev.y };
    } else {
      dir = { x: 0, y: -1 };
    }
    const mag = Math.sqrt(dir.x * dir.x + dir.y * dir.y) || 1;
    return { x: -dir.y / mag, y: dir.x / mag };
  }

  init(start, end) {
    if (end && end.x > window.innerWidth - this.thick) {
      end.x = start.x - this.segAmount * this.segSpace;
    }
    start.x = clamp(start.x, this.thick, window.innerWidth - this.thick);
    start.y = clamp(start.y, this.thick, window.innerHeight - this.thick);

    this.segments = [];
    var step = 1 / this.segAmount;
    var t = 0;
    var lastPos = end ? end : new Vec2(start.x + this.segSpace * this.segAmount, start.y);
    for (let i = 0; i < this.segAmount; i++) {
      var pos = new Vec2(lerp(start.x, lastPos.x, t), lerp(start.y, lastPos.y, t));
      var newSegment = new RopeSegment(pos, this);
      this.segments.push(newSegment);
      t += step;
    }
    if (end && start) {
      this.setAnchor([0, this.segAmount - 1]);
    } else if (start && !end) {
      this.setAnchor([0]);
    }
  }

  applyCollisions(n) {
    for (let i = 0; i < this.segments.length; i++) {
      let a = this.segments[i];
      if (a.isAnchor) continue;
      for (const b of shapes) this.handleShapeCollision(a, b);
      if (n % SelfCollisionsInterval === 0) {
        var closeSegments = colGrid.getAtPos(a.pos.x, a.pos.y);
        for (const b of closeSegments) {
          if (b.rope === undefined) continue;
          if (b.isAnchor) continue;
          if (Math.abs(this.segments.indexOf(a) - this.segments.indexOf(b)) > 2) {
            this.handleSegCollision(a, b);
          }
        }
      }
    }
  }

  update() {
    this.grounded = false;
    for (var i = 0; i < this.segments.length; i++) this.segments[i].update();
    for (let n = 0; n < numOfConstraintsRuns; n++) {
      this.applyConstraits();
      if (colGrid.active && n % collisionSegmentInteval === 0) this.applyCollisions(n);
    }
  }

  handleShapeCollision(seg, shape = null) {
    if (seg.anchorObject === shape) return;
    switch (shape.type) {
      case "SQUARE": {
        var shapeSize = new Vec2(shape.size.x + this.thick, shape.size.y + this.thick);
        var shapePos = new Vec2(shape.pos.x - this.thick / 2, shape.pos.y - this.thick / 2);
        if (!pointInRect(seg.pos, shapePos, shapeSize)) return false;
        if (seg === selSegment && mouse.pressed && !seg.anchorObject) {
          seg.attachToShape(shape);
          return;
        }
        let left = Math.abs(seg.pos.x - shapePos.x);
        let right = Math.abs(seg.pos.x - (shapePos.x + shapeSize.x));
        let top = Math.abs(seg.pos.y - shapePos.y);
        let bottom = Math.abs(seg.pos.y - (shapePos.y + shapeSize.y));
        let minDist = Math.min(left, right, top, bottom);
        if (minDist === left) seg.pos.x = shapePos.x;
        else if (minDist === right) seg.pos.x = shapePos.x + shapeSize.x;
        else if (minDist === top) seg.pos.y = shapePos.y;
        else if (minDist === bottom) seg.pos.y = shapePos.y + shapeSize.y;
        if (shape !== selShape && this.segments[this.segments.length - 1].isAnchor) {
          var overstretch = this.getSegmentOverstretchAmount(seg, seg.pos.x, seg.pos.y, 0.8);
          if (overstretch > 0) {
            const dir = this.getSegmentNormal(seg);
            // shape.angle = Math.atan2(dir.y, dir.x);
            if (shape === selShape && input.keys["shift"]) seg.attachToShape(shape);
            var damping = Math.min(1, overstretch / (this.segSpace * 0.3)); // Clamp to 0-1
            shape.vel.x *= 0.5 - damping * 0.4;
            shape.vel.y *= 0.9 - damping * 0.3;

            if (Math.abs(dir.x) > Math.abs(dir.y)) {
              shape.vel.x *= -1;
              shape.vel.y *= 0.99;
              var pushX = damping * 2;
              if (shape.pos.x + shape.size.x / 2 < seg.pos.x) shape.pos.x -= pushX;
              else shape.pos.x += pushX;
            } else {
              shape.vel.y *= -1;
              shape.vel.x *= 0.99;
              var pushY = damping * 2;
              if (shape.pos.y + shape.size.y / 2 < seg.pos.y) shape.pos.y -= pushY;
              else shape.pos.y += pushY;
            }
          }
        }
        break;
      }
      case "CIRCLE": {
        var discCenter = new Vec2(shape.pos.x, shape.pos.y);
        var discRad = shape.size.x + this.thick / 2;
        var dx = seg.pos.x - discCenter.x;
        var dy = seg.pos.y - discCenter.y;
        var distSq = dx * dx + dy * dy;
        var radSq = discRad * discRad;
        if (distSq >= radSq) return false;
        if (seg === selSegment && mouse.pressed && !seg.anchorObject) {
          seg.attachToShape(shape);
          return;
        }
        var dist = Math.sqrt(distSq) || 0.0001;
        var pushDist = discRad;
        seg.pos.x = discCenter.x + (dx / dist) * pushDist;
        seg.pos.y = discCenter.y + (dy / dist) * pushDist;
        if (shape === selShape) {
          if (input.keys["shift"]) seg.attachToShape(shape);
        } else {
          var overstretch = this.getSegmentOverstretchAmount(seg, seg.pos.x, seg.pos.y, 0.8);
          if (overstretch > 0) {
            if (!this.isSnake && !this.segments[this.segments.length - 1].isAnchor) {
              shape.vel.x *= 0.9999;
              shape.vel.y *= 0.9999;
              return true;
            }
            const dir = this.getSegmentNormal(seg);
            var damping = Math.min(1, overstretch / (this.segSpace * 0.3));
            shape.vel.x *= 0.5 - damping * 0.4;
            shape.vel.y *= 0.9 - damping * 0.3;

            if (Math.abs(dir.x) > Math.abs(dir.y)) {
              shape.vel.x *= -1;
              shape.vel.y *= 0.99;
              var pushX = damping * 2;
              if (shape.pos.x + shape.size.x / 2 < seg.pos.x) shape.pos.x -= pushX;
              else shape.pos.x += pushX;
            } else {
              shape.vel.y *= -1;
              shape.vel.x *= 0.99;
              var pushY = damping * 2;
              if (shape.pos.y + shape.size.y / 2 < seg.pos.y) shape.pos.y -= pushY;
              else shape.pos.y += pushY;
            }
          }
        }

        break;
      }
      case "TRIANGLE":
        break;
    }
    return true;
  }

  handleSegCollision(a, b, minDist = this.thick * 0.75) {
    let dx = a.pos.x - b.pos.x;
    let dy = a.pos.y - b.pos.y;
    let dist = Math.sqrt(dx * dx + dy * dy);
    if (dist < minDist && dist > 0.01) {
      let overlap = minDist - dist;
      let nx = dx / dist;
      let ny = dy / dist;
      let correction = overlap * 0.5 * overlapFactor;
      a.pos.x += nx * correction;
      a.pos.y += ny * correction;
      b.pos.x -= nx * correction;
      b.pos.y -= ny * correction;
    }
  }

  applyConstraits() {
    for (let i = 0; i < this.segAmount - 1; i++) {
      var seg = this.segments[i];
      var nextSegment = this.segments[i + 1];
      var segIsAnchor = seg.isAnchor;
      var nextIsAnchor = nextSegment.isAnchor;
      var delta = sub_v2(seg.pos, nextSegment.pos);
      var dist = Math.sqrt(delta.x * delta.x + delta.y * delta.y);
      var diff = dist - this.segSpace;
      var changeDir = sub_v2(seg.pos, nextSegment.pos);
      var normalizedChangeDir = normalize_v2(changeDir);
      var changeVector = scale_v2(normalizedChangeDir, diff);

      const isSnake = this.type === "SNAKE";
      const headBias = isSnake && i === 0 ? 0.2 : 0.5;
      const tailBias = isSnake && i === 0 ? 0.8 : 0.5;

      if (!segIsAnchor && !nextIsAnchor) {
        if (seg !== selSegment) seg.pos = sub_v2(seg.pos, scale_v2(changeVector, headBias));
        nextSegment.pos = add_v2(nextSegment.pos, scale_v2(changeVector, tailBias));
      } else if (!segIsAnchor && nextIsAnchor) {
        if (seg !== selSegment) seg.pos = sub_v2(seg.pos, changeVector);
      } else if (segIsAnchor && !nextIsAnchor) {
        nextSegment.pos = add_v2(nextSegment.pos, changeVector);
      }
    }
  }
  render(hasCircle = true) {
    var isHighlight = (selSegment && selSegment.rope === this) || (prevHov && prevHov.rope === this);
    var lineWidth = this.thick;
    var lw = lineWidth / 2;
    var circles = [];

    var lineClr = isHighlight ? setAlpha(this.color, 0.8) : this.color;
    for (let i = 0; i < this.segAmount; i++) {
      var curClr = lineClr;
      if (this.segments[i].anchorObject) curClr = this.segments[i].anchorObject.color;
      else if (this.color2) curClr = addColor(lineClr, this.color2, i / this.segAmount);
      if (this.isSnake && i % 2 === 0) curClr = addColor(curClr, "black", 0.2);
      var seg = this.segments[i];
      var isColliding = false;
      var colWidth = Math.max(lineWidth, 8);
      if (!selAirPusher && !selDirPusher && !selShape && pointInRect(mouse.pos, new Vec2(seg.pos.x - colWidth, seg.pos.y - colWidth), new Vec2(colWidth * 2, colWidth * 2))) {
        if (hovSegment) isColliding = magnitude_v2(mouse.pos, seg.pos) < magnitude_v2(mouse.pos, hovSegment.pos);
        else isColliding = true;
      }
      if (isColliding) hovSegment = seg;
      if (i < this.segments.length - 1) {
        var nextSeg = this.segments[i + 1];
        var end = nextSeg.pos;
        drawLine(ctx, [seg.pos.x, seg.pos.y], [end.x, end.y], curClr, lineWidth, 0);
        if (hasCircle) drawCircle2(ctx, [seg.pos.x, seg.pos.y], lw, curClr, "rgba(0,0,0,0)", 0);
      } else if (hasCircle) {
        drawCircle2(ctx, [seg.pos.x, seg.pos.y], lw, curClr, "rgba(0,0,0,0)", 0);
      }
      if (hasCircle && seg.isAnchor && showAnchors) circles.push([seg.pos, 4, curClr, "black", 1]);
    }
    for (const c of circles) drawCircle2(ctx, [c[0].x, c[0].y], c[1], c[2], c[3], c[4]);
    if (showDots) for (const seg of this.segments) drawRect(seg.pos.x - 4, seg.pos.y - 4, 8, 8, "rgba(0,0,0,0)", "yellow");
  }

  static globalModifier(_newThick = null, _segAmount = null, _segSpace = null) {
    for (const r of ropes) {
      if (_newThick) r.thick = _newThick;
      if (_segAmount) r.setNewSegAmount(_segAmount);
      if (_segSpace) r.segSpace = _segSpace;
    }
    if (_newThick) segThickness = _newThick;
    if (_segAmount) segAmount = _segAmount;
    if (_segSpace) segSpace = _segSpace;
  }

  static remove(rope) {
    if (rope.rope !== undefined) rope = rope.rope;
    var idx = ropes.indexOf(rope);
    if (idx === -1) return;
    if (selSegment && selSegment.rope === rope) selSegment = null;
    ropes.splice(idx, 1);
  }

  static instantiate(start, end, isAnchored = true) {
    var rope = new Rope(new Vec2(start.x, start.y), end ? new Vec2(end.x, end.y) : null);
    ropes.push(rope);
    if (!isAnchored) rope.segments[0].setAnchor(null);
    return rope;
  }
}

class RopeEntity extends Rope {
  constructor(start, end = null, color = getRandomColor(), thick = 20, _segAmount = segAmount, _segSpace = segSpace, damp = dampingFactor) {
    super(start, end, color, thick, segAmount, segSpace, damp);
    this.vel = new Vec2(r_range(-5, 5), r_range(-5, 5));
    this.segments[0].isAnchor = false;
    this.maxVel = new Vec2(8, 5);
    this.steerFactor = new Vec2(0.1, 0.02);
    this.steerSpeed = mult_v2(this.maxVel, this.steerFactor);
    this.groundFriction = 0.94;
    this.jumpChance = 100;
    this.grounded = false;
  }

  jump(force = 10) {
    this.vel.y = -this.maxVel.y * force;
    setTimeout(() => (this.vel.y = 0), 200);
  }

  steerAgent(headPos = this.segments[0].pos) {
    if (headPos.x <= this.thick * 4) this.vel.x += this.steerSpeed.x;
    else if (headPos.x >= window.innerWidth - this.thick * 4) this.vel.x -= this.steerSpeed.x;
    else this.vel.x = clamp(this.vel.x + (Math.random() * 2 - 1) * this.steerSpeed.x, -this.maxVel.x, this.maxVel.x);

    if (headPos.y <= this.thick * 4) this.vel.y += this.steerSpeed.y;
    else if (headPos.y >= window.innerHeight - this.thick * 4) this.vel.y -= this.steerSpeed.y;
    else this.vel.y = clamp(this.vel.y + (Math.random() * 2 - 1) * this.steerSpeed.y, -this.maxVel.y, this.maxVel.y);
    if (this.grounded && r_range_int(0, this.jumpChance) == 0) this.jump(10);
    return add_v2(headPos, this.vel);
  }

  controlSnake(headPos = this.segments[0].pos) {
    if (input.keyClicked === " " && this.grounded) {
      this.jump();
      return headPos;
    }
    if (this.vel.y < -this.maxVel.y) this.vel.y += 1;
    var inputVec = input.wasd;
    var max = mult_v2(this.maxVel, new Vec2(2, 2));
    var steerSpeed = 1;
    if (!inputVec.x) this.vel.x *= 0.8;
    else this.vel.x = clamp(this.vel.x + inputVec.x * steerSpeed, -max.x, max.x);
    if (inputVec.y) this.vel.y = clamp(this.vel.y + inputVec.y * steerSpeed, -max.y, max.y);
    return add_v2(headPos, this.vel);
  }

  getHeadMovement() {
    if (this === player) return this.controlSnake();
    else return this.steerAgent();
  }

  handleEntityCollisions() {
    var a = this.segments[0];
    for (const s of this.segments) {
      var p = s.pos;
      var closeSegments = colGrid.getAtPos(p.x, p.y);
      for (const b of closeSegments) {
        if (b.type === "CIRCLE" && circleOverlap(a.pos, this.thick, b.pos, b.size.x)) {
          b.vel = add_v2(b.vel, scale_v2(this.vel, 2));
          var dx = b.pos.x - a.pos.x;
          var dy = b.pos.y - a.pos.y;
          var dist = Math.sqrt(dx * dx + dy * dy) || 0.0001;
          var pushDist = (this.thick + b.size.x) / 2 - dist;
          if (pushDist > 0) {
            b.pos.x += (dx / dist) * pushDist * 0.5;
            b.pos.y += (dy / dist) * pushDist * 0.5;
          }
        } else if (b.type === "SQUARE" && circleInRect(a.pos, this.thick, b.pos, b.size)) {
          b.vel = add_v2(b.vel, scale_v2(this.vel, 2));
          var closestX = Math.max(b.pos.x, Math.min(a.pos.x, b.pos.x + b.size.x));
          var closestY = Math.max(b.pos.y, Math.min(a.pos.y, b.pos.y + b.size.y));
          var dx = a.pos.x - closestX;
          var dy = a.pos.y - closestY;
          var dist = Math.sqrt(dx * dx + dy * dy) || 0.0001;
          var pushDist = this.thick / 2 - dist;
          if (pushDist > 0) {
            b.pos.x += (dx / dist) * pushDist * 0.5;
            b.pos.y += (dy / dist) * pushDist * 0.5;
          }
        } else if (Math.abs(this.segments.indexOf(a) - this.segments.indexOf(b)) > 2) this.handleSegCollision(a, b, 0.2);
      }
    }
  }

  update() {
    var newP = this.getHeadMovement();
    this.handleEntityCollisions();
    this.segments[0].pos = newP;
    super.update();
  }

  control() {
    player = this;
  }

  renderEntity() {
    var lastSegP = this.segments[0].pos;
    var w = this.thick / 4;
    var cVel = clamp_v2(this.vel, new Vec2(3, 3));
    var eye1P = [lastSegP.x - w / 2 + cVel.x, lastSegP.y - w / 2 + cVel.y];
    var eye2P = [lastSegP.x - w / 2 + cVel.x * 0.7, lastSegP.y - w / 2 + cVel.y * 0.7];

    drawCircle2(ctx, eye1P, w, "white", "rgba(0, 0, 0, 1)", 2);
    drawCircle2(ctx, eye2P, w, "rgba(206, 202, 202, 1)", "rgba(0, 0, 0, 1)", 2);
    drawCircle2(ctx, [eye2P[0] + cVel.x * 0.25 + w / 16, eye2P[1] + cVel.y * 0.25 + w / 16], w / 4, "black", null, 0);
  }

  render() {
    super.render();
    this.renderEntity();
    if (this === player) drawText(ctx, [this.segments[0].pos.x, this.segments[0].pos.y - 50], "YOU", "white", null, 12);
  }

  static instantiate(pos, thick = r_range(5, 20)) {
    var entity = new this(pos, null, getRandomColor(), thick, segAmount, r_range(1, 5));
    ropes.push(entity);
    return entity;
  }
}

class Snake extends RopeEntity {
  constructor(start, end = null, color = getRandomColor(), thick = 20, _segAmount = segAmount, _segSpace = segSpace, damp = dampingFactor) {
    super(start, end, color, thick, segAmount, segSpace, damp);
    this.type = "SNAKE";
  }

  static instantiate(pos, thick = r_range(5, 20)) {
    var snake = new Snake(pos, null, getRandomColor(), thick, segAmount, r_range(1, 5));
    ropes.push(snake);
    return snake;
  }
}

class Spider {
  constructor(pos, bodySize = 15, legCount = 8, legLength = 40, legThickness = 3) {
    this.pos = new Vec2(pos.x, pos.y);
    this.vel = new Vec2(r_range(-5, 5), r_range(-5, 5));
    this.bodySize = bodySize;
    this.legCount = legCount;
    this.legLength = legLength;
    this.legThickness = legThickness;
    this.color = getRandomColor();
    this.maxVel = new Vec2(8, 5);
    this.type = "SPIDER";
    this.steerFactor = new Vec2(0.1, 0.02);
    this.steerSpeed = mult_v2(this.maxVel, this.steerFactor);
    this.grounded = false;
    this.jumpChance = 100;
    this.gravity = new Vec2(gravity.x, gravity.y);

    this.body = new Shape(pos, new Vec2(bodySize, bodySize), "CIRCLE", this.color);
    shapes.push(this.body);
    this.legs = [];
    this.initLegs(this.body);
  }

  initLegs() {
    const angleStep = (Math.PI * 2) / this.legCount;
    for (let i = 0; i < this.legCount; i++) {
      const angle = angleStep * i;
      const legStart = new Vec2(this.pos.x + Math.cos(angle) * this.bodySize, this.pos.y + Math.sin(angle) * this.bodySize);
      const legEnd = new Vec2(this.pos.x + Math.cos(angle) * (this.bodySize + this.legLength), this.pos.y + Math.sin(angle) * (this.bodySize + this.legLength));
      const legOffset = new Vec2(Math.cos(angle) * this.bodySize, Math.sin(angle) * this.bodySize);
      const leg = new Rope(legStart, legEnd, this.color, this.legThickness, 15, 3);
      leg.spiderParent = this;
      leg.legAngle = angle;
      leg.segments[leg.segments.length - 1].setAnchor(null);
      leg.segments[0].attachToShape(this.body, legOffset);
      this.legs.push(leg);
      ropes.push(leg);
    }
  }

  getBodyMovement() {
    if (this === player) return this.controlSpider();
    else return this.steerAgent();
  }

  update() {
    this.grounded = false;
    var newPos = this.getBodyMovement();

    this.body.update();
    return;
    // Handle gravity and collisions
    this.vel.y += this.gravity.y * dt * 10;
    const floorY = window.innerHeight - this.bodySize / 2;
    if (newPos.y >= floorY) {
      this.vel.y *= -0.4;
      newPos.y = floorY;
      this.grounded = true;
    }

    newPos.x = clamp(newPos.x, this.bodySize, window.innerWidth - this.bodySize);
    this.pos = newPos;

    // Update all legs to follow body
    const angleStep = (Math.PI * 2) / this.legCount;
    for (let i = 0; i < this.legs.length; i++) {
      const leg = this.legs[i];
      const angle = angleStep * i;

      // Anchor first segment to body
      leg.segments[0].pos = new Vec2(this.pos.x + Math.cos(angle) * this.bodySize, this.pos.y + Math.sin(angle) * this.bodySize);
      leg.segments[0].prevPos = leg.segments[0].pos;
      leg.segments[0].setAnchor();
    }
  }

  control() {
    player = this;
  }

  render() {
    var w = this.bodySize / 3;
    var cVel = clamp_v2(this.vel, new Vec2(2, 2));
    var eye1P = [this.pos.x - w / 2 + cVel.x, this.pos.y - w / 2 + cVel.y];
    var eye2P = [this.pos.x + w / 2 + cVel.x, this.pos.y - w / 2 + cVel.y];

    drawCircle2(ctx, eye1P, w / 2, "white", "rgba(0, 0, 0, 1)", 1);
    drawCircle2(ctx, eye2P, w / 2, "white", "rgba(0, 0, 0, 1)", 1);

    if (this === player) drawText(ctx, [this.pos.x, this.pos.y - this.bodySize - 30], "YOU", "white", null, 12);
  }

  static instantiate(pos) {
    var spider = new Spider(pos, 15, 8, 40, 3);
    return spider;
  }

  static remove(spider) {
    for (const leg of spider.legs) {
      Rope.remove(leg);
    }
    if (player === spider) player = null;
  }
}
