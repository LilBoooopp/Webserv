function addButton(label, pos = [0, 0], onEnd = null, color = null, bgrClr = null) {
  let div = document.createElement("div");
  div.textContent = label;

  div.style.display = "inline-block";
  div.style.padding = "8px 12px";
  div.style.position = "absolute";
  div.style.userSelect = "none";
  div.style.backgroundColor = "rgba(255, 255, 255, 0.15)";
  div.style.color = "black";
  div.style.cursor = "pointer";
  div.style.borderRadius = "5px";
  if (color) div.style.color = color;
  else if (window.DARKMODE !== "undefined") div.style.color = window.DARKMODE === 0 ? "black" : "white";
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

  div.style.left = (p[0] / window.innerWidth) * 100 + "%";
  div.style.top = (p[1] / window.innerHeight) * 100 + "%";

  return div;
}

function moveToUrl(text) {
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
      .then(async (res) => {
        const body = await res.text();
        const ct = res.headers.get("Content-Type") || "";

        // if (!res.ok && ct.includes("text/html")) {
        // 	document.open();
        // 	document.write(body);
        // 	document.close();
        // 	return;
        // }

        const responseText = body.trim();

        // ✅ Cas succès logique
        if (onEnd) {
          onEnd(responseText, res);
        } else {
          // ✅ Erreur applicative (401, 403, 500 en text/plain)
          announce(responseText || `Error ${res.status}`);
        }
      })
      .catch((e) => {
        console.warn("Network error:", e);
        announce("Network / server unreachable");
      });
  });

  return but;
}

// function cgiButton(label, scriptPath, pos = [0, 0], arg = null, onEnd = moveToUrl) {
//   const but = addButton(label, pos);
//   but.addEventListener("mousedown", () => {
//     let url = scriptPath;
//     if (arg != null) url += "?arg=" + encodeURIComponent(arg);
//     fetch(url)
//       .then((r) => r.text().then((t) => ({ r, t })))
//       .then(({ r, t }) => {
//         const responseText = t.trim();
//         if (onEnd) onEnd(responseText);
//       });
//   });
//   return but;
// }

function loopRequest() {
  const counter = 1000;
  let completed = 0;
  var start = performance.now();
  var div = announce("SENT 0 requests", 6000);
  for (let i = 0; i < counter; i++) {
    fetch("/").then((r) => {
      if (!r.ok) {
        var mult = completed > 1 ? "s" : "";
        div.textContent = `requests err returned ${r.status} ${r.statusText} at ${completed}${mult} request`;
        return;
      }
      completed++;
      if (completed >= counter) div.textContent = `${completed} requests processed in ${Number((performance.now() - start) / 1000).toFixed(3)}s`;
      else div.textContent = `${completed}'s responses from Server`;
    });
  }
}

function postBytes(size) {
  const length = size;
  const buf = new Uint8Array(length);

  for (let i = 0; i < length; i++) buf[i] = 65;

  const textEncoder = new TextEncoder();
  const header = textEncoder.encode("TEST ");

  const body = new Blob([header, buf]);

  const start = performance.now();
  fetch("/upload", {
    method: "POST",
    headers: {
      "Content-Type": "text/plain; charset=utf-8",
      "X-Upload-Path": size + ".txt",
    },
    body,
  })
    .then((r) => r.text().then((t) => ({ r, t })))
    .then(({ r, t }) => {
      const responseText = t.trim();
      announce(`Server received package in ${Number((performance.now() - start) / 1000).toFixed(3)}s, response: ${responseText}`);
    })
    .catch((e) => {
      console.warn("Upload error:", e);
      announce("Upload failed: " + (e.message || "Network error"));
    });
}

function addHomeButton() {
  const c = [window.innerWidth / 2, window.innerHeight / 2];
  addButton("HOME", [c[0], window.innerHeight - 40], () => (window.location.href = "/"));
}

