function addInputDiv(label, pos, onEnd = null) {
  let input = document.createElement("input");
  input.type = "text";

  input.id = label;
  addDiv(label, [pos[0] - 30, pos[1] + 10], 1);

  input.style.position = "absolute";
  input.style.left = pos[0] + "px";
  input.style.top = pos[1] + "px";

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
