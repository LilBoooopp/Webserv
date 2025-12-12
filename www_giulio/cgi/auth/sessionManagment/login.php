<?php
require_once __DIR__ . '/../storage.php';

$user = $_SERVER['HTTP_USERNAME'] ?? null;
if (!$user) {
    header('Content-Type: text/plain');
    header('Status: 400 Bad Request');
    http_response_code(400);
    echo "USERNAME MISSING\n";
    exit;
}

$pass = $_SERVER['HTTP_PASSWORD'] ?? null;
if (!$pass) {
    header('Content-Type: text/plain');
    header('Status: 400 Bad Request');
    http_response_code(400);
    echo "PASSWORD MISSING\n";
    exit;
}

$ctx = storage_open();
$row = storage_get_user($ctx, $user);

if (!$row) {
	header('Content-Type: text/plain');
	header('Status: 404 Not Found');
	http_response_code(404);
	echo "USER NOT FOUND\n";
	exit;
}
if (!isset($row['password']) || !password_verify($pass, $row['password'])) {
	header('Content-Type: text/plain');
	header('Status: 401 Unauthorized');
	http_response_code(401);
	echo "WRONG PASSWORD\n";
	exit;
}

session_start();
$_SESSION["user_id"] = $user;
$_SESSION["username"] = $user;
$_SESSION["logged_in"] = true;
$_SESSION["name"] = $row['name'] ?? '';
$_SESSION["tel"] = $row['tel'] ?? '';
$_SESSION["email"] = $row['email'] ?? '';
$_SESSION["secret"] = $row['secret'] ?? '';
$_SESSION["darkmode"] = (int)($row['darkmode'] ?? 0);
$_SESSION["snakeHighScore"] = (int)($row['snakeHighScore'] ?? 0);

header('Content-Type: text/plain');
echo "OK\n";