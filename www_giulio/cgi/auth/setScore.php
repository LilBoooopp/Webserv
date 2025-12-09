<?php
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

// Ensure columns exist before writing
$db = new PDO("sqlite:" . __DIR__ . '/auth.db');
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
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
		// ignore if column exists
	}
}

$snakeHighScore = $_POST['snakeHighScore'] ?? '';
$snakeHighScore = $snakeHighScore !== '' ? $snakeHighScore : ($headersLower['x-snakehighscore'] ?? '');
$snakeHighScore = (int)$snakeHighScore;

// Persist user preference
$stmt = $db->prepare("UPDATE users SET snakeHighScore = ? WHERE username = ?");
$stmt->execute(array($snakeHighScore, $_SESSION['user_id']));

$_SESSION['snakeHighScore'] = $snakeHighScore;

// Ensure session persistence before exiting
session_write_close();

header('Content-Type: text/plain');
echo "OK\n";
