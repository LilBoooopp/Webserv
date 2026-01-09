function openFileDialog(callback) {
  const input = document.createElement("input");
  input.type = "file";
  input.multiple = true;
  input.accept = "*/*";

  input.onchange = () => {
    if (callback) callback(input.files);
  };
  input.click();
}

function uploadFile(files) {
  for (let i = 0; i < files.length; i++) {
    const file = files[i];
    const name = file.name;
    const dot = name.lastIndexOf(".");
    if (dot <= 0) {
      announce("Can't upload " + name + " - no extension");
      return;
    }
    const ext = name.slice(dot + 1).toLowerCase();
    const start = performance.now();

    fetch("/upload", {
      method: "POST",
      headers: {
        "Content-Type": file.type || "application/octet-stream",
        "X-Filename": name,
      },
      body: file,
    })
      .then((r) => r.text())
      .then((t) => {
        const dt = Number((performance.now() - start) / 1000).toFixed(3);
        announce("Server received file in " + dt + "s, response: " + t.trim());
      })
      .catch((err) => announce("Upload failed: " + err));
  }
}

function addScrollerMenu(label, p, options, onClick) {
  const d = addButton(label, p);
  d.buttons = [];
  d.hov = null;
  d.bgr = writeBox(60, 30 * options.length, p[0] - 30, p[1] + 15, "rgba(0, 0, 0, 0.04)");
  d.bgr.style.zIndex = -100;
  d.bgr.style.display = "none";

  for (let i = 0; i < options.length; i++) {
    const handler = onClick && onClick[i] ? onClick[i] : null;

    const optI = addButton(options[i], [p[0], p[1] + (i + 1) * 30], handler, "white", "rgba(0, 0, 0, 0)");
    optI.style.scale = ".8";
    optI.style.display = "none";

    d.buttons.push(optI);

    optI.addEventListener("mouseenter", () => {
      optI.style.scale = 1;
      d.hov = optI;
      d.bgr.style.display = "block";
    });

    optI.addEventListener("mouseleave", () => {
      optI.style.scale = 0.8;
      setTimeout(() => {
        if (d.hov === optI) {
          for (const o of d.buttons) o.style.display = "none";
          d.hov = null;
          d.bgr.style.display = "none";
        }
      }, 100);
    });
  }

  d.addEventListener("mouseenter", () => {
    d.hov = d;
    for (const o of d.buttons) o.style.display = "block";
    d.bgr.style.display = "block";
  });

  d.addEventListener("mouseleave", () => {
    if (d.hov === d) d.hov = null;
    setTimeout(() => {
      if (!d.hov) {
        for (const o of d.buttons) o.style.display = "none";
        d.bgr.style.display = "none";
      }
    }, 100);
  });
  return d;
}

function addInputField(label, p, clr, bgrClr) {
  let div = document.createElement("input");
  div.placeholder = label;

  div.style.position = "absolute";

  div.style.left = (p[0] / window.innerWidth) * 100 + "%";
  div.style.top = (p[1] / window.innerHeight) * 100 + "%";
  div.style.transform = "translate(-50%, -50%) scale(" + 1 + ")";

  div.style.transformOrigin = "center center";

  div.style.userSelect = "none";
  if (clr) div.style.color = clr;
  if (bgrClr) div.style.backgroundColor = bgrClr;

  document.body.appendChild(div);
  return div;
}
