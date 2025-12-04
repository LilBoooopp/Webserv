
function handleAuth(e) {
  e.preventDefault();

  const form = e.target;

  // Le bouton effectivement cliqué
  const btn = e.submitter;
  const action = btn ? btn.value : "login";

  const url = action === "login" ? "/login" : "/newLogin";

  const formData = new FormData(form);

  fetch(url, {
    method: "POST",
    body: new URLSearchParams(formData),
  })
    .then(async (res) => {
      const msg = await res.text();

      if (res.ok) {
        announce(action === "login" ? "Welcome back!" : "Account created!");
        window.location.href = "/account";
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
