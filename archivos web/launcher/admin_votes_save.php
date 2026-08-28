<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../config/config.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['ok' => false, 'error' => 'Método no permitido']));
}

$input = json_decode(file_get_contents('php://input'), true);
$user = trim($input['username'] ?? '');
$sites = $input['sites'] ?? [];

if (!defined('ADMIN_USER') || $user !== ADMIN_USER) {
    http_response_code(403);
    exit(json_encode(['ok' => false, 'error' => 'Acceso denegado.']));
}

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME . ";charset=UTF-8";
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    $pdo->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);

    $pdo->exec("DELETE FROM VOTES_SITES");

    $stmt = $pdo->prepare("INSERT INTO VOTES_SITES (votesite_title, votesite_link, votesite_reward, votesite_time, reward_type) VALUES (:title, :link, :reward, :time, :type)");
    foreach ($sites as $site) {
        $stmt->execute([
            ':title'  => trim($site['votesite_title'] ?? ''),
            ':link'   => trim($site['votesite_link'] ?? ''),
            ':reward' => intval($site['votesite_reward'] ?? 0),
            ':time'   => intval($site['votesite_time'] ?? 12),
            ':type'   => strtoupper(trim($site['reward_type'] ?? 'WC'))
        ]);
    }

    echo json_encode(['ok' => true, 'message' => 'Sitios guardados.']);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'Error de base de datos: ' . $e->getMessage()]);
}