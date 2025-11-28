function addInputDiv(label, pos, onEnd = null) {
  let input = document.createElement("input");
  input.type = "text";

  input.id = label;
  addDiv(label, [pos[0] - 30, pos[1] + 10], 1);

  input.style.position = "absolute";
  input.style.left = (pos[0] / window.innerWidth) * 100 + "%";
  input.style.top = (pos[1] / window.innerHeight) * 100 + "%";

  document.body.appendChild(input);

  if (!onEnd) return input;
  input.addEventListener("change", () => {
    onEnd();
    input.blur();
  });
  return input;
}

function addFormulary(pos = [50, 50]) {
  const inputFields = ["name", "age"];

  pos[0] -= 60;
  function sendFormulary(okButton) {
    const l = [];
    for (const field of okButton.fields) {
      l.push(`${field.label}: ${field.input.value}`);
    }
    console.warn(`Form Sent: ${l.join(", ")}`);
  }

  const okButton = addButton("OK", [pos[0] + 60, pos[1] + 40 + (inputFields.length + 1) * 40]);

  okButton.fields = [];

  okButton.addEventListener("mousedown", () => sendFormulary(okButton));

  let y = pos[1] + 50;
  for (const label of inputFields) {
    const input = addInputDiv(label, [pos[0], y]);
    okButton.fields.push({ label, input });
    y += 40;
  }
}

function openFileDialog(callback) {
  const input = document.createElement("input");
  input.type = "file";
  input.accept = "*/*";

  input.onchange = () => {
    if (callback) callback(input.files);
  };
  input.click();
}

function uploadFile(files) {
  console.warn("YOOOOOOO" + files.length);
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
