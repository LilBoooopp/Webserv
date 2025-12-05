<?php

$user = $_SERVER['HTTP_USERNAME'] ?? null;
$pass = $_SERVER['HTTP_PASSWORD'] ?? null;

if (!$user) {
	echo "USERNAME MISSING\n";
	exit;
}
if (!$pass) {
	echo "PASSWORD MISSING\n";
	exit;
}

$hash = password_hash($pass, PASSWORD_DEFAULT);

$db = new PDO("sqlite:auth.db");
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

$db->exec("CREATE TABLE IF NOT EXISTS users (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	username TEXT UNIQUE,
	password TEXT
)");

try {
	$stmt = $db->prepare("INSERT INTO users (username, password) VALUES (?, ?)");
	$stmt->execute([$user, $hash]);
	echo "OK\n";
} catch (PDOException $e) {
	if ($e->getCode() === "23000") {
		echo "USERNAME_TAKEN\n";
	} else {
		echo "ERROR\n";
	}
}
