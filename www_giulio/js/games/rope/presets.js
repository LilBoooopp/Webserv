function snakesInSpace(am = 10) {
  colGrid.init(100, false);
  clearAll();
  var center = new Vec2(window.innerWidth / 2, window.innerHeight / 2);
  gravity = new Vec2(0, 0);
  for (let i = 0; i < am; i++) {
    var r = Snake.instantiate(mouse.pos);
    if (i === am - 1) r.control();
  }
  for (let i = 0; i < am / 2; i++) {
    var size = r_range(10, 40);
    Shape.instantiate("CIRCLE", rand_v2(), new Vec2(size, size));
  }
}

function upsideDownWorld(am = 100) {
  colGrid.init(20, false);
  clearAll();
  gravity = new Vec2(0, -100);
  for (let i = 0; i < am; i++) {
    var p = new Vec2((window.innerWidth / am) * i, window.innerHeight);
    var r = new Rope(p, p, "rgba(101, 235, 228, 1)", 2, 40, 10);
    r.color2 = "rgba(0, 255, 51, 1)";
    r.segments[r.segments.length - 1].setAnchor(null);
    ropes.push(r);
  }
  Shape.removeAll();
  setTimeout(() => {
    var s = new Shape(new Vec2(0, window.innerHeight * 0.4), new Vec2(40, 40), "CIRCLE", getRandomColor(), 0, new Vec2(0, 100));
    s.vel.x = 1000;
    shapes.push(s);
  }, 1000);
}

function weirdWorld(am = 400) {
  colGrid.init(200, false);
  numOfConstraintsRuns = 1;
  SelfCollisionsInterval = 0;
  showAnchors = false;
  gravity = new Vec2(0, 0);
  var limit = 100;
  for (let i = 0; i < am; i++) {
    var p = new Vec2(r_range(limit, window.innerWidth - limit), r_range(limit, window.innerHeight - limit));
    var r = new Rope(p, p, getRandomColor(), 1, 60, 3);
    r.segments[r.segments.length - 1].setAnchor(null);
    ropes.push(r);
  }
  for (let i = 0; i < 4; i++) {
    var s = Shape.instantiate("CIRCLE", rand_v2());
    s.vel = rand_v2(new Vec2(-1000, 1000), new Vec2(-1000, 1000));
    s.gravity.x = s.gravity.y = 0;
    s.bounceFactor = 1;
    s.dragFactor = 1;
  }
  //   Shape.setGlobalGravity(new Vec2(0, 100));
  var p = new Vec2(window.innerWidth / 2, window.innerHeight / 2);
  var airPusher = AirPusher.instantiate(p, 0);
  player = airPusher;
}

function hairWorld(am = 500) {
  gravity = new Vec2(-5, 0);
  var limit = 100;
  for (let i = 0; i < am; i++) {
    var p = new Vec2(r_range(limit, window.innerWidth - limit), r_range(limit, window.innerHeight - limit));
    var r = new Rope(p, p, getRandomColor(), 1, 60, 3);
    r.segments[r.segments.length - 1].setAnchor(null);
    ropes.push(r);
  }
  Rope.globalModifier(1, 30, 20);
  shakeAll(5);
  SelfCollisionsInterval = 0;
  //   Shape.setGlobalGravity(new Vec2(0, 100));
  var p = new Vec2(window.innerWidth / 2, window.innerHeight * 0.9);
  var airPusher = AirPusher.instantiate(p, -Math.PI / 2);
  airPusher.baseForce = 40;
  numOfConstraintsRuns = 10;
  showAnchors = false;
  Shape.instantiate();
  player = airPusher;
}

function animatedWorld() {
  gravity = new Vec2(0, 100);
  var am = 20;
  var dur = 100;
  for (let i = 0; i < am; i++) {
    setTimeout(() => {
      if (input.lastKey === "r") return;
      var segSpace = Math.max((am - i) / 3, 0.1);
      segSpace = r_range(2, 4);
      var segAmount = 50;
      ropes.push(new Rope(new Vec2(40 + (window.innerWidth / am) * i, 20), null, getRandomColor(), r_range_int(10, 30), segAmount, segSpace));
    }, i * dur);
  }
  setTimeout(() => {
    if (input.lastKey === "r") return;
    var circle = new Shape(new Vec2(window.innerWidth - 100, window.innerHeight * 0.9), new Vec2(40, 40), "CIRCLE");
    circle.vel.x = -1500;
    circle.vel.y = -1500;
    shapes.push(circle);
  }, am * 1.5 * dur);
}

function borderGrass() {
  colGrid.init(1, false);
  gravity = new Vec2(0, 0);
  SelfCollisionsInterval = 0;
  showAnchors = false;
  let am = 10;
  let gravAm = 5;
  ropeShaker = 0.05;
  var pad = 5;
  let dirs = ["left", "up", "right", "down"];
  for (const d of dirs) {
    for (let i = 0; i < am; i++) {
      var p;
      var grav = new Vec2(gravity.x, gravity.y);
      grav.y = d === "down" ? -gravAm : d === "up" ? gravAm : 0;
      grav.x = d === "right" ? -gravAm : d === "left" ? gravAm : 0;

      if (d === "down") p = new Vec2((window.innerWidth / am) * i, window.innerHeight - pad);
      else if (d === "left") p = new Vec2(pad, (window.innerHeight / am) * i);
      else if (d === "right") p = new Vec2(window.innerWidth - pad, (window.innerHeight / am) * i);
      else if (d === "up") p = new Vec2((window.innerWidth / am) * i, pad);
      var r = new Rope(p, p, getRandomColor(), 2, d === "down" || d === "up" ? 50 : 70, 10);
      r.gravity = grav;
      r.segments[r.segments.length - 1].setAnchor(null);
      ropes.push(r);
      var s = Shape.instantiate("CIRCLE", p, new Vec2(5, 5));
      s.gravity = grav;
      r.segments[r.segments.length - 1].attachToShape(s);
    }
  }
  var p = new Vec2(window.innerWidth / 2, window.innerHeight / 2);
  var airPusher = AirPusher.instantiate(p, 0);
  player = airPusher;
  Shape.removeAll();
}

function snakeBasketball() {
  colGrid.init(200, false);
  gravity = new Vec2(0, 100);
  var thick = 40;
  var limit = 200;
  var snake = Snake.instantiate(new Vec2(limit, window.innerHeight - limit), thick);
  snake.control();
  Snake.instantiate(new Vec2(window.innerWidth - limit, window.innerHeight - limit), thick);
  var ball = Shape.instantiate("CIRCLE", screenCenter, new Vec2(40, 40));
  ball.color = "rgba(249, 120, 0, 1)";
}

function setPreset(preset) {
  contextMenu.hide();
  clearAll();
  preset();
}
