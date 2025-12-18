<?php
require_once __DIR__ . "/../storage.php";
header("Content-Type: application/json");
session_start();

if (empty($_SESSION["user_id"])) {
	echo json_encode(["success" => false, "error" => "NOT_LOGGED_IN"]);
	exit();
}

$user = $_SESSION["user_id"];
$ctx = storage_open();
$deleted = storage_delete_user($ctx, $user);

if ($deleted === 0) {
	echo json_encode(["success" => false, "error" => "USER_NOT_FOUND"]);
	exit();
}

$_SESSION = [];
if (ini_get("session.use_cookies")) {
	$params = session_get_cookie_params();
	setcookie(session_name(), "", time() - 42000, $params["path"], $params["domain"], $params["secure"], $params["httponly"]);
}
session_destroy();

echo json_encode(["success" => true]);
