function initAudioPlayer(p = [100, 50]) {
  let audioIndex = 0;
  const audioPath = "../ressources/audio/";
  const trackNames = ["Macintosh", "Petaluma", "Forecast", "Resonance"];
  const artits = ["Vektroid", "Cocktail Shakers Girl", "Kevin MacLeod", "HOME"];

  const audioFiles = new Array(trackNames.length);
  for (let i = 0; i < trackNames.length; i++) {
    const audio = new Audio(audioPath + trackNames[i] + ".mp3");
    audioFiles[i] = { au: audio, dur: 0, name: trackNames[i], artist: artits[i] };
    audio.addEventListener("loadedmetadata", function () {
      audioFiles[i].dur = audio.duration;
    });
  }

  const label = addDiv(trackNames[0], [p[0] - 40, p[1] + 50], "white", "black", null, false);
  label.style.transform = "scale(0.8)";
  label.style.transformOrigin = "0 0";
  const artistLabel = addDiv(artits[0], [p[0] - 40, p[1] + 65], "white", "black", null, false);
  artistLabel.style.transform = "scale(0.6)";
  artistLabel.style.transformOrigin = "0 0";

  let mus = audioFiles[0];

  function switchTrack(dir) {
    const au = mus.au;
    const wasPlaying = !au.paused && au.currentTime > 0 && !au.ended;
    audioIndex += dir;
    if (audioIndex < 0) audioIndex = audioFiles.length - 1;
    else if (audioIndex >= audioFiles.length) audioIndex = 0;
    au.pause();
    mus = audioFiles[audioIndex];
    if (!mus) return;
    mus.au.currentTime = 0;
    if (wasPlaying) mus.au.play();
    label.textContent = mus.name;
    artistLabel.textContent = mus.artist;
  }

  const playButton = initImage(30, 30, p[0] - 12, p[1], "../ressources/img/audioPlayer/play.png");
  playButton.style.userSelect = "none";
  playButton.style.scale = 0.6;
  function toggleAudio() {
    if (!mus) return;
    const au = mus.au;
    const isPlaying = !au.paused && au.currentTime > 0 && !au.ended;
    if (!isPlaying) {
      au.play();
      playButton.src = "../ressources/img/audioPlayer/pause.png";
    } else {
      au.pause();
      playButton.src = "../ressources/img/audioPlayer/play.png";
    }
  }
  window.addEventListener("keydown", (e) => {
    if (e.code === "Space") toggleAudio();
    else if (e.key === "d") switchTrack(1);
    else if (e.key === "a") switchTrack(-1);
  });
  playButton.addEventListener("mousedown", () => toggleAudio());
  playButton.addEventListener("mouseenter", () => (playButton.style.scale = 0.8));
  playButton.addEventListener("mouseleave", () => (playButton.style.scale = 0.6));

  const backButton = initImage(30, 30, p[0] - 40, p[1], "../ressources/img/audioPlayer/efback.png");
  backButton.style.scale = 0.6;
  backButton.style.userSelect = "none";
  backButton.addEventListener("mouseenter", () => (backButton.style.scale = 0.8));
  backButton.addEventListener("mouseleave", () => (backButton.style.scale = 0.6));
  backButton.addEventListener("mousedown", () => switchTrack(-1));

  const nextButton = initImage(30, 30, p[0] + 15, p[1], "../ressources/img/audioPlayer/effronte.png");
  nextButton.style.scale = 0.6;
  nextButton.style.userSelect = "none";
  nextButton.addEventListener("mouseenter", () => (nextButton.style.scale = 0.8));
  nextButton.addEventListener("mouseleave", () => (nextButton.style.scale = 0.6));
  nextButton.addEventListener("mousedown", () => switchTrack(1));

  const timeBox = writeBox(80, 10, p[0] - 38, p[1] + 35, "black");
  const curBox = writeBox(0, 10, p[0] - 38, p[1] + 35, "white");
  timeBox.addEventListener("mousedown", (e) => {
    if (!mus) return;
    const au = mus.au;
    const dur = mus.dur;
    if (au.paused || !dur) return;
    const rect = timeBox.getBoundingClientRect();
    const ratio = (e.clientX - rect.left) / rect.width;
    const clamped = Math.max(0, Math.min(1, ratio));
    au.currentTime = dur * clamped;
  });
  setInterval(() => {
    if (!mus) return;
    const au = mus.au;
    const dur = mus.dur;
    if (!au.paused && dur) {
      const ratio = au.currentTime / dur;
      curBox.style.width = ratio * timeBox.offsetWidth + "px";
    }
  }, 10);

  function moveHandle(e) {
    const rect = AudioLevelBackground.getBoundingClientRect();
    const ratio = (e.clientX - rect.left) / rect.width;
    const clamped = Math.max(0, Math.min(1, ratio));
    handle.style.left = AudioLevelBackground.offsetLeft + clamped * AudioLevelBackground.offsetWidth - handle.offsetWidth / 2 + "px";
    fillbackground.style.width = parseFloat(handle.style.left) - AudioLevelBackground.offsetLeft + "px";
    if (!mus) return;
    mus.au.volume = ratio < 0.01 ? 0 : ratio > 1 ? 1 : ratio;
  }
  var AudioLevelBackground = writeBox(80, 5, p[0] - 38, p[1] + 80, "white");
  var fillbackground = writeBox(80, 5, p[0] - 38, p[1] + 80, "black");
  fillbackground.style.pointerEvents = "none";
  AudioLevelBackground.style.borderRadius = "10%";
  AudioLevelBackground.addEventListener("mousedown", (e) => {
    moveHandle(e);
    handle.isSel = true;
  });
  var handle = writeBox(10, 10, p[0] - 38 + 75, p[1] + 77.5, "black");
  handle.addEventListener("mousedown", () => (handle.isSel = true));
  handle.style.borderRadius = "50%";
  handle.addEventListener("mouseenter", () => (handle.style.scale = "1.2"));
  handle.addEventListener("mouseleave", () => (handle.style.scale = "1"));
  window.addEventListener("mousemove", (e) => {
    if (handle.isSel) moveHandle(e);
  });
  window.addEventListener("mouseup", (e) => {
    handle.isSel = false;
  });

  function uploadFile(files) {
    for (let i = 0; i < files.length; i++) {
      const file = files[i];
      const name = file.name;
      const dot = name.lastIndexOf(".");
      if (dot <= 0) {
        announce("Can't upload " + name + " - not an audio file");
        continue;
      }
      const ext = name.slice(dot + 1).toLowerCase();
      if (ext !== "mp3" && ext !== "wav") {
        announce("Can't upload " + name + " - wrong format: " + ext, 3000, "red");
        continue;
      }
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
          const dot = name.lastIndexOf(".");
          const baseName = dot !== -1 ? name.slice(0, dot) : name;
          const audio = new Audio("../ressources/uploads/" + baseName + ".mp3");
          const idx = audioFiles.length;
          audioFiles.push({ au: audio, dur: 0, name: baseName, artist: "" });
          audio.addEventListener("loadedmetadata", function () {
            audioFiles[idx].dur = audio.duration;
            audioIndex = 0;
            switchTrack(idx);
          });
        })
        .catch((err) => announce("Upload failed: " + err));
    }
  }

  const button = addButton("Upload", [p[0], p[1] + 120], () => openFileDialog(uploadFile), "black", "rgba(255, 255, 255, 0.24)");
  button.style.scale = 0.8;
  button.addEventListener("mouseenter", () => (button.style.scale = 1));
  button.addEventListener("mouseleave", () => (button.style.scale = 0.8));
}


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
}
