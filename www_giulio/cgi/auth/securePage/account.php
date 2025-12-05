<?php
session_start();

if (empty($_SESSION["user_id"])) {
	header("Location: /main/login.html");
	exit;
}

$user = $_SESSION["username"] ?? null;

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

    <script src="/js/auth.js"></script>
    <script src="/js/div.js"></script>
    <script src="/js/buttons.js"></script>

    <script>
		window.CURRENT_USER = <?php echo json_encode($user); ?>;

      function init() {
        const c = [window.innerWidth / 2, window.innerHeight / 2];
		addDiv(window.CURRENT_USER, [window.innerWidth - 60, 40], 1);
        addDiv("ACCOUNT", [c[0], c[1] - 150], 3);
        const names = ["about", "somePage"];
        for (let i = 0; i < names.length; i++) {
          addRedirectButton(names[i].toUpperCase(), "/cgi/auth/securePage/" + names[i] + ".php", [c[0], c[1] - 50 + 50 * i]);
        }
        addLogoutButton();
      }
      init();
    </script>
  </body>
</html>
