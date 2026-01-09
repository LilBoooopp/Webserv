class RopeEntity extends Rope {
  constructor(start, end = null, color = getRandomColor(), thick = 20, _segAmount = segAmount, _segSpace = segSpace, damp = dampingFactor) {
    super(start, end, color, thick, _segAmount, _segSpace, damp);
    this.vel = new Vec2(r_range(-5, 5), r_range(-5, 5));
    this.segments[0].isAnchor = false;
    this.maxVel = new Vec2(8, 5);
    this.steerFactor = new Vec2(0.1, 0.02);
    this.steerSpeed = mult_v2(this.maxVel, this.steerFactor);
    this.steerChance = 100;
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
    else if (r_range_int(0, 100) <= this.steerChance) this.vel.x = clamp(this.vel.x + (Math.random() * 2 - 1) * this.steerSpeed.x, -this.maxVel.x, this.maxVel.x);

    if (headPos.y <= this.thick * 4) this.vel.y += this.steerSpeed.y;
    else if (headPos.y >= window.innerHeight - this.thick * 4) this.vel.y -= this.steerSpeed.y;
    else if (r_range_int(0, 100) <= this.steerChance) this.vel.y = clamp(this.vel.y + (Math.random() * 2 - 1) * this.steerSpeed.y, -this.maxVel.y, this.maxVel.y);
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
    super.update();
  }

  static duplicate() {
    var rp = RopeEntity.instantiate(this.constructor, new Vec2(this.pos.x, this.pos.y));
    return rp;
  }

  control() {
    player = this;
  }

  render() {
    super.render();
    if (this === player) drawText(ctx, [this.segments[0].pos.x, this.segments[0].pos.y - 50], "YOU", "white", null, 12);
  }

  static instantiate(constructor, pos) {
    try {
      const entity = new constructor(pos);
      entities.push(entity);
      return entity;
    } catch (e) {
      console.warn("Error instantiating entity: " + e.message);
      return null;
    }
  }
}

class Snake extends RopeEntity {
  constructor(start, end = null, color = getRandomColor(), thick = 20, _segAmount = segAmount, _segSpace = segSpace, damp = dampingFactor) {
    super(start, end, color, thick, segAmount, segSpace, damp);
    this.type = "SNAKE";
    this.isRigid = true;
  }

  update() {
    var newP = this.getHeadMovement();
    this.handleEntityCollisions();
    this.segments[0].pos = newP;
    super.update();
  }

  render() {
    super.render();

    var lastSegP = this.segments[0].pos;
    var w = this.thick / 4;
    var cVel = clamp_v2(this.vel, new Vec2(3, 3));
    var eye1P = [lastSegP.x - w / 2 + cVel.x, lastSegP.y - w / 2 + cVel.y];
    var eye2P = [lastSegP.x - w / 2 + cVel.x * 0.7, lastSegP.y - w / 2 + cVel.y * 0.7];

    drawCircle2(ctx, eye1P, w, "white", "rgba(0, 0, 0, 1)", 2);
    drawCircle2(ctx, eye2P, w, "rgba(206, 202, 202, 1)", "rgba(0, 0, 0, 1)", 2);
    drawCircle2(ctx, [eye2P[0] + cVel.x * 0.25 + w / 16, eye2P[1] + cVel.y * 0.25 + w / 16], w / 4, "black", null, 0);
  }

  static instantiate(pos, thick = r_range(5, 20)) {
    var snake = new Snake(pos, null, getRandomColor(), thick, segAmount, r_range(1, 5));
    entities.push(snake);
    return snake;
  }
}

class Spider {
  constructor(pos, bodySize = 15, legCount = 8, legAmount = 5, legSpace = 40, legThickness = 3) {
    this.pos = new Vec2(pos.x, pos.y);
    this.vel = new Vec2(r_range(-5, 5), r_range(-5, 5));
    this.bodySize = bodySize;
    this.legCount = legCount;
    this.legAmount = legAmount;
    this.legSpace = legSpace;
    this.legThickness = legThickness;
    this.color = getRandomColor();
    this.maxVel = new Vec2(8, 5);
    this.type = "SPIDER";
    this.steerFactor = new Vec2(0.1, 0.02);
    this.steerSpeed = mult_v2(this.maxVel, this.steerFactor);
    this.grounded = false;
    this.jumpChance = 100;
    this.gravity = new Vec2(gravity.x, gravity.y);
    this.init();
  }

