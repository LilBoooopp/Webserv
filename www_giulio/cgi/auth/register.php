<?php

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
		http_response_code(409);
    	header('Content-Type: text/plain');
		echo "USERNAME_TAKEN\n";
	} else {
		http_response_code(401);
   		header('Content-Type: text/plain');
		echo "ERROR\n";
	}
}
