<?php

$user = $_SERVER['HTTP_USERNAME'] ?? null;
if (!$user) {echo "USERNAME MISSING\n"; exit;}
$pass = $_SERVER['HTTP_PASSWORD'] ?? null;
if (!$pass) {echo "PASSWORD MISSING\n"; exit;}

try {
	$db = new PDO("sqlite:auth.db");
	$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

	$stmt = $db->prepare("SELECT password FROM users WHERE username = ?");
	$stmt->execute([$user]);
	$row = $stmt->fetch(PDO::FETCH_ASSOC);

	if (!$row) {
		echo "USER NOT FOUND\n";
		exit;
	}

	if (password_verify($pass, $row['password'])){
		session_start();
		$_SESSION["user_id"] = $user;
		$_SESSION["username"] = $user;
		$_SESSION["logged_in"] = true;
		echo "OK\n";
	}
	else {
		echo "WRONG PASSWORD\n";
	}

} catch (PDOException $e) {
	echo "DB ERROR: " . $e->getMessage() . "\n";
	exit;
}