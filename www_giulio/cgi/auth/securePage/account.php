<?php
session_start();

if (empty($_SESSION["user_id"])) {
	header("Location: /login.html");
	exit;
}

$user = $_SESSION["username"] ?? null;
$darkmode = $_SESSION["darkmode"] ?? null;
$name = $_SESSION["name"] ?? null;
$tel = $_SESSION["tel"] ?? null;
$email = $_SESSION["email"] ?? null;
$secret = $_SESSION["secret"] ?? null;

echo "<!-- SESSION_DUMP\n";
var_dump($_SESSION);
echo "\nEND_SESSION_DUMP -->";

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
    <script src="/js/inputField.js"></script>

    <script>
		window.CURRENT_USER = <?php echo json_encode($user); ?>;
		window.DARKMODE = <?php echo json_encode($darkmode); ?>;
		var data = {
			"name": <?php echo json_encode($name); ?>,
			"tel": <?php echo json_encode($tel); ?>,
			"email": <?php echo json_encode($email); ?>,
			"secret": <?php echo json_encode($secret); ?>,
		}

    applyBackground();
		window.PAGE_NAME = "account";
      function init() {
        const c = [window.innerWidth / 2, window.innerHeight / 2];
        addDiv(window.CURRENT_USER, [c[0], c[1] - 150], 3);

		const labels = ["name", "email", "secret", "tel"];
		const w = 25;
		for (let i = 0; i < labels.length; i++)
			addDiv(labels[i] + ": " + (data[labels[i]] === null ? "?" : data[labels[i]]), [c[0], c[1] + w * i]);
		addDeleteAccountButton([c[0], window.innerHeight - 40])
		addScrollerProfileMenu();
      }
      init();
    </script>
  </body>
</html>
