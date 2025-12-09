<?php
session_start();

if (empty($_SESSION["user_id"])) {
	header("Location: /login.html");
	exit;
}

$user = $_SESSION["username"] ?? null;
$darkmode = $_SESSION["darkmode"] ?? false;
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
    <script src="/js/snake.js"></script>
    <script src="/js/inputField.js"></script>

    <script>
		const c = [window.innerWidth / 2, window.innerHeight / 2];
		window.CURRENT_USER = <?php echo json_encode($user); ?>;
		window.DARKMODE = <?php echo json_encode($darkmode); ?>;
		window.PAGE_NAME = "Games";

		addTitle();
		applyBackground();
		addButton("Snake", [c[0], c[1]], () => {window.location.href = "/cgi/auth/securePage/snake.php"});
		addScrollerProfileMenu();
    </script>
  </body>
</html>

