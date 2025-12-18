<?php
require_once __DIR__ . "/../storage.php";
header("Content-Type: application/json");
session_start();

if (empty($_SESSION["user_id"])) {
	echo json_encode(["success" => false, "error" => "UNAUTHORIZED"]);
	exit();
}

$ctx = storage_open();

// Get all users from storage
$users = [];
if ($ctx["mode"] === "sqlite") {
	$stmt = $ctx["db"]->query("SELECT username, name, snakeHighScore FROM users");
	while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
		$users[] = [
			"username" => $row["username"],
			"name" => $row["name"] ?? "",
			"snakeHighScore" => (int) ($row["snakeHighScore"] ?? 0),
		];
	}
} else {
	// File mode
	$all = storage_file_read_all($ctx["file"]);
	foreach ($all as $u) {
		$users[] = [
			"username" => $u["username"] ?? "",
			"name" => $u["name"] ?? "",
			"snakeHighScore" => (int) ($u["snakeHighScore"] ?? 0),
		];
	}
}

header("Content-Type: application/json");
echo json_encode($users);
