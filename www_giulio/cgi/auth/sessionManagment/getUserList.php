<?php
require_once __DIR__ . '/../storage.php';
session_start();

if (empty($_SESSION['user_id'])) {
	http_response_code(401);
	header('Content-Type: application/json');
	echo json_encode(array('error' => 'UNAUTHORIZED'));
	exit;
}

$ctx = storage_open();

// Get all users from storage
$users = array();
if ($ctx['mode'] === 'sqlite') {
	$stmt = $ctx['db']->query('SELECT username, name, snakeHighScore FROM users');
	while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
		$users[] = array(
			'username' => $row['username'],
			'name' => $row['name'] ?? '',
			'snakeHighScore' => (int)($row['snakeHighScore'] ?? 0)
		);
	}
} else {
	// File mode
	$all = storage_file_read_all($ctx['file']);
	foreach ($all as $u) {
		$users[] = array(
			'username' => $u['username'] ?? '',
			'name' => $u['name'] ?? '',
			'snakeHighScore' => (int)($u['snakeHighScore'] ?? 0)
		);
	}
}

header('Content-Type: application/json');
echo json_encode($users);

