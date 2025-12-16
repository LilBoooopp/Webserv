<?php
require_once __DIR__ . "/../storage.php";
header("Content-Type: application/json");
session_start();

if (empty($_SESSION["user_id"])) {
	echo json_encode(["success" => false, "error" => "UNAUTHORIZED"]);
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
	echo json_encode(["success" => false, "error" => "USER NOT FOUND"]);
	exit();
}
$user["darkmode"] = $darkMode;
storage_update_user($ctx, $user);

$_SESSION["darkmode"] = $darkMode;

// Ensure session persistence before exiting
session_write_close();

echo json_encode(["success" => true, "darkmode" => $darkMode]);
