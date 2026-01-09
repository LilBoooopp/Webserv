<?php
// Return JSON list of files in a given upload directory
// Only allow listing within /ressources/uploads/

header("Content-Type: application/json");

// Get directory path from query parameter
$dir = isset($_GET["dir"]) ? $_GET["dir"] : "";

// Normalize and validate path
$dir = trim($dir, "/");
if ($dir === "" || $dir === ".") {
	$dir = "";
}

// Prevent directory traversal
if (strpos($dir, "..") !== false || strpos($dir, "/..") !== false) {
	http_response_code(403);
	echo json_encode(["error" => "Access denied"]);
	exit();
}

// Construct full path
$uploadsBase = realpath(__DIR__ . "/../ressources/uploads");
if (!$uploadsBase) {
	http_response_code(500);
	echo json_encode(["error" => "Uploads directory not found"]);
	exit();
}

$fullPath = $uploadsBase . ($dir ? "/" . $dir : "");
$fullPath = realpath($fullPath);

// Verify path is within uploads directory
if (!$fullPath || strpos($fullPath, $uploadsBase) !== 0) {
	http_response_code(403);
	echo json_encode(["error" => "Access denied"]);
	exit();
}

// Check if path exists and is a directory
if (!is_dir($fullPath)) {
	http_response_code(404);
	echo json_encode(["error" => "Directory not found"]);
	exit();
}

// List files and directories
$files = [];
$entries = scandir($fullPath);

if ($entries === false) {
	http_response_code(500);
	echo json_encode(["error" => "Failed to read directory"]);
	exit();
}

foreach ($entries as $entry) {
	if ($entry === "." || $entry === "..") {
		continue;
	}

	$entryPath = $fullPath . "/" . $entry;
	$isDir = is_dir($entryPath);
	$size = $isDir ? 0 : filesize($entryPath);
	$relativePath = $dir ? $dir . "/" . $entry : $entry;

	$files[] = [
		"name" => $entry,
		"relativePath" => $relativePath,
		"size" => $size,
		"isDir" => $isDir,
		"extension" => $isDir ? "folder" : pathinfo($entry, PATHINFO_EXTENSION),
	];
}

// Sort: directories first, then alphabetically
usort($files, function ($a, $b) {
	if ($a["isDir"] !== $b["isDir"]) {
		return $b["isDir"] ? 1 : -1;
	}
	return strcasecmp($a["name"], $b["name"]);
});

http_response_code(200);
echo json_encode($files);
?>
