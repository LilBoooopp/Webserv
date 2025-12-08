<?php
session_start();

if (empty($_SESSION['user_id'])) {
	http_response_code(401);
	header('Content-Type: text/plain');
	echo "UNAUTHORIZED\n";
	exit;
}

$db = new PDO("sqlite:" . __DIR__ . '/auth.db');
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

// Ensure columns exist
$alterStatements = array(
	"ALTER TABLE users ADD COLUMN name TEXT",
	"ALTER TABLE users ADD COLUMN tel TEXT",
	"ALTER TABLE users ADD COLUMN email TEXT",
	"ALTER TABLE users ADD COLUMN secret TEXT",
	"ALTER TABLE users ADD COLUMN darkmode TEXT DEFAULT '0'"
);
foreach ($alterStatements as $sql) {
	try {
		$db->exec($sql);
	} catch (PDOException $e) {
		// ignore if column exists
	}
}

$rawBody = file_get_contents('php://input');

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

$name   = $_POST['name']   ?? '';
$tel    = $_POST['tel']    ?? '';
$email  = $_POST['email']  ?? '';
$secret = $_POST['secret'] ?? '';

$name   = $name   !== '' ? $name   : ($headersLower['x-name']   ?? '');
$tel    = $tel    !== '' ? $tel    : ($headersLower['x-tel']    ?? '');
$email  = $email  !== '' ? $email  : ($headersLower['x-email']  ?? '');
$secret = $secret !== '' ? $secret : ($headersLower['x-secret'] ?? '');

$_SESSION['name']   = $name;
$_SESSION['tel']    = $tel;
$_SESSION['email']  = $email;
$_SESSION['secret'] = $secret;

// Persist in DB for future sessions
$stmt = $db->prepare("UPDATE users SET name = ?, tel = ?, email = ?, secret = ? WHERE username = ?");
$stmt->execute(array($name, $tel, $email, $secret, $_SESSION['user_id']));

header('Content-Type: text/plain');
echo "OK\n";
