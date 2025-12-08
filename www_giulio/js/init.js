function initAudioPlayer() {
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

  const p = [100, 50];
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

  const playButton = initImage(30, 30, p[0] - 12, p[1], "../ressources/img/play.png");
  playButton.style.scale = 0.6;
  function toggleAudio() {
    if (!mus) return;
    const au = mus.au;
    const isPlaying = !au.paused && au.currentTime > 0 && !au.ended;
    if (!isPlaying) {
      au.play();
      playButton.src = "../ressources/img/pause.png";
    } else {
      au.pause();
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

  const backButton = initImage(30, 30, p[0] - 40, p[1], "../ressources/img/efback.png");
  backButton.style.scale = 0.6;
  backButton.addEventListener("mouseenter", () => (backButton.style.scale = 0.8));
  backButton.addEventListener("mouseleave", () => (backButton.style.scale = 0.6));
  backButton.addEventListener("mousedown", () => switchTrack(-1));

  const nextButton = initImage(30, 30, p[0] + 15, p[1], "../ressources/img/effronte.png");
  nextButton.style.scale = 0.6;
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

  const audioLevel = writeBox(80, 5, p[0] - 38, p[1] + 80, "red");

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

function init() {
  initAudioPlayer();
  const c = [window.innerWidth / 2, window.innerHeight * 0.4];
  const colSpacing = 200;
  const rowSpacing = 50;

  const menus = ["Navigation", "Requests", "CGI", "Post"];
  const totalWidth = (menus.length - 1) * colSpacing;

  const colX = [];
  for (let i = 0; i < menus.length; i++) {
    const x = c[0] - totalWidth / 2 + i * colSpacing;
    colX.push(x);
    const div = addDiv(menus[i], [x, c[1] - 40], 1.5, "white");
  }
  addDiv("WebServ", [c[0], c[1] - 150], 3, "white");

  const navX = colX[0];
  const reqX = colX[1];
  const cgiX = colX[2];
  const postX = colX[3];

  cgiButton("LOGIN", "cgi/printArg.py", [navX, c[1] + 25], "login.html");

  addButton("SPAM", [reqX, c[1] + 25], loopRequest);
  addButton("not found", [reqX, c[1] + 25 + rowSpacing], () => (window.location.href = "/notfound"));
  addButton("bad url", [reqX, c[1] + 25 + rowSpacing * 2], () => (window.location.href = "awfin/qwq1||//#"));

  const cgPaths = ["print.cgi", "printArg.py", "infinite.py", "hugeResponse.py"];
  for (let i = 0; i < cgPaths.length; i++) {
    cgiButton(cgPaths[i], "cgi/" + cgPaths[i], [cgiX, c[1] + 25 + rowSpacing * i], "Cool", announceResponse);
  }

  var y = 1;
  addButton("File", [postX, c[1] + 25], () => openFileDialog(uploadFile));
  addButton("1 Byte", [postX, c[1] + 25 + rowSpacing], () => postBytes(1));
  addButton("1 MB", [postX, c[1] + 25 + ++y * rowSpacing], () => postBytes(1_000_000));
  addButton("10 MB", [postX, c[1] + 25 + ++y * rowSpacing], () => postBytes(10_000_000));
  addButton("1 GB", [postX, c[1] + 25 + ++y * rowSpacing], () => postBytes(999_999_999));
}
