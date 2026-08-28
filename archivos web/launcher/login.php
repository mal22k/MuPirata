<?php
session_start();
header('Content-Type: application/json');
require_once __DIR__ . '/../config/config.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['ok' => false, 'error' => 'Método no permitido']));
}

$input = json_decode(file_get_contents('php://input'), true);
$user = trim($input['username'] ?? '');
$pass = trim($input['password'] ?? '');

if ($user === '' || $pass === '') {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'Usuario y contraseña son obligatorios.']));
}

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME;
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    // Buscar ignorando mayúsculas/minúsculas
    $stmt = $pdo->prepare("SELECT memb__pwd, memb_name, bloc_code FROM MEMB_INFO WHERE LOWER(memb___id) = LOWER(:id)");
    $stmt->execute([':id' => $user]);
    $row = $stmt->fetch(PDO::FETCH_ASSOC);

    if (!$row) {
        http_response_code(401);
        exit(json_encode(['ok' => false, 'error' => 'Usuario o contraseña incorrectos.']));
    }

    if ($row['bloc_code'] === '1') {
        http_response_code(403);
        exit(json_encode(['ok' => false, 'error' => 'Cuenta bloqueada.']));
    }

    // Comparar ignorando mayúsculas/minúsculas
    if (strcasecmp($pass, $row['memb__pwd']) !== 0) {
        http_response_code(401);
        exit(json_encode(['ok' => false, 'error' => 'Usuario o contraseña incorrectos.']));
    }

    $_SESSION['user_id'] = $user;
    $_SESSION['user_name'] = $row['memb_name'];

    echo json_encode([
        'ok' => true,
        'message' => 'Inicio de sesión exitoso.',
        'username' => $user,
        'name' => $row['memb_name']
    ]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'Error de base de datos: ' . $e->getMessage()]);
}