<?php
session_start();
header('Content-Type: application/json');
require_once __DIR__ . '/../config/config.php';

// Quitar en producción
error_reporting(E_ALL);
ini_set('display_errors', 1);

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['ok' => false, 'error' => 'Método no permitido']));
}

$input = json_decode(file_get_contents('php://input'), true);
$user  = trim($input['memb___id'] ?? '');
$pass  = $input['memb__pwd'] ?? '';
$name  = trim($input['memb_name'] ?? '');
$email = trim($input['mail_addr'] ?? '');
$captchaToken = $input['captcha_token'] ?? '';
$captchaResp  = intval($input['captcha_resp'] ?? 0);

if (strlen($user) < 3 || strlen($user) > 10) {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'El ID debe tener entre 3 y 10 caracteres.']));
}
if (strlen($pass) < 6) {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'La contraseña debe tener al menos 6 caracteres.']));
}
if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'El correo electrónico no es válido.']));
}
if ($name === '') {
    $name = $user;
}
if (strlen($name) > 10) {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'El nombre debe tener hasta 10 caracteres.']));
}

if (!isset($_SESSION['captcha_answer']) || $captchaResp !== $_SESSION['captcha_answer']) {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'Respuesta de CAPTCHA incorrecta.']));
}
unset($_SESSION['captcha_answer']);

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME;
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    // Verificar ID y email
    $stmt = $pdo->prepare("SELECT COUNT(*) FROM MEMB_INFO WHERE memb___id = :id");
    $stmt->execute([':id' => $user]);
    if ($stmt->fetchColumn() > 0) {
        http_response_code(409);
        exit(json_encode(['ok' => false, 'error' => 'El ID ya está en uso.']));
    }
    $stmt = $pdo->prepare("SELECT COUNT(*) FROM MEMB_INFO WHERE mail_addr = :mail");
    $stmt->execute([':mail' => $email]);
    if ($stmt->fetchColumn() > 0) {
        http_response_code(409);
        exit(json_encode(['ok' => false, 'error' => 'El correo ya está registrado.']));
    }

    // Guardar contraseña en texto plano (sin hash)
    $stmt = $pdo->prepare("
        INSERT INTO MEMB_INFO 
            (memb___id, memb__pwd, memb_name, sno__numb, bloc_code, ctl1_code, mail_addr)
        VALUES 
            (:id, :pwd, :name, :sno, :bloc, :ctl, :mail)
    ");
    $stmt->execute([
        ':id'   => $user,
        ':pwd'  => $pass,     // <-- texto plano
        ':name' => $name,
        ':sno'  => '111111111111111111',
        ':bloc' => '0',
        ':ctl'  => '0',
        ':mail' => $email
    ]);

    http_response_code(201);
    echo json_encode(['ok' => true, 'message' => 'Cuenta creada exitosamente.']);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'Error real: ' . $e->getMessage()]);
}