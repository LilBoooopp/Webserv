<?php
require_once __DIR__ . "/../storage.php";
session_start();

if (empty($_SESSION["user_id"])) {
	header("Content-Type: text/plain");
	header("Status: 401 Unauthorized");
	http_response_code(401);
	echo "NOT_LOGGED_IN\n";
	exit();
}

$user = $_SESSION["user_id"];
$ctx = storage_open();
$deleted = storage_delete_user($ctx, $user);

if ($deleted === 0) {
	header("Content-Type: text/plain");
	header("Status: 404 Not Found");
	http_response_code(404);
	echo "USER_NOT_FOUND\n";
	exit();
}

$_SESSION = [];
if (ini_get("session.use_cookies")) {
	$params = session_get_cookie_params();
	setcookie(session_name(), "", time() - 42000, $params["path"], $params["domain"], $params["secure"], $params["httponly"]);
}
session_destroy();

header("Content-Type: text/plain");
http_response_code(200);
echo "OK\n";
