function initImage(w, h, x, y, src) {
  const imgDiv = document.createElement("img");
  imgDiv.src = src;
  imgDiv.width = w;
  imgDiv.height = h;
  imgDiv.style.left = x + "px";
  imgDiv.style.top = y + "px";
  imgDiv.style.position = "absolute";
  document.body.appendChild(imgDiv);
  return imgDiv;
}

function initLabelDiv(x, y, text = "", bgrColor = null, color = "white", parent = document.body) {
  let div = document.createElement("label");
  div.className = "infoText";
  div.style.position = "fixed";
  if (bgrColor) {
    div.style.paddingTop = "3px";
    div.style.paddingLeft = "3px";
    div.style.backgroundColor = bgrColor;
  }
  div.style.top = y + "px";
  div.style.left = x + "px";
  div.style.whiteSpace = "pre";
  div.textContent = text;
  div.style.color = color;
  if (parent) parent.appendChild(div);
  return div;
}

function writeBox(w, h, x, y, bgrClr) {
  const div = document.createElement("div");
  div.style.width = w + "px";
  div.style.height = h + "px";
  div.style.top = y + "px";
  div.style.left = x + "px";
  div.style.position = "absolute";
  div.style.backgroundColor = bgrClr;
  document.body.appendChild(div);
  return div;
}

// writeBox(window.innerWidth, 20, 0, window.innerHeight - 100, "rgba(0, 0, 0, 0.26)");
// writeBox(window.innerWidth, 100, 0, window.innerHeight - 100, "rgba(0, 0, 0, 0.13)");
let infoBoxes = [];
function announce(msg, dur = 2000, bgr = null) {
  const base = [window.innerWidth / 2, window.innerHeight - 55];
  const y = base[1] + infoBoxes.length * 20;
  const box = addDiv(msg, [base[0], y], 1, "white", bgr);
  infoBoxes.push(box);
  setTimeout(() => {
    box.remove();
    const i = infoBoxes.indexOf(box);
    if (i !== -1) infoBoxes.splice(i, 1);
    for (let j = 0; j < infoBoxes.length; j++) {
      const div = infoBoxes[j];
      div.style.top = base[1] + j * 20 + "px";
    }
  }, dur);
  return box;
}
