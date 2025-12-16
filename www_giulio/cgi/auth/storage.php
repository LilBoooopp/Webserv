<?php
// Simple storage wrapper: try SQLite, fallback to JSON file

function storage_open($forceFile = false) {
    $baseDir = __DIR__;

    // Allow forcing JSON mode even when SQLite is available (e.g., testing)
    $envForce = getenv('AUTH_FORCE_FILE');
    $forceEnv = ($envForce !== false) && (strtolower($envForce) === '1' || strtolower($envForce) === 'true');
    if ($forceFile || $forceEnv) {
        $file = $baseDir . '/auth_store.json';
        return array('mode' => 'file', 'file' => $file, 'forced' => true);
    }

    $dbPath  = $baseDir . '/auth.db';
    try {
        $db = new PDO('sqlite:' . $dbPath);
        $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        storage_ensure_sqlite_schema($db);
        return array('mode' => 'sqlite', 'db' => $db);
    } catch (Exception $e) {
        $file = $baseDir . '/auth_store.json';
        return array('mode' => 'file', 'file' => $file, 'error' => $e->getMessage());
    }
}

function storage_ensure_sqlite_schema($db) {
    $db->exec("CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE,
        password TEXT,
        name TEXT,
        tel TEXT,
        email TEXT,
        secret TEXT,
        darkmode INTEGER DEFAULT 0,
        snakeHighScore INTEGER DEFAULT 0,
        minesweeperMaxLevel INTEGER DEFAULT 1
    )");
    $alterStatements = array(
        "ALTER TABLE users ADD COLUMN name TEXT",
        "ALTER TABLE users ADD COLUMN tel TEXT",
        "ALTER TABLE users ADD COLUMN email TEXT",
        "ALTER TABLE users ADD COLUMN secret TEXT",
        "ALTER TABLE users ADD COLUMN darkmode INTEGER DEFAULT 0",
        "ALTER TABLE users ADD COLUMN snakeHighScore INTEGER DEFAULT 0",
        "ALTER TABLE users ADD COLUMN minesweeperMaxLevel INTEGER DEFAULT 1"
    );
    foreach ($alterStatements as $sql) {
        try {
            $db->exec($sql);
        } catch (Exception $ignored) {
        }
    }
}

function storage_file_read_all($file) {
    if (!file_exists($file)) return array();
    $data = file_get_contents($file);
    if ($data === false || $data === '') return array();
    $arr = json_decode($data, true);
    return is_array($arr) ? $arr : array();
}

function storage_file_write_all($file, $arr) {
    $dir = dirname($file);
    if (!is_dir($dir)) {
        mkdir($dir, 0770, true);
    }
    $fp = fopen($file, 'c+');
    if (!$fp) return false;
    if (!flock($fp, LOCK_EX)) {
        fclose($fp);
        return false;
    }
    ftruncate($fp, 0);
    rewind($fp);
    $ok = fwrite($fp, json_encode($arr)) !== false;
    fflush($fp);
    flock($fp, LOCK_UN);
    fclose($fp);
    @chmod($file, 0600);
    return $ok;
}

function storage_get_user($ctx, $username) {
    if ($ctx['mode'] === 'sqlite') {
        $stmt = $ctx['db']->prepare('SELECT * FROM users WHERE username = ?');
        $stmt->execute(array($username));
        $row = $stmt->fetch(PDO::FETCH_ASSOC);
        return $row ? $row : null;
    }
    $all = storage_file_read_all($ctx['file']);
    foreach ($all as $u) {
        if (isset($u['username']) && $u['username'] === $username) return $u;
    }
    return null;
}

function storage_insert_user($ctx, $user) {
    if ($ctx['mode'] === 'sqlite') {
        $stmt = $ctx['db']->prepare('INSERT INTO users (username, password, name, tel, email, secret, darkmode, snakeHighScore, minesweeperMaxLevel) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)');
        return $stmt->execute(array(
            $user['username'],
            $user['password'],
            $user['name'] ?? '',
            $user['tel'] ?? '',
            $user['email'] ?? '',
            $user['secret'] ?? '',
            $user['darkmode'] ?? 0,
            $user['snakeHighScore'] ?? 0,
            $user['minesweeperMaxLevel'] ?? 1,
        ));
    }
    $all = storage_file_read_all($ctx['file']);
    foreach ($all as $u) {
        if ($u['username'] === $user['username']) {
            return false; // duplicate
        }
    }
    $all[] = $user;
    return storage_file_write_all($ctx['file'], $all);
}

function storage_update_user($ctx, $user) {
    if ($ctx['mode'] === 'sqlite') {
        $stmt = $ctx['db']->prepare('UPDATE users SET password = ?, name = ?, tel = ?, email = ?, secret = ?, darkmode = ?, snakeHighScore = ?, minesweeperMaxLevel = ? WHERE username = ?');
        return $stmt->execute(array(
            $user['password'] ?? '',
            $user['name'] ?? '',
            $user['tel'] ?? '',
            $user['email'] ?? '',
            $user['secret'] ?? '',
            $user['darkmode'] ?? 0,
            $user['snakeHighScore'] ?? 0,
            $user['minesweeperMaxLevel'] ?? 1,
            $user['username'],
        ));
    }
    $all = storage_file_read_all($ctx['file']);
    $found = false;
    foreach ($all as $i => $u) {
        if ($u['username'] === $user['username']) {
            $all[$i] = array_merge($u, $user);
            $found = true;
            break;
        }
    }
    if (!$found) $all[] = $user;
    return storage_file_write_all($ctx['file'], $all);
}

function storage_delete_user($ctx, $username) {
    if ($ctx['mode'] === 'sqlite') {
        $stmt = $ctx['db']->prepare('DELETE FROM users WHERE username = ?');
        $stmt->execute(array($username));
        return $stmt->rowCount();
    }
    $all = storage_file_read_all($ctx['file']);
    $new = array();
    $deleted = 0;
    foreach ($all as $u) {
        if ($u['username'] === $username) {
            $deleted++;
            continue;
        }
        $new[] = $u;
    }
    storage_file_write_all($ctx['file'], $new);
    return $deleted;
}

?>
