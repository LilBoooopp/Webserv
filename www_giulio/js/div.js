function addDiv(text, pos, size, clr = null, bgrClr = null, centered = true) {
  let div = document.createElement("div");
  div.textContent = text;

  div.style.position = "absolute";

  div.style.left = (pos[0] / window.innerWidth) * 100 + "%";
  div.style.top = (pos[1] / window.innerHeight) * 100 + "%";
  div.style.whiteSpace = "pre";

  if (centered) {
    div.style.transform = "translate(-50%, -50%) scale(" + size + ")";
  } else {
    div.style.transform = "scale(" + size + ")";
  }

  div.style.transformOrigin = "center center";

  div.style.userSelect = "none";
  if (clr) div.style.color = clr;
  else if (window.DARKMODE !== "undefined") div.style.color = window.DARKMODE === 1 ? "white" : "black";
  if (bgrClr) div.style.backgroundColor = bgrClr;

  document.body.appendChild(div);
  return div;
}

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

let infoBoxes = [];
function announce(msg, dur = 2000, bgr = null) {
  const base = [window.innerWidth / 2, window.innerHeight * 0.8];
  const y = base[1] + infoBoxes.length * 20;
  const box = addDiv(msg, [base[0], y], 1, "white", bgr);
  box.classList.add("infoBox");

  infoBoxes.push(box);

  requestAnimationFrame(() => {
    box.classList.add("show");
  });

  setTimeout(() => {
    box.classList.remove("show");
    box.classList.add("hide");

    setTimeout(() => {
      box.remove();
      const i = infoBoxes.indexOf(box);
      if (i !== -1) infoBoxes.splice(i, 1);

      for (let j = 0; j < infoBoxes.length; j++) {
        const div = infoBoxes[j];
        div.style.top = base[1] + j * 20 + "px";
      }
    }, 250);
  }, dur);

  return box;
}

function addTitle(posOffset = [0, -150]) {
  const title = addDiv(window.PAGE_NAME.toUpperCase(), [window.innerWidth / 2 + posOffset[0], window.innerHeight / 2 + posOffset[1]], 3);
  title.style.textDecoration = "underline";
  title.style.textDecorationColor = title.style.color;
  title.style.textDecorationThickness = "1px";
  if (window.CURRENT_USER !== undefined) {
    const u1_title = addDiv("User: ", [50, 50], 1, "grey");
    const u_title = addDiv(window.CURRENT_USER, [100, 50], 1);
  }
  return title;
}

function applyBackground(flag = window.DARKMODE !== undefined && window.DARKMODE === 1) {
  document.documentElement.style.transition = "background-color 0.2s ease";
  document.documentElement.style.setProperty("background-color", flag ? "rgba(34, 34, 34, 1)" : "", "important");

  const elements = document.querySelectorAll("div, button, #createForm input, #createForm");
  elements.forEach((el) => {
    if (!el.style.transition.includes("color")) {
      el.style.transition = (el.style.transition ? el.style.transition + ", " : "") + "color 0.5s ease, background-color 0.5s ease, text-decoration-color 0.5s ease";
    }
    const currentColor = window.getComputedStyle(el).color;
    const currentBgColor = window.getComputedStyle(el).backgroundColor;
    const isBlack = currentColor === "rgb(0, 0, 0)";
    const isWhite = currentColor === "rgb(255, 255, 255)";
    const isBgBlack = currentBgColor === "rgb(0, 0, 0)";
    const isBgWhite = currentBgColor === "rgb(255, 255, 255)";

    if (flag && isBlack) {
      el.style.color = "white";
      el.style.textDecorationColor = "white";
      if (el.tagName === "INPUT" || el.tagName === "BUTTON") {
        el.style.backgroundColor = "rgba(0, 0, 0, 0.8)";
      }
    } else if (!flag && isWhite) {
      el.style.color = "black";
      el.style.textDecorationColor = "black";
      if (el.tagName === "INPUT" || el.tagName === "BUTTON") {
        el.style.backgroundColor = "rgba(255, 255, 255, 0.9)";
      }
    }

    // Toggle background colors for game elements
    if (flag && isBgBlack) {
      el.style.backgroundColor = "white";
    } else if (!flag && isBgWhite) {
      el.style.backgroundColor = "black";
    }
  });
}

function initBackground(flag = window.DARKMODE === undefined || window.DARKMODE) {
  window.DARKMODE = flag;
  document.documentElement.style.setProperty("background-color", flag ? "rgba(34, 34, 34, 1)" : "", "important");
  const elements = document.querySelectorAll("div, button, #createForm input, #createForm");
  elements.forEach((el) => {
    var isBlack = 0;
    var isWhite = 0;
    if (el.isBlack !== undefined) isBlack = el.isBlack;
    else if (el.isWhite !== undefined) isWhite = el.isWhite;
    else {
      const currentColor = window.getComputedStyle(el).color;
      isBlack = currentColor === "rgb(0, 0, 0)";
      isWhite = currentColor === "rgb(255, 255, 255)";
    }
    if (flag && isBlack) {
      el.isWhite = true;
      el.style.color = "white";
      if (el.tagName === "INPUT" || el.tagName === "BUTTON") {
        el.style.backgroundColor = "rgba(0, 0, 0, 0.8)";
      }
    } else if (!flag && isWhite) {
      el.isBlack = true;
      el.style.color = "black";
      if (el.tagName === "INPUT" || el.tagName === "BUTTON") {
        el.style.backgroundColor = "rgba(255, 255, 255, 0.9)";
      }
    }
  });
}