  init() {
    this.body = new Shape(this.pos, new Vec2(this.bodySize, this.bodySize), "CIRCLE", this.color);
    this.body.bounceFactor = 0;
    this.body.dragFactor = 0;
    this.body.angVel = 0;
    shapes.push(this.body);
    this.legs = [];
    let distFromCenter = this.bodySize * 1.2;
    const angleStep = (Math.PI * 2) / this.legCount;
    for (let i = 0; i < this.legCount; i++) {
      const angle = angleStep * i;
      const legStart = new Vec2(this.pos.x + Math.cos(angle) * this.bodySize, this.pos.y + Math.sin(angle) * this.bodySize);
      const legOffset = new Vec2(Math.cos(angle) * distFromCenter, Math.sin(angle) * distFromCenter);
      // Second segment goes upward in web-like manner
      const seg1Offset = new Vec2(Math.cos(angle) * distFromCenter * 1, -distFromCenter * 2.5);
      // Third segment continues horizontally outward
      const seg2Offset = new Vec2(Math.cos(angle) * distFromCenter * 3, Math.sin(angle) * distFromCenter * 0.5);
      const seg3Offset = new Vec2(seg2Offset.x, seg2Offset.y + 20);
      const seg4Offset = new Vec2(seg2Offset.x, seg2Offset.y + 30);

      const leg = new Rope(legStart, null, this.color, this.legThickness, this.legAmount, this.legSpace);
      leg.parent = this;
      leg.legAngle = angle;
      leg.segments[0].attachToShape(this.body, legOffset);
      leg.segments[1].attachToShape(this.body, seg1Offset);
      leg.segments[2].attachToShape(this.body, seg2Offset);
      leg.segments[3].attachToShape(this.body, seg3Offset);
      leg.segments[4].attachToShape(this.body, seg4Offset);

      this.legs.push(leg);
      ropes.push(leg);
    }
  }

  update() {
    this.legs[0].segments[1].pos.y = this.body.pos.y - 50;
    return;
    this.vel.y += this.gravity.y * 0.016; // deltaTime approximation
    this.pos.x += this.vel.x;
    this.pos.y += this.vel.y;
    this.body.pos = this.pos;
    for (const leg of this.legs) {
      const lastSegment = leg.segments[leg.segments.length - 1];
      // Try to place leg endpoint on ground or slightly above
      const targetY = Math.min(lastSegment.pos.y, window.innerHeight - 10);

      // Prevent legs from going too far from body
      const maxLegReach = this.bodySize + this.legAmount * this.legSpace;
      const dx = lastSegment.pos.x - this.pos.x;
      const dy = lastSegment.pos.y - this.pos.y;
      const dist = Math.sqrt(dx * dx + dy * dy);

      if (dist > maxLegReach) {
        const ratio = maxLegReach / dist;
        lastSegment.pos.x = this.pos.x + dx * ratio;
        lastSegment.pos.y = this.pos.y + dy * ratio;
      }

      // Simple ground collision for leg endpoints
      if (lastSegment.pos.y >= window.innerHeight - 5) {
        lastSegment.pos.y = window.innerHeight - 5;
      }
    }

    // Support body: push up if touching ground
    let groundContactCount = 0;
    for (const leg of this.legs) {
      const lastSegment = leg.segments[leg.segments.length - 1];
      if (lastSegment.pos.y >= window.innerHeight - 10) {
        groundContactCount++;
      }
    }

    // If legs are on ground, reduce downward velocity
    if (groundContactCount > 0) {
      this.vel.y *= 0.8;
      if (this.vel.y > 0) this.vel.y = 0;
    }
  }

  render() {
    return;
  }

  static instantiate(pos) {
    var spider = new Spider(pos);
    entities.push(spider);
    return spider;
  }

  static remove(spider) {
    for (const leg of spider.legs) {
      Rope.remove(leg);
    }
    if (player === spider) player = null;
  }
}

class Lugworm extends RopeEntity {
  constructor(pos, _segAmount = 80, _segSpace = 5) {
    super(pos, pos, getRandomColor(), 2, _segAmount, _segSpace);
    this.pos = this.segments[0].pos;
    this.vel = new Vec2(r_range(-5, 5), r_range(-5, 5));
    this.type = "LUGWORM";
    this.maxVel = new Vec2(8, 8);
    this.steerFactor = new Vec2(5, 4);
    this.steerSpeed = mult_v2(this.maxVel, this.steerFactor);
    this.steerChance = 2;
    this.gravity = new Vec2(0, -20);
  }

  update() {
    this.pos = this.segments[0].pos;
    var newP = this.steerAgent();
    this.segments[0].pos = newP;
    super.update();
  }
  static instantiate(pos) {
    var lugworm = new Lugworm(pos);
    entities.push(lugworm);
    return lugworm;
  }
}

class RobotArm extends RopeEntity {
  constructor(pos, _segAmount = 10, _segSpace = 40, color = getRandomColor(), targetObject = mouse) {
    super(pos, null, color, 2, _segAmount, _segSpace, dampingFactor);
    this.segments[0].setAnchor(pos);
    this.type = "ROBOTARM";
    this.isRigid = true;
    this.gravity.y = -100;
    this.targetObject = targetObject;
  }

  update() {
    super.update();
    if (this.targetObject) {
      const seg = this.segments[this.segments.length - 1];
      const p = seg.pos;
      const t = this.targetObject.pos;
      const dx = t.x - p.x;
      const dy = t.y - p.y;
      const dist = Math.sqrt(dx * dx + dy * dy) || 0.0001;
      const step = Math.min(10000, dist);
      const angleToTarget = Math.atan2(dy, dx);
      seg.pos = new Vec2(p.x + Math.cos(angleToTarget) * step, p.y + Math.sin(angleToTarget) * step);
    }
  }

  render() {
    super.render();
  }

  static instantiate(pos) {
    var robotArm = new RobotArm(pos);
    entities.push(robotArm);
    return robotArm;
  }
}