function addToggleButton(label, p, active, onClick) {
  p[0] += 50;
  const clrs = ["rgba(255, 0, 0, .4)", "rgba(0, 255, 4, 0.4)"];
  const div = addButton(
    "",
    p,
    () => {
      div.active = !div.active;
      div.style.backgroundColor = clrs[div.active === true ? 1 : 0];
      onClick(div.active);
    },
    null,
    clrs[active === true ? 1 : 0]
  );
  div.active = active;
  let labelDiv = addDiv(label, [p[0] - 70, p[1]], 1, "grey");
  document.body.appendChild(labelDiv);
  document.body.appendChild(div);
  return div;
}

function addDeleteAccountButton(p) {
  addButton("Delete Account", [p[0], p[1]], () => {
    fetch("/cgi/auth/sessionManagment/delete.php", {
      method: "POST",
    })
      .then(async (res) => {
        const msg = await res.text();
        if (res.ok && msg.trim() === "OK") {
          window.location.href = "/login.html";
        } else {
          announce(msg || "account deletion failed");
        }
      })
      .catch(() => {
        announce("error while deleting account");
      });
  });
}

function addInfiniteRequestButton(p) {
  addButton("No Timeout CGI", [p[0], p[1]], () => {
    fetch("/cgi/test/infinite.py", {
      method: "POST",
      headers: {
        "X-Async": "1",
      },
    })
      .then(async (res) => {
        const msg = await res.text();
        if (res.ok && msg.trim() === "OK") {
        } else {
          announce(msg || "no Timeout CGI failed");
        }
      })
      .catch(() => {
        announce("error in No Timeout CGI call");
      });
  });
}

function addToggleButton(label, p, startActive, onSwitch) {
  var lab = addDiv(label, [p[0] - label.length * 8, p[1]], 0.5);
  var fill = writeBox(20, 8, p[0] - 5, p[1] - 4, "rgba(0, 0, 0, 0.25)");
  fill.style.borderRadius = "10%";
  var handle = writeBox(10, 10, p[0] + (startActive ? 5 : 0), p[1] - 5, "white");
  handle.lastToggle = 0;
  handle.style.borderRadius = "50%";
  handle.style.transition = "transform 0.2s ease-out";
  handle.value = startActive;
  var btn = addButton("", p, () => {
    if (performance.now() - handle.lastToggle < 500) return;
    onSwitch();
    handle.lastToggle = performance.now();
    handle.value = !handle.value;
    var am = handle.value ? 0 : 10;
    if (startActive) am *= -1;
    handle.style.transform = "translateX(" + am + "px)";
  });
  btn.style.width = "10px";
}

function addDarkModeButton(offset = [0, 0]) {
  var p = [window.innerWidth - 50 + offset[0], 40 + offset[1]];
  addToggleButton("DARK", p, true, () => {
    window.DARKMODE = !window.DARKMODE;
    if (window.CURRENT_USER !== undefined) {
      const body = new URLSearchParams({ darkmode: window.DARKMODE ? 1 : 0 });
      fetch("/cgi/auth/setters/setDarkmode.php", {
        method: "POST",
        credentials: "same-origin",
        headers: {
          "Content-Type": "application/x-www-form-urlencoded",
          "X-Darkmode": window.DARKMODE ? "1" : "0",
        },
        body: body.toString(),
      }).catch(() => announce("Can't save darkmode"));
    }
    applyBackground(window.DARKMODE);
  });
}

function addBackButton(p) {
  var backBtn = addButton("../", p, () => {
    const params = new URLSearchParams(window.location.search);
    const dir = params.get("dir") || "";

    if (dir) {
      const last = dir.lastIndexOf("/");
      const parentDir = last > 0 ? dir.slice(0, last) : "";
      const newUrl = window.location.pathname + (parentDir ? "?dir=" + encodeURIComponent(parentDir) : "");
      window.location.href = newUrl;
    } else {
      window.location.href = window.location.pathname;
    }
  });
  return backBtn;
}
