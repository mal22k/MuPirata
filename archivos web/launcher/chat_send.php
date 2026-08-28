<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../config/config.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['ok' => false, 'error' => 'Método no permitido']));
}

$input = json_decode(file_get_contents('php://input'), true);
$username = trim($input['username'] ?? '');
$message = trim($input['message'] ?? '');

if ($username === '' || $message === '') {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'Usuario y mensaje requeridos.']));
}

// Limitar longitud del mensaje
if (strlen($message) > 500) {
    $message = substr($message, 0, 500);
}

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME . ";charset=UTF-8";
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    $stmt = $pdo->prepare("INSERT INTO CHAT_MESSAGES (username, message) VALUES (:username, :message)");
    $stmt->execute([':username' => $username, ':message' => $message]);

    echo json_encode(['ok' => true]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'Error al guardar mensaje.']);
}