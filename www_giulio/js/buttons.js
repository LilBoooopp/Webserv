function addDiv(text, pos, size, clr = null, bgrClr = null) {
  let div = document.createElement("div");
  div.textContent = text;

  div.style.position = "absolute";
  div.style.left = pos[0] + "px";
  div.style.top = pos[1] + "px";
  div.style.transform = "translate(-50%, -50%) scale(" + size + ")";
  div.style.transformOrigin = "center center";

  div.style.userSelect = "none";
  if (clr) div.style.color = clr;
  if (bgrClr) div.style.backgroundColor = bgrClr;

  document.body.appendChild(div);
  return div;
}

function addButton(label, pos = [0, 0], onEnd = null, color = null, bgrClr = null) {
  let div = document.createElement("div");
  div.textContent = label;

  div.style.display = "inline-block";
  div.style.padding = "8px 12px";
  div.style.position = "absolute";
  div.style.userSelect = "none";
  div.style.backgroundColor = "black";
  div.style.color = "white";
  div.style.cursor = "pointer";
  div.style.borderRadius = "5px";
  if (color) div.style.color = color;
  if (bgrClr) div.style.backgroundColor = bgrClr;

  document.body.appendChild(div);

  const rect = div.getBoundingClientRect();

  const p = [pos[0] - rect.width / 2, pos[1] - rect.height / 2];

  div.style.transition = "scale 0.2s ease-out";

  div.addEventListener("mouseenter", () => (div.style.scale = 1.1));
  div.addEventListener("mouseleave", () => (div.style.scale = 1));

  if (onEnd) {
    div.addEventListener("mousedown", onEnd);
  }
  div.style.left = p[0] + "px";
  div.style.top = p[1] + "px";
  return div;
}

function moveToUrl(_response, text) {
  console.warn("moving to " + text);
  window.location.href = text;
}
function announceResponse(response, _text) {
  announce("Status " + response.status + " " + _text);
}

function cgiButton(label, scriptPath, pos = [0, 0], arg = null, onEnd = moveToUrl) {
  const but = addButton(label, pos);
  but.addEventListener("mousedown", () => {
    let url = scriptPath;
    if (arg != null) url += "?arg=" + encodeURIComponent(arg);
    fetch(url)
      .then((r) => r.text().then((t) => ({ r, t })))
      .then(({ r, t }) => {
        const responseText = t.trim();
        if (onEnd) onEnd(r, responseText);
      });
  });
  return but;
}

function loopRequest() {
  const counter = 9999;
  let completed = 0;
  const div = addDiv("SENT 0 REQUESTS", [window.innerWidth / 2, window.innerHeight / 2 - 100], 1, "red", "black");
  for (let i = 0; i < counter; i++) {
    fetch("/").then(() => {
      completed++;
      div.textContent = "SENT " + completed + " REQUESTS";
      if (completed >= 9998) div.remove();
    });
  }
}

function longRequest() {
  length = 99999999;
  var string = "TEST ";
  for (let i = 0; i < length; i++) string += "0";
  fetch(string);
  announce("Done!");
}

function addHomeButton() {
  const c = [window.innerWidth / 2, window.innerHeight / 2];
  cgiButton("HOME", "/cgi/printArg.py", [c[0], window.innerHeight - 40]);
}
