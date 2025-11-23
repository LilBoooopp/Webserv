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

function announce(msg, dur = 2000, bgr = "rgba(0, 0, 0, 1)") {
  const infoBox = addDiv(msg, [window.innerWidth / 2, window.innerHeight - 100], 1, "white", bgr);
  setTimeout(() => infoBox.remove(), dur);
}
