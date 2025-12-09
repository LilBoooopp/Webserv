function handleAuth(e) {
  e.preventDefault();

  const form = e.target;
  const btn = e.submitter;
  const action = btn ? btn.value : "login";

  const formData = new FormData(form);
  const username = formData.get("user") || "";
  const password = formData.get("pass") || "";

  const url = "/cgi/auth/" + (action === "login" ? "login.php" : "register.php");

  fetch(url, {
    method: "POST",
    headers: {
      USERNAME: username,
      PASSWORD: password,
    },
  })
    .then(async (res) => {
      const body = await res.text();
      const msg = body.trim();

      if (res.ok && msg === "OK") {
        if (action === "login") {
          window.location.href = "/cgi/auth/securePage/account.php";
        } else {
          announce("Account created");
        }
        return;
      }

      const ct = res.headers.get("Content-Type") || "";
      if (ct.includes("text/html")) {
        document.open();
        document.write(body);
        document.close();
      } else {
        console.warn("Auth failed:", res.status, body);
        announce(msg || "Authentication failed");
      }
    })
    .catch((err) => {
      console.warn("Network/server error:", err);
      announce("Server error");
    });
}

function logout() {
  const url = "/cgi/auth/logout.php";
  fetch(url, {
    method: "POST",
    credentials: "include",
  })
    .then(async (res) => {
      const msg = await res.text();
      if (res.ok && msg.trim() === "OK") {
        window.location.href = "/login.html";
      } else {
        announce("Logout failed");
      }
    })
    .catch(() => announce("Server error"));
}

function addLogoutButton() {
  const c = [window.innerWidth / 2, window.innerHeight - 40];

  const b = addButton("LOGOUT", c, logout);
}

function addRedirectButton(label, url, p) {
  function go() {
    window.location.href = url;
  }
  const b = addButton(label, p, go);
}

function addScrollerProfileMenu() {
  const pageNames = ["account", "settings", "about", "games", "logout"];
  const fs = [() => (window.location.href = "/cgi/auth/securePage/account.php"), () => (window.location.href = "/cgi/auth/securePage/settings.php"), () => (window.location.href = "/cgi/auth/securePage/about.php"), () => (window.location.href = "/cgi/auth/securePage/games.php"), () => logout()];
  let pi = pageNames.indexOf(window.PAGE_NAME);
  if (pi != -1) {
    pageNames.splice(pi, 1);
    fs.splice(pi, 1);
  }
  addScrollerMenu("menu", [window.innerWidth - 60, 40], pageNames, fs);
}

function applyBackground(flag = window.DARKMODE === 1) {
  document.documentElement.style.setProperty("background-color", flag ? "rgba(34, 34, 34, 1)" : "", "important");
}
