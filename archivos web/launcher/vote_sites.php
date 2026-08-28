<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../config/config.php';

$accountId = trim($_GET['accountId'] ?? '');

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME . ";charset=UTF-8";
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    $pdo->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);

    // Obtener todos los sitios de voto
    $stmt = $pdo->query("SELECT votesite_id, votesite_title, votesite_link, votesite_reward, votesite_time, reward_type FROM VOTES_SITES ORDER BY votesite_id ASC");
    $sites = $stmt->fetchAll(PDO::FETCH_ASSOC);

    // Si hay un usuario logueado, calcular tiempo restante para cada sitio
    if ($accountId !== '') {
        foreach ($sites as &$site) {
            $timeLimit = date('Y-m-d H:i:s', time() - ($site['votesite_time'] * 3600));
            $stmt = $pdo->prepare("SELECT MAX(timestamp) FROM VOTES_LOG WHERE user_id = :uid AND votesite_id = :sid AND timestamp > :tlimit");
            $stmt->execute([':uid' => $accountId, ':sid' => $site['votesite_id'], ':tlimit' => $timeLimit]);
            $lastClaim = $stmt->fetchColumn();
            if ($lastClaim) {
                // Calcular segundos restantes
                $lastTimestamp = strtotime($lastClaim);
                $nextAvailable = $lastTimestamp + ($site['votesite_time'] * 3600);
                $remaining = $nextAvailable - time();
                $site['remaining_seconds'] = max(0, $remaining);
            } else {
                $site['remaining_seconds'] = 0; // Puede reclamar inmediatamente
            }
        }
    }

    echo json_encode(['ok' => true, 'sites' => $sites]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => $e->getMessage()]);
}