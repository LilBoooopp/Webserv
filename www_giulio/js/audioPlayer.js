function uploadFile(files) {
  for (let i = 0; i < files.length; i++) {
    const file = files[i];
    const name = file.name;
    const dot = name.lastIndexOf(".");
    if (dot <= 0) {
      announce(`Can't upload ${name} - not an audio file`);
      continue;
    }
    const ext = name.slice(dot + 1).toLowerCase();
    if (ext !== "mp3") {
      announce(`Can't upload ${name} - wrong format: ${ext}`, 3000, "red");
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
        announce(`Server received file in ${dt}s, response: ${t.trim()}`);
      })
      .catch((err) => announce("Upload failed: " + err));
  }
}

function initAudioPlayer() {
  let audioIndex = 0;
  const audioPath = "../ressources/audio/";
  const audioFiles = ["Macintosh", "Petaluma", "Forecast", "Resonance"];
  const artits = ["Vektroid", "Cocktail Shakers Girl", "Kevin MacLeod", "HOME"];
  const durations = new Array(audioFiles.length).fill(null);

  for (let i = 0; i < audioFiles.length; i++) {
    const audio = new Audio(audioPath + audioFiles[i] + ".mp3");
    audio.addEventListener("loadedmetadata", function () {
      durations[i] = audio.duration;
    });
  }

  const p = [100, 50];
  const label = addDiv(audioFiles[0], [p[0] - 40, p[1] + 50], "white", "black", null, false);
  label.style.transform = "scale(0.8)";
  label.style.transformOrigin = "0 0";
  const artistLabel = addDiv(artits[0], [p[0] - 40, p[1] + 65], "white", "black", null, false);
  artistLabel.style.transform = "scale(0.6)";
  artistLabel.style.transformOrigin = "0 0";
  var dur = [];
  for (let i = 0; i < audioFiles.length; i++) {
    var mus = new Audio(audioPath + audioFiles[i] + ".mp3");
    mus.addEventListener("loadedmetadata", function handler() {
      dur.push(mus.duration);
    });
  }
  var mus = new Audio(audioPath + audioFiles[0] + ".mp3");

  function switchTrack(dir) {
    const wasPlaying = !mus.paused && mus.currentTime > 0 && !mus.ended;
    audioIndex += dir;
    if (audioIndex < 0) audioIndex = audioFiles.length - 1;
    else if (audioIndex >= audioFiles.length) audioIndex = 0;
    mus.pause();
    mus.src = audioPath + audioFiles[audioIndex] + ".mp3";
    mus.currentTime = 0;
    if (wasPlaying) mus.play();
    label.textContent = audioFiles[audioIndex];
    artistLabel.textContent = artits[audioIndex];
  }

  const playButton = initImage(30, 30, p[0] - 12, p[1], "../ressources/img/play.png");
  playButton.style.scale = 0.6;
  function toggleAudio() {
    const isPlaying = !mus.paused && mus.currentTime > 0 && !mus.ended;
    if (!isPlaying) {
      mus.play();
      playButton.src = "../ressources/img/pause.png";
    } else {
      mus.pause();
      playButton.src = "../ressources/img/play.png";
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
  playButton.style.userSelect = "none";

  const backButton = initImage(30, 30, p[0] - 40, p[1], "../ressources/img/efback.png");
  backButton.style.scale = 0.6;
  backButton.addEventListener("mouseenter", () => (backButton.style.scale = 0.8));
  backButton.addEventListener("mouseleave", () => (backButton.style.scale = 0.6));
  backButton.addEventListener("mousedown", () => switchTrack(-1));
  backButton.style.userSelect = "none";
  const nextButton = initImage(30, 30, p[0] + 15, p[1], "../ressources/img/effronte.png");
  nextButton.style.scale = 0.6;
  nextButton.addEventListener("mouseenter", () => (nextButton.style.scale = 0.8));
  nextButton.addEventListener("mouseleave", () => (nextButton.style.scale = 0.6));
  nextButton.addEventListener("mousedown", () => switchTrack(1));
  nextButton.style.userSelect = "none";

  const timeBox = writeBox(80, 10, p[0] - 38, p[1] + 35, "black");
  const curBox = writeBox(0, 10, p[0] - 38, p[1] + 35, "white");
  timeBox.addEventListener("mousedown", (e) => {
    if (mus.paused || !durations[audioIndex]) return;
    const rect = timeBox.getBoundingClientRect();
    const ratio = (e.clientX - rect.left) / rect.width;
    const clamped = Math.max(0, Math.min(1, ratio));
    const target = durations[audioIndex] * clamped;
    if (mus.readyState >= 1) {
      mus.currentTime = 5;
      console.warn(mus.currentTime);
    }
  });
  setInterval(() => {
    if (!mus.paused && durations[audioIndex]) {
      const ratio = mus.currentTime / durations[audioIndex];
      curBox.style.width = ratio * timeBox.offsetWidth + "px";
    }
  }, 500);
  const x = p[0] - 38;
  const w = 80;
  function setLevel(mx) {
    const max = w;
    const p = mx - x;
    const n = p < 5 ? 5 : p > max ? max : p;
    const v = n < 5.1 ? 0 : n / max;
    const percent = (p / max) * 100;
    mus.volume = v;
    audioHandle.style.background = `linear-gradient(to right, 
    ${"blue"} 0%, 
    ${"blue"} ${percent}%, 
    ${"red"} ${percent}%, 
    ${"red"} 100%)`;
    audioHandle.selected = true;
  }
  const audioHandle = writeBox(w, 5, x, p[1] + w, "blue");
  audioHandle.selected = false;
  audioHandle.addEventListener("mousedown", (e) => {
    setLevel(e.clientX);
  });
  window.addEventListener("mouseup", () => {
    audioHandle.selected = false;
  });
  window.addEventListener("mousemove", (e) => {
    if (audioHandle.selected) setLevel(e.clientX);
  });

  const button = addButton("Upload", [p[0], p[1] + 120], () => openFileDialog(uploadFile), "black", "rgba(255, 255, 255, 0.24)");
  button.style.scale = 0.8;
}
