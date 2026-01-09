<?php

// Read file path from X-Delete-Path header
$fileName = isset($_SERVER["HTTP_X_DELETE_PATH"]) ? $_SERVER["HTTP_X_DELETE_PATH"] : null;
error_log("delete.php received file path: " . $fileName);

if (!$fileName) {
	http_response_code(400);
	echo json_encode(["error" => "Missing X-Delete-Path header"]);
	exit();
}

// Security: ensure the file is within the uploads directory
$uploadsDir = realpath(__DIR__ . "/../ressources/uploads");

// Normalize incoming path:
// - decode URL-encoded content
// - strip leading slashes
// - if it mistakenly includes "/ressources/uploads" prefix, remove it to make it relative
$requested = urldecode($fileName);
if ($requested !== null) {
	$requested = trim($requested);
}
// If the client sent an absolute-looking path, make it relative to uploads
if (strpos($requested, "/ressources/uploads/") === 0) {
	$requested = substr($requested, strlen("/ressources/uploads/"));
}
$relative = ltrim($requested, "/");

// Build and resolve the final path; realpath returns false if not existing
$fullPath = realpath($uploadsDir . "/" . $relative);

error_log("uploadsDir: " . $uploadsDir);
error_log("fullPath: " . $fullPath);

// Ensure resolved path is within uploadsDir
if (!$fullPath || strncmp($fullPath, $uploadsDir . DIRECTORY_SEPARATOR, strlen($uploadsDir) + 1) !== 0) {
	http_response_code(403);
	echo json_encode(["error" => "Access denied"]);
	exit();
}

// Check if file exists
if (!file_exists($fullPath)) {
	http_response_code(404);
	echo json_encode(["error" => "File not found"]);
	exit();
}

// Attempt to delete the file
if (unlink($fullPath)) {
	http_response_code(200);
	echo json_encode(["success" => true, "message" => "File deleted"]);
} else {
	http_response_code(500);
	echo json_encode(["error" => "Failed to delete file"]);
}
?>
