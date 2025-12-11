<?php
require_once __DIR__ . '/../storage.php';
session_start();

if (empty($_SESSION['user_id'])) {
	http_response_code(401);
	header('Content-Type: text/plain');
	echo "UNAUTHORIZED\n";
	exit;
}

$rawBody = file_get_contents('php://input');

// Fallback parse if the webserver does not populate $_POST
if (empty($_POST) && $rawBody !== false && $rawBody !== '') {
	$tmp = array();
	parse_str($rawBody, $tmp);
	$_POST = $tmp; // make it available to the rest of the script
}

$headers = function_exists('getallheaders') ? getallheaders() : array();
if (empty($headers)) {
	foreach ($_SERVER as $key => $value) {
		if (strpos($key, 'HTTP_') === 0) {
			$h               = str_replace(' ', '-', ucwords(strtolower(str_replace('_', ' ', substr($key, 5)))));
			$headers[$h] = $value;
		}
	}
}
$headersLower = array_change_key_case($headers, CASE_LOWER);
$ctx = storage_open();

$snakeHighScore = $_POST['snakeHighScore'] ?? '';
$snakeHighScore = $snakeHighScore !== '' ? $snakeHighScore : ($headersLower['x-snakehighscore'] ?? '');
$snakeHighScore = (int)$snakeHighScore;

// Persist user preference
$user = storage_get_user($ctx, $_SESSION['user_id']);
if (!$user) {
	http_response_code(404);
	header('Content-Type: text/plain');
	echo "USER NOT FOUND\n";
	exit;
}
$user['snakeHighScore'] = $snakeHighScore;
storage_update_user($ctx, $user);

$_SESSION['snakeHighScore'] = $snakeHighScore;

// Ensure session persistence before exiting
session_write_close();

header('Content-Type: text/plain');
echo "OK\n";
