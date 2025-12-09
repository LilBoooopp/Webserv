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

	// Ensure columns exist for profile and darkmode
	$alterStatements = array(
	    "ALTER TABLE users ADD COLUMN name TEXT",
	    "ALTER TABLE users ADD COLUMN tel TEXT",
	    "ALTER TABLE users ADD COLUMN email TEXT",
	    "ALTER TABLE users ADD COLUMN secret TEXT",
	    "ALTER TABLE users ADD COLUMN darkmode INTEGER DEFAULT 0",
	    "ALTER TABLE users ADD COLUMN snakeHighScore INTEGER DEFAULT 0",
	);
	foreach ($alterStatements as $sql) {
	    try {
	        $db->exec($sql);
	    } catch (PDOException $e) {
	        // Ignore if column already exists
	    }
	}

	$stmt = $db->prepare("SELECT password, name, tel, email, secret, darkmode, snakeHighScore FROM users WHERE username = ?");
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
	$_SESSION["name"] = $row['name'] ?? '';
	$_SESSION["tel"] = $row['tel'] ?? '';
	$_SESSION["email"] = $row['email'] ?? '';
	$_SESSION["secret"] = $row['secret'] ?? '';
	$_SESSION["darkmode"] = $row['darkmode'] ?? 0;
	$_SESSION["snakeHighScore"] = $row['snakeHighScore'] ?? 0;
	echo "OK\n";

} catch (PDOException $e) {
	echo "DB ERROR: " . $e->getMessage() . "\n";
	exit;
}