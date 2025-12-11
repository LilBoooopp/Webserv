<?php
require_once __DIR__ . '/../storage.php';

$user = $_SERVER['HTTP_USERNAME'] ?? null;
if (!$user) {
	header('Content-Type: text/plain');
	http_response_code(400);
	echo "USERNAME MISSING\n";
	exit;
}

$pass = $_SERVER['HTTP_PASSWORD'] ?? null;
if (!$pass) {
	header('Content-Type: text/plain');
	http_response_code(400);
	echo "PASSWORD MISSING\n";
	exit;
}

$hash = password_hash($pass, PASSWORD_DEFAULT);
$ctx = storage_open();

if (storage_get_user($ctx, $user)) {
	http_response_code(409);
	header('Content-Type: text/plain');
	echo "USERNAME_TAKEN\n";
	exit;
}

$ok = storage_insert_user($ctx, array(
	'username' => $user,
	'password' => $hash,
	'name' => '',
	'tel' => '',
	'email' => '',
	'secret' => '',
	'darkmode' => 0,
	'snakeHighScore' => 0,
));

if (!$ok) {
	http_response_code(500);
	header('Content-Type: text/plain');
	echo "ERROR\n";
	exit;
}

header('Content-Type: text/plain');
echo "OK\n";
