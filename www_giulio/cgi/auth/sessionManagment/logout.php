<?php
require_once __DIR__ . "/../storage.php";

header("Content-Type: application/json");

session_start();

if (empty($_SESSION["user_id"])) {
	echo json_encode(["success" => false, "error" => "UNAUTHORIZED"]);
	exit();
}

$ctx = storage_open();
$user = storage_get_user($ctx, $_SESSION["user_id"]);
if (!$user) {
	// Clear any local session even if backing store lost the user
	$_SESSION = [];
	session_destroy();
	echo json_encode(["success" => false, "error" => "USER NOT FOUND"]);
	exit();
}

$_SESSION = [];

if (ini_get("session.use_cookies")) {
	$params = session_get_cookie_params();
	setcookie(session_name(), "", time() - 42000, $params["path"], $params["domain"], $params["secure"], $params["httponly"]);
}

session_destroy();

echo json_encode(["success" => true]);
