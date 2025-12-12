<?php
require_once __DIR__ . "/../storage.php";

session_start();

if (empty($_SESSION["user_id"])) {
	http_response_code(401);
	header("Status: 401 Unauthorized");
	header("Content-Type: text/plain");
	echo "UNAUTHORIZED\n";
	exit();
}

$ctx = storage_open();
$user = storage_get_user($ctx, $_SESSION["user_id"]);
if (!$user) {
	// Clear any local session even if backing store lost the user
	$_SESSION = [];
	session_destroy();
	http_response_code(404);
	header("Status: 404 Not Found");
	header("Content-Type: text/plain");
	echo "USER NOT FOUND\n";
	exit();
}

$_SESSION = [];

if (ini_get("session.use_cookies")) {
	$params = session_get_cookie_params();
	setcookie(session_name(), "", time() - 42000, $params["path"], $params["domain"], $params["secure"], $params["httponly"]);
}

session_destroy();

header("Content-Type: text/plain");
echo "OK\n";
