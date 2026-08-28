<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../config/config.php';

// Parámetro opcional: last_id (para obtener solo mensajes nuevos)
$lastId = isset($_GET['last_id']) ? intval($_GET['last_id']) : 0;

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME . ";charset=UTF-8";
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    if ($lastId > 0) {
        $stmt = $pdo->prepare("SELECT id, username, message, timestamp FROM CHAT_MESSAGES WHERE id > :last_id ORDER BY id ASC");
        $stmt->execute([':last_id' => $lastId]);
    } else {
        // Solo los últimos 100 mensajes (carga inicial)
        $stmt = $pdo->query("SELECT TOP 50 id, username, message, timestamp FROM CHAT_MESSAGES ORDER BY id DESC");
        $rows = $stmt->fetchAll(PDO::FETCH_ASSOC);
        $rows = array_reverse($rows); // Orden cronológico
        echo json_encode(['messages' => $rows]);
        exit;
    }

    $rows = $stmt->fetchAll(PDO::FETCH_ASSOC);
    echo json_encode(['messages' => $rows]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['messages' => [], 'error' => 'Error al leer mensajes.']);
}