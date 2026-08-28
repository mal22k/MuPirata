<?php
header('Content-Type: application/json');
require_once __DIR__ . '/../config/config.php';

$type = $_GET['type'] ?? 'reset';

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME;
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    switch ($type) {
        case 'reset':
            $sql = "SELECT TOP 10 Name, ResetCount, MasterResetCount, (ResetCount * 100 + MasterResetCount) AS Score, Class FROM Character ORDER BY Score DESC";
            break;
        case 'level':
            $sql = "SELECT TOP 10 Name, cLevel, Class FROM Character ORDER BY cLevel DESC";
            break;
        case 'online':
            $sql = "SELECT TOP 10 m.memb___id, m.OnlineRewardTime1, c.Class FROM MEMB_INFO m LEFT JOIN Character c ON m.memb___id = c.AccountID ORDER BY m.OnlineRewardTime1 DESC";
            break;
        case 'pk':
            $sql = "SELECT TOP 10 Name, PkCount, Class FROM Character ORDER BY PkCount DESC";
            break;
        default:
            http_response_code(400);
            exit(json_encode(['error' => 'Tipo de ranking no válido']));
    }

    $stmt = $pdo->query($sql);
    $rows = $stmt->fetchAll(PDO::FETCH_ASSOC);
    echo json_encode(['ranking' => $rows]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['error' => 'Error de base de datos']);
}