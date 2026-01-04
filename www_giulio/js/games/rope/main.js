var screenCenter = new Vec2(window.innerWidth / 2, window.innerHeight / 2);

let player = null;

//	COLLISIONS
let collisionSegmentInteval = 2;
let colCellSize = window.innerWidth / 8;
let SelfCollisionsInterval = 4;
let colGrid = null;
let overlapFactor = 0.25;
let ropeGroundFriction = 0.65;

//	ELEMENTS
let ropes = [];
let shapes = [];
let airPushers = [];

let winSize = new Vec2(window.innerWidth, window.innerHeight);
//	SELECTION
let useMouse = true;
let selShape = null;
let hovShape = null;
let hovDirPusher = null;
let selDirPusher = null;
let selAirPusher = null;
let hovAirPusher = null;
let selSegment = null;
let prevHov = null;
let hovSegment = null;

//	ROPES PARAMS
let gravity = new Vec2(0, 100);
let segAmount = 50;
let segSpace = 10;
let segThickness = 30;
let dampingFactor = 0.95;
let numOfConstraintsRuns = 50;
let showDots = false;
let ropeShaker = 0;
let showAnchors = true;

// SHAPES
let ShapeScale = 30;

// TIME
let frame = 0;
let dt = 1;
let now = performance.now();
let curFps = 0;
let fps = 0;
let lastFpsTimer = performance.now();

function render() {
  ctx.fillStyle = "black";
  ctx.fillRect(0, 0, _canvas.width, _canvas.height);
  drawText(ctx, [window.innerWidth / 2, window.innerHeight * 0.4], "Ropes", "white", null, 50);
  for (const a of airPushers) a.render();
  for (const r of ropes) r.render();
  for (const s of shapes) s.render();

  var seg = hovSegment ? hovSegment : selSegment;
  if (seg) {
    var lineWidth = Math.max(8, seg.rope.thick * 2);
    drawRect(seg.pos.x - lineWidth / 2, seg.pos.y - lineWidth / 2, lineWidth, lineWidth, "rgba(0,0,0,0)", "yellow");
  }
  contextMenu.render();
  if (colGrid.shown) colGrid.show();
  document.body.style.cursor = selSegment || selShape || selAirPusher || selDirPusher ? "grab" : hovAirPusher || hovDirPusher || hovSegment || hovShape ? "pointer" : "default";
  if (contextMenu.selSlider) document.body.style.cursor = "grabbing";
  else if (document.body.style.cursor === "default" && contextMenu.active && contextMenu.hovPath.length > 0) document.body.style.cursor = "pointer";
  drawText(ctx, [window.innerWidth - 30, window.innerHeight - 30], "fps " + fps, "white", null, 12, true);
}

function updateSegSelection() {
  if (hovSegment && !selSegment && mouse.clicked) {
    selSegment = hovSegment;
    if (input.keys["alt"]) selSegment.rope.duplicate();
    else if (input.keys["shift"]) {
      if (selSegment.isAnchor) selSegment.setAnchor(null);
      else selSegment.setAnchor();
    }
    selSegment.prevAnchor = selSegment.isAnchor;
    selSegment.isAnchor = true;
  } else if (selSegment && !mouse.pressed) {
    selSegment.isAnchor = selSegment.prevAnchor;
    selSegment = null;
  } else if (selSegment) selSegment.place(new Vec2(mouse.pos.x, mouse.pos.y));

  if (!selShape && hovShape && mouse.clicked) {
    selShape = hovShape;
    selShape.vel = new Vec2(0, 0);
    if (input.keys["alt"]) selShape.duplicate();
  }

  if (input.keys["z"]) shakeAll();
  if (input.keys["r"]) clearAll();
  if (input.keyClicked === "x" || input.keyClicked === "backspace") {
    if (hovSegment && hovSegment.isAnchor) hovSegment.setAnchor(null);
    else if (hovSegment) ropes.splice(ropes.indexOf(hovSegment.rope), 1);
    if (hovAirPusher) airPushers.splice(airPushers.indexOf(hovAirPusher), 1);
    if (hovShape) shapes.splice(shapes.indexOf(hovShape), 1);
  }
}

function update() {
  prevHov = hovSegment;
  hovSegment = null;
  hovAirPusher = null;
  hovShape = null;
  hovDirPusher = null;

  if (ropeShaker && frame % 2 === 0) shakeAll(ropeShaker);
  for (const s of shapes) s.update();
  for (const r of ropes) r.update();
  if (colGrid.active) colGrid.update();
  render();
  if (useMouse) updateSegSelection();
  mouse.reset();
  input.reset();
}

function loop() {
  var newNow = performance.now();
  dt = (newNow - now) / 1000;
  now = newNow;
  if (newNow - lastFpsTimer > 1000) {
    fps = curFps;
    curFps = 0;
    lastFpsTimer = newNow;
  }
  frame++;
  curFps++;
  update();
  requestAnimationFrame(loop);
}

function initRopeSimulation() {
  initCanvas();
  colGrid = new CollisionGrid();
  loop();
}
