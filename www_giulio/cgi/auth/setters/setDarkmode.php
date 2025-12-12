<?php
require_once __DIR__ . "/../storage.php";
session_start();

if (empty($_SESSION["user_id"])) {
	http_response_code(401);
	header("Status: 401 Unauthorized");
	header("Content-Type: text/plain");
	echo "UNAUTHORIZED\n";
	exit();
}

$rawBody = file_get_contents("php://input");

// Fallback parse if the webserver does not populate $_POST
if (empty($_POST) && $rawBody !== false && $rawBody !== "") {
	$tmp = [];
	parse_str($rawBody, $tmp);
	$_POST = $tmp; // make it available to the rest of the script
}

$headers = function_exists("getallheaders") ? getallheaders() : [];
if (empty($headers)) {
	foreach ($_SERVER as $key => $value) {
		if (strpos($key, "HTTP_") === 0) {
			$h = str_replace(" ", "-", ucwords(strtolower(str_replace("_", " ", substr($key, 5)))));
			$headers[$h] = $value;
		}
	}
}
$headersLower = array_change_key_case($headers, CASE_LOWER);
$ctx = storage_open();

$darkMode = $_POST["darkmode"] ?? "";
$darkMode = $darkMode !== "" ? $darkMode : $headersLower["x-darkmode"] ?? "";
$darkMode = $darkMode === "1" || strtolower($darkMode) === "true" ? 1 : 0;

// Persist user preference
$user = storage_get_user($ctx, $_SESSION["user_id"]);
if (!$user) {
	http_response_code(404);
	header("Status: 404 Not Found");
	header("Content-Type: text/plain");
	echo "USER NOT FOUND\n";
	exit();
}
$user["darkmode"] = $darkMode;
storage_update_user($ctx, $user);

$_SESSION["darkmode"] = $darkMode;

// Ensure session persistence before exiting
session_write_close();

header("Content-Type: text/plain");
echo "OK\n";
