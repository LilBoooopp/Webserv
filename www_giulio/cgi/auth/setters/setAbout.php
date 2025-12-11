<?php
require_once __DIR__ . '/../storage.php';
session_start();

if (empty($_SESSION['user_id'])) {
	http_response_code(401);
	header('Content-Type: text/plain');
	echo "UNAUTHORIZED\n";
	exit;
}

$ctx = storage_open();

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

// Persist in storage for future sessions
$user = storage_get_user($ctx, $_SESSION['user_id']);
if (!$user) {
	http_response_code(404);
	header('Content-Type: text/plain');
	echo "USER NOT FOUND\n";
	exit;
}
$user['name'] = $name;
$user['tel'] = $tel;
$user['email'] = $email;
$user['secret'] = $secret;
storage_update_user($ctx, $user);

header('Content-Type: text/plain');
echo "OK\n";
