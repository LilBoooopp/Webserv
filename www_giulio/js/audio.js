function initAudioPlayer(p = [100, 50]) {
  let audioIndex = 0;
  const audioPath = "../ressources/audio/";
  const trackNames = ["Resonance", "Macintosh", "Petaluma", "Forecast"];
  const artits = ["HOME", "Vektroid", "Cocktail Shakers Girl", "Kevin MacLeod"];

  const audioFiles = new Array(trackNames.length);
  for (let i = 0; i < trackNames.length; i++) {
    const audio = new Audio(audioPath + trackNames[i] + ".mp3");
    audioFiles[i] = { au: audio, dur: 0, name: trackNames[i], artist: artits[i], isUploaded: false };
    audio.addEventListener("loadedmetadata", function () {
      audioFiles[i].dur = audio.duration;
    });
  }

  function addDeleteButton(pos) {
    var button = addButton("X", pos, () => {
      if (!mus.isUploaded) {
        return;
      }
      fetch(`/delete`, {
        method: "DELETE",
        headers: { "X-Delete-Path": "audio/" + mus.name + ".mp3" },
      })
        .then((r) => {
          if (r.ok) window.location.reload();
        })
        .catch((err) => console.warn("Failed to delete uploaded audio:", err));
    });
    button.style.scale = ".3";
    button.addEventListener("mouseenter", () => (button.style.scale = ".5"));
    button.addEventListener("mouseleave", () => (button.style.scale = ".3"));
    button.style.display = "none";
    return button;
  }

  function loadUploadedAudio() {
    fetch("/cgi/list.php?dir=audio")
      .then((r) => r.json())
      .then((files) => {
        files.forEach((file) => {
          if ((file.name.endsWith(".mp3") || file.name.endsWith(".wav")) && !file.isDir) {
            const idx = audioFiles.length;
            const audio = new Audio("../ressources/uploads/audio/" + file.name);
            audioFiles.push({
              au: audio,
              dur: 0,
              name: file.name.replace(/\.(mp3|wav)$/i, ""),
              artist: "Uploaded",
              isUploaded: true,
            });
            audio.addEventListener("loadedmetadata", function () {
              audioFiles[idx].dur = audio.duration;
            });
          }
        });
      })
      .catch((err) => console.warn("Failed to load uploaded audio:", err));
  }

  loadUploadedAudio();
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
    // curBox.style.width = "0px";
    delBtn.style.display = mus.isUploaded ? "block" : "none";
  }

  function toggleAudio() {
    if (!mus) return;
    const au = mus.au;
    const isPlaying = !au.paused && au.currentTime > 0 && !au.ended;
    if (!isPlaying) {
      au.play();
      playButton.src = "../ressources/img/audioIcons/pause.png";
    } else {
      au.pause();
      playButton.src = "../ressources/img/audioIcons/play.png";
    }
  }
  window.addEventListener("keydown", (e) => {
    if (e.code === "Space") toggleAudio();
    else if (e.key === "d") switchTrack(1);
    else if (e.key === "a") switchTrack(-1);
  });
  const playButton = initImage(30, 30, p[0] - 12, p[1], "../ressources/img/audioIcons/play.png");
  playButton.classList.add("audio-button");
  playButton.addEventListener("mousedown", () => toggleAudio());

  const backButton = initImage(30, 30, p[0] - 40, p[1], "../ressources/img/audioIcons/efback.png");
  backButton.classList.add("audio-button");
  backButton.addEventListener("mousedown", () => switchTrack(-1));

  const nextButton = initImage(30, 30, p[0] + 15, p[1], "../ressources/img/audioIcons/effronte.png");
  nextButton.classList.add("audio-button");
  nextButton.addEventListener("mousedown", () => switchTrack(1));

  // const timeBox = writeBox(80, 10, p[0] - 38, p[1] + 35, "black");
  // const curBox = writeBox(0, 10, p[0] - 38, p[1] + 35, "white");
  // function updateCurBox() {
  // 	const au = mus.au;
  // 	const dur = mus.dur;
  // 	var paused = false;
  // 	if (au.currentTime >= mus.dur) {
  // 		au.currentTime = 0;
  // 		paused = true;
  // 	}
  // 	if (!au.paused && dur) {
  // 		const ratio = au.currentTime / dur;
  // 		curBox.style.width = ratio * timeBox.offsetWidth + "px";
  // 	}
  // 	if (paused) {
  // 		au.pause();
  // 		playButton.src = "../ressources/img/audioIcons/play.png";
  // 	}
  // }
  // timeBox.addEventListener("mousedown", (e) => {
  // 	if (!mus) return;
  // 	const au = mus.au;
  // 	const dur = mus.dur;
  // 	if (au.paused || !dur) return;
  // 	const rect = timeBox.getBoundingClientRect();
  // 	const ratio = (e.clientX - rect.left) / rect.width;
  // 	const clamped = Math.max(0, Math.min(1, ratio));
  // 	au.currentTime = dur * clamped;
  // });
  // setInterval(() => {
  // 	if (mus) updateCurBox();
  // }, 10);

  function moveHandle(e) {
    const rect = AudioLevelBackground.getBoundingClientRect();
    const ratio = (e.clientX - rect.left) / rect.width;
    const clamped = Math.max(0, Math.min(0.95, ratio));
    handle.style.left = AudioLevelBackground.offsetLeft + clamped * AudioLevelBackground.offsetWidth - handle.offsetWidth / 2 + "px";
    fillbackground.style.width = 5 + parseFloat(handle.style.left) - AudioLevelBackground.offsetLeft + "px";
    if (!mus) return;
    mus.au.volume = ratio < 0.01 ? 0 : ratio > 1 ? 1 : ratio;
  }
  var volumeSliderY = p[1] + 40;
  var AudioLevelBackground = writeBox(80, 5, p[0] - 38, volumeSliderY, "black");
  var fillbackground = writeBox(75, 5, p[0] - 38, volumeSliderY, "white");
  fillbackground.style.pointerEvents = "none";
  AudioLevelBackground.style.borderRadius = "10%";
  AudioLevelBackground.addEventListener("mousedown", (e) => {
    moveHandle(e);
    handle.isSel = true;
  });
  var handle = writeBox(10, 10, p[0] - 38 + 70, volumeSliderY - 2.5, "grey");
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

  var labP = [p[0] - 38, volumeSliderY + 10];
  const label = addDiv(trackNames[0], labP, null, null, null, false);
  label.style.transform = "scale(0.8)";
  label.style.transformOrigin = "0 0";
  const artistLabel = addDiv(artits[0], [labP[0], labP[1] + 20], null, null, null, false);
  artistLabel.style.transform = "scale(0.6)";
  artistLabel.style.transformOrigin = "0 0";
  var delBtn = addDeleteButton([labP[0] + 50, labP[1] + 25]);

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
          "X-Upload-Path": "audio/" + name,
        },
        body: file,
      })
        .then((r) => r.text())
        .then((t) => {
          const dt = Number((performance.now() - start) / 1000).toFixed(3);
          announce("Server received file in " + dt + "s, response: " + t.trim());
          const dot = name.lastIndexOf(".");
          const baseName = dot !== -1 ? name.slice(0, dot) : name;
          const audio = new Audio("../ressources/uploads/audio/" + baseName + ".mp3");
          const idx = audioFiles.length;
          audioFiles.push({
            au: audio,
            dur: 0,
            name: file.name.replace(/\.(mp3|wav)$/i, ""),
            artist: "Uploaded",
            isUploaded: true,
          });
          audio.addEventListener("loadedmetadata", function () {
            audioFiles[idx].dur = audio.duration;
            audioIndex = 0;
            switchTrack(idx);
          });
        })
        .catch((err) => announce("Upload failed: " + err));
    }
  }

  const button = addButton("Upload", [p[0], volumeSliderY + 65], () => openFileDialog(uploadFile), null, "rgba(255, 255, 255, 0.24)");
  button.style.backgroundColor = "rgba(255, 255, 255, 0.01)";
  button.style.scale = 0.8;
  button.addEventListener("mouseenter", () => (button.style.scale = 1));
  button.addEventListener("mouseleave", () => (button.style.scale = 0.8));
}
function playSound(src, volume = 1) {
  var au = new Audio(src);
  au.volume = volume;
  au.play();
}

function playClick() {
  playSound("/ressources/audio/click.mp3", 1);
}
