<?php
header('Content-Type: application/json');
require_once __DIR__ . '/../config/config.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['ok' => false, 'error' => 'Método no permitido']));
}

$input = json_decode(file_get_contents('php://input'), true);
$user = trim($input['username'] ?? '');

if ($user === '') {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'Usuario requerido.']));
}

// Verificar si el usuario es el administrador definido en config.php
$esAdmin = (defined('ADMIN_USER') && $user === ADMIN_USER);

echo json_encode(['ok' => true, 'admin' => $esAdmin]);