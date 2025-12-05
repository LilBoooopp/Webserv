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
      const msg = await res.text();

      if (res.ok && msg.trim() === "OK") {
        announce(action === "login" ? "Welcome back!" : "Account created!");
        if (action === "login") window.location.href = "/cgi/auth/securePage/account.php";
      } else {
        console.warn("Auth failed:", res.status, msg);
        announce(msg || "Authentication failed");
      }
    })
    .catch((err) => {
      console.warn("Network/server error:", err);
      announce("Server error");
    });
}

function addLogoutButton() {
  const c = [window.innerWidth / 2, window.innerHeight - 40];
  const url = "/cgi/auth/logout.php";

  function logout() {
    fetch(url, {
      method: "POST",
      credentials: "include",
    })
      .then(async (res) => {
        const msg = await res.text();
        if (res.ok && msg.trim() === "OK") {
          window.location.href = "/main/login.html";
        } else {
          announce("Logout failed");
        }
      })
      .catch(() => announce("Server error"));
  }
  const b = addButton("LOGOUT", c, logout);
}

function addRedirectButton(label, url, p) {
  function go() {
    window.location.href = url;
  }
  const b = addButton(label, p, go);
}
