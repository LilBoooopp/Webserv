<?php
require_once __DIR__ . "/../storage.php";

header("Content-Type: application/json");

$user = $_SERVER["HTTP_USERNAME"] ?? null;
if (!$user) {
	echo json_encode(["success" => false, "error" => "USERNAME MISSING"]);
	exit();
}

$pass = $_SERVER["HTTP_PASSWORD"] ?? null;
if (!$pass) {
	echo json_encode(["success" => false, "error" => "PASSWORD MISSING"]);
	exit();
}

$hash = password_hash($pass, PASSWORD_DEFAULT);
$ctx = storage_open();

if (storage_get_user($ctx, $user)) {
	echo json_encode(["success" => false, "error" => "USERNAME TAKEN"]);
	exit();
}

$ok = storage_insert_user($ctx, [
	"username" => $user,
	"password" => $hash,
	"name" => "",
	"tel" => "",
	"email" => "",
	"secret" => "",
	"darkmode" => 0,
	"snakeHighScore" => 0,
	"minesweeperMaxLevel" => 1,
]);

if (!$ok) {
	echo json_encode(["success" => false, "error" => "ERROR"]);
	exit();
}

session_start();
$_SESSION["user_id"] = $user;
$_SESSION["username"] = $user;
$_SESSION["darkmode"] = 0;
$_SESSION["snakeHighScore"] = 0;
$_SESSION["minesweeperMaxLevel"] = 1;
$_SESSION["name"] = "";
$_SESSION["tel"] = "";
$_SESSION["email"] = "";
$_SESSION["secret"] = "";

echo json_encode(["success" => true, "user" => $user]);
