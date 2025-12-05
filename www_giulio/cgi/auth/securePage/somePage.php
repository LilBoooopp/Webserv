<?php
session_start();

if (empty($_SESSION["user_id"])) {
	header("Location: /main/login.html");
	exit;
}
?>

<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
    <link rel="stylesheet" href="/css/style.css" />
  </head>

  <body>
    <div id="ui-root"></div>

    <script src="/js/div.js"></script>
    <script src="/js/buttons.js"></script>
    <script src="/js/auth.js"></script>

    <script>
      function init() {
        const c = [window.innerWidth / 2, window.innerHeight / 2];
        addDiv("SOME PAGE", [c[0], c[1] - 150], 3);
		const names = ["about", "account"];
        for (let i = 0; i < names.length; i++) {
          addRedirectButton(names[i].toUpperCase(), "/cgi/auth/securePage/" + names[i] + ".php", [c[0], c[1] - 50 + 50 * i]);
        }
        addLogoutButton();
      }
      init();
    </script>
  </body>
</html>
