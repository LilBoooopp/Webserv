<?php
require_once __DIR__ . "/../storage.php";

header("Content-Type: application/json");
header("Connection: close");

$session_dir = realpath(__DIR__ . '/../../../../sessions');
if ($session_dir) {
	session_save_path($session_dir);
}

$send_json = function (array $payload) {
	$json = json_encode($payload);
	header("Content-Length: " . strlen($json));
	echo $json;
	exit();
};

$user = $_SERVER["HTTP_USERNAME"] ?? null;
if (!$user) {
	$send_json(["success" => false, "error" => "USERNAME MISSING"]);
}

$pass = $_SERVER["HTTP_PASSWORD"] ?? null;
if (!$pass) {
	$send_json(["success" => false, "error" => "PASSWORD MISSING"]);
}

$hash = password_hash($pass, PASSWORD_DEFAULT);
$ctx = storage_open();

if (storage_get_user($ctx, $user)) {
	$send_json(["success" => false, "error" => "USERNAME TAKEN"]);
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
	$send_json(["success" => false, "error" => "ERROR"]);
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

session_write_close();

$send_json(["success" => true, "user" => $user]);
