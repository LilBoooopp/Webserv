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

$ctx = storage_open();
$row = storage_get_user($ctx, $user);

if (!$row) {
	echo json_encode(["success" => false, "error" => "USER NOT FOUND"]);
	exit();
}

if (!isset($row["password"]) || !password_verify($pass, $row["password"])) {
	echo json_encode(["success" => false, "error" => "WRONG PASSWORD"]);
	exit();
}

session_start();
$_SESSION["user_id"] = $user;
$_SESSION["username"] = $row["username"] ?? $user;
$_SESSION["darkmode"] = $row["darkmode"] ?? 0;
$_SESSION["snakeHighScore"] = $row["snakeHighScore"] ?? 0;
$_SESSION["minesweeperMaxLevel"] = $row["minesweeperMaxLevel"] ?? 1;
$_SESSION["name"] = $row["name"] ?? "";
$_SESSION["tel"] = $row["tel"] ?? "";
$_SESSION["email"] = $row["email"] ?? "";
$_SESSION["secret"] = $row["secret"] ?? "";

echo json_encode([
	"success" => true,
	"user" => [
		"username" => $user,
		"darkmode" => $_SESSION["darkmode"],
		"snakeHighScore" => $_SESSION["snakeHighScore"],
		"minesweeperMaxLevel" => $_SESSION["minesweeperMaxLevel"],
	],
]);
