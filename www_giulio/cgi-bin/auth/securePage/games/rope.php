<?php
session_start();

if (empty($_SESSION["user_id"])) {
	header("Location: /login.html");
	exit;
}

$user = $_SESSION["username"] ?? null;
$darkmode = $_SESSION["darkmode"] ?? false;
$minesweeperMaxLevel = $_SESSION["minesweeperMaxLevel"] ?? 1;
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
    <script>
		window.CURRENT_USER = <?php echo json_encode($user); ?>;
		window.DARKMODE = <?php echo json_encode($darkmode); ?>;
		window.PAGE_NAME = "rope";
	</script>

    <script src="/js/div.js"></script>
    <script src="/js/buttons.js"></script>
    <script src="/js/auth.js"></script>
    <script src="/js/inputField.js"></script>
    <script src="/js/audio.js"></script>
	<script src="/js/color.js"></script>
    <script src="/js/games/vec2.js"></script>
    <script src="/js/games/canvas.js"></script>
    <script src="/js/games/input.js"></script>
    <script src="/js/games/rope/rope.js"></script>
    <script src="/js/games/rope/main.js"></script>
    <script src="/js/games/rope/shapes.js"></script>
    <script src="/js/games/rope/utils.js"></script>
    <script src="/js/games/rope/presets.js"></script>

    <script>
		initRopeSimulation();
		addScrollerProfileMenu();
		addDarkModeButton();
		initBackground();
	</script>
  </body>
</html>
