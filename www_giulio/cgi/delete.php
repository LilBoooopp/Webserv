<?php

// Read file path from query parameter
$fileName = isset($_GET['file']) ? $_GET['file'] : null;
error_log("delete.php received file parameter: " . $fileName);

if (!$fileName) {
    http_response_code(400);
    echo json_encode(['error' => 'Missing file parameter']);
    exit;
}

// Security: ensure the file is within the uploads directory
$uploadsDir = realpath(__DIR__ . '/../ressources/uploads');
$fullPath = realpath($uploadsDir . '/' . $fileName);

error_log("uploadsDir: " . $uploadsDir);
error_log("fullPath: " . $fullPath);

if (!$fullPath || strpos($fullPath, $uploadsDir) !== 0) {
    http_response_code(403);
    echo json_encode(['error' => 'Access denied']);
    exit;
}

// Check if file exists
if (!file_exists($fullPath)) {
    http_response_code(404);
    echo json_encode(['error' => 'File not found']);
    exit;
}

// Attempt to delete the file
if (unlink($fullPath)) {
    http_response_code(200);
    echo json_encode(['success' => true, 'message' => 'File deleted']);
} else {
    http_response_code(500);
    echo json_encode(['error' => 'Failed to delete file']);
}
?>
