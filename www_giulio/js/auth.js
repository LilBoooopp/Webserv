function handleAuth(e) {
  e.preventDefault();
  const form = e.target;
  const btn = e.submitter;
  const action = btn ? btn.value : "login";

  const formData = new FormData(form);
  const username = formData.get("user") || "";
  const password = formData.get("pass") || "";

  const url = "/cgi-bin/auth/sessionManagment/" + (action === "login" ? "login.php" : "register.php");

  fetch(url, {
    method: "POST",
    headers: {
      USERNAME: username,
      PASSWORD: password,
    },
  })
    .then(async (res) => {
      const contentType = res.headers.get("Content-Type") || "";
      const text = await res.text();

      console.log("Response status:", res.status);
      console.log("Response Content-Type:", contentType);
      console.log("Response body:", text.substring(0, 200));

      if (!contentType.includes("application/json")) {
        console.warn("Non-JSON response received");
        announce("Server error: invalid response format");
        return;
      }

      let data;
      try {
        data = JSON.parse(text);
      } catch (e) {
        console.error("JSON parse error:", e);
        announce("Server error: invalid JSON");
        return;
      }

      if (data.success) {
        if (action === "login") window.location.href = "/cgi-bin/auth/securePage/account.php";
        else announce("Account created");
        return;
      }
      announce(data.error || "Authentication failed");
    })
    .catch((err) => {
      console.warn("Network/server error:", err);
      announce("Server error");
    });
}

function logout() {
  const url = "/cgi-bin/auth/sessionManagment/logout.php";
  fetch(url, {
    method: "POST",
    credentials: "include",
  })
    .then(async (res) => {
      const data = await res.json();
      if (data.success) window.location.href = "/login.html";
      else announce(data.error || "Logout failed");
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
  const fs = [
    () => (window.location.href = "/cgi-bin/auth/securePage/account.php"),
    () => (window.location.href = "/cgi-bin/auth/securePage/settings.php"),
    () => (window.location.href = "/cgi-bin/auth/securePage/about.php"),
    () => (window.location.href = "/cgi-bin/auth/securePage/games.php"),
    () => logout(),
  ];
  let pi = pageNames.indexOf(window.PAGE_NAME);
  if (pi != -1) {
    pageNames.splice(pi, 1);
    fs.splice(pi, 1);
  }
  addScrollerMenu("menu", [window.innerWidth - 60, 80], pageNames, fs);
}

function loadUsers(onLoad = null) {
  fetch("/cgi-bin/auth/sessionManagment/getUserList.php")
    .then((response) => response.json())
    .then((data) => {
      const users = data.success ? data.data : [];
      window.USERS = users;
      if (onLoad) onLoad(users);
    })
    .catch((err) => console.error("Failed to fetch users:", err));
}
