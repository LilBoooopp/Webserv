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
