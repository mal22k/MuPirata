<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../config/config.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['ok' => false, 'error' => 'M¨¦todo no permitido']));
}

$input = json_decode(file_get_contents('php://input'), true);
$accountId = trim($input['accountId'] ?? '');

if ($accountId === '') {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'Cuenta no especificada.']));
}

try {
    // Conexi¨®n sin SET NAMES (no soportado en MSSQL v¨ªa dblib)
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME . ";charset=UTF-8";
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    $pdo->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);

    $stmt = $pdo->prepare("SELECT WCoinC, WCoinP, GoblinPoint FROM CashShopData WHERE AccountID = :acc");
    $stmt->execute([':acc' => $accountId]);
    $row = $stmt->fetch(PDO::FETCH_ASSOC);

    if ($row) {
        echo json_encode([
            'ok' => true,
            'wc' => (int)$row['WCoinC'],
            'wp' => (int)$row['WCoinP'],
            'gp' => (int)$row['GoblinPoint']
        ]);
    } else {
        echo json_encode(['ok' => true, 'wc' => 0, 'wp' => 0, 'gp' => 0]);
    }
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'Error de base de datos: ' . $e->getMessage()]);
}