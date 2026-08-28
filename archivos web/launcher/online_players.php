<?php
header('Content-Type: application/json');
require_once __DIR__ . '/../config/config.php';

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME;
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    // Contar jugadores con ConnectStat = 1 (online)
    $stmt = $pdo->query("SELECT COUNT(*) FROM MEMB_STAT WHERE ConnectStat = 1");
    $online = (int) $stmt->fetchColumn();

    echo json_encode(['online' => $online]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['online' => 0, 'error' => 'Error de base de datos']);
}