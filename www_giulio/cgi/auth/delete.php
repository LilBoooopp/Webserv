<?php
session_start();

if (empty($_SESSION["user_id"])) {
    header('Content-Type: text/plain');
    http_response_code(401);
    echo "NOT_LOGGED_IN\n";
    exit;
}

$user = $_SESSION["user_id"];

try {
    $db = new PDO("sqlite:auth.db");
    $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    $stmt = $db->prepare("DELETE FROM users WHERE username = ?");
    $stmt->execute([$user]);

    if ($stmt->rowCount() === 0) {
        header('Content-Type: text/plain');
        http_response_code(404);
        echo "USER_NOT_FOUND\n";
        exit;
    }

    // ✅ Détruire la session après suppression
    $_SESSION = [];

    if (ini_get("session.use_cookies")) {
        $params = session_get_cookie_params();
        setcookie(session_name(), '', time() - 42000,
            $params["path"],
            $params["domain"],
            $params["secure"],
            $params["httponly"]
        );
    }

    session_destroy();

    header('Content-Type: text/plain');
    http_response_code(200);
    echo "OK\n";

} catch (PDOException $e) {
    header('Content-Type: text/plain');
    http_response_code(500);
    echo "DB_ERROR\n";
}
