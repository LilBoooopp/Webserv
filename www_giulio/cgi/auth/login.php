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

try {
	$db = new PDO("sqlite:auth.db");
	$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

	$stmt = $db->prepare("SELECT password FROM users WHERE username = ?");
	$stmt->execute([$user]);
	$row = $stmt->fetch(PDO::FETCH_ASSOC);

	if (!$row) {
		header('Content-Type: text/plain');
		http_response_code(404);
		echo "USER NOT FOUND\n";
		exit;
	}
	if (!password_verify($pass, $row['password'])){
		header('Content-Type: text/plain');
		http_response_code(401);
		echo "WRONG PASSWORD\n";
		exit;
	}
	session_start();
	$_SESSION["user_id"] = $user;
	$_SESSION["username"] = $user;
	$_SESSION["logged_in"] = true;
	echo "OK\n";

} catch (PDOException $e) {
	echo "DB ERROR: " . $e->getMessage() . "\n";
	exit;
}