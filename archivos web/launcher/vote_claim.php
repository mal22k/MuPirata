<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../config/config.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['ok' => false, 'error' => 'Método no permitido']));
}

$input     = json_decode(file_get_contents('php://input'), true);
$accountId = trim($input['accountId'] ?? '');
$siteId    = intval($input['siteId'] ?? 0);
$userIp    = $_SERVER['REMOTE_ADDR'];

if ($accountId === '' || $siteId === 0) {
    http_response_code(400);
    exit(json_encode(['ok' => false, 'error' => 'Datos incompletos.']));
}

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME . ";charset=UTF-8";
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    $pdo->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);

    // Obtener datos del sitio
    $stmt = $pdo->prepare("SELECT votesite_reward, votesite_time, reward_type FROM VOTES_SITES WHERE votesite_id = :id");
    $stmt->execute([':id' => $siteId]);
    $site = $stmt->fetch(PDO::FETCH_ASSOC);
    if (!$site) {
        http_response_code(404);
        exit(json_encode(['ok' => false, 'error' => 'Sitio no encontrado.']));
    }

    // Verificar tiempo de espera
    $timeLimit = date('Y-m-d H:i:s', time() - ($site['votesite_time'] * 3600));
    $stmt = $pdo->prepare("SELECT COUNT(*) FROM VOTES_LOG WHERE user_id = :uid AND votesite_id = :sid AND timestamp > :tlimit");
    $stmt->execute([':uid' => $accountId, ':sid' => $siteId, ':tlimit' => $timeLimit]);
    if ($stmt->fetchColumn() > 0) {
        http_response_code(409);
        exit(json_encode(['ok' => false, 'error' => 'Ya reclamaste este voto recientemente.']));
    }

    // Verificar si existe voto reciente (o insertarlo automáticamente)
    $stmt = $pdo->prepare("SELECT COUNT(*) FROM VOTES WHERE vote_site_id = :sid AND user_id = :uid AND timestamp > :torder");
    $stmt->execute([':sid' => $siteId, ':uid' => $accountId, ':torder' => $timeLimit]);
    if ($stmt->fetchColumn() == 0) {
        $stmt = $pdo->prepare("INSERT INTO VOTES (user_id, user_ip, vote_site_id, timestamp) VALUES (:uid, :ip, :sid, :ts)");
        $stmt->execute([
            ':uid' => $accountId,
            ':ip'  => $userIp,
            ':sid' => $siteId,
            ':ts'  => date('Y-m-d H:i:s')
        ]);
    }

    // Entregar recompensa según reward_type
    $reward = (int)$site['votesite_reward'];
    $rewardType = strtoupper(trim($site['reward_type'] ?? 'WC'));

    switch ($rewardType) {
        case 'WP':
            $stmt = $pdo->prepare("UPDATE CashShopData SET WCoinP = WCoinP + :reward WHERE AccountID = :acc");
            break;
        case 'GP':
            $stmt = $pdo->prepare("UPDATE CashShopData SET GoblinPoint = GoblinPoint + :reward WHERE AccountID = :acc");
            break;
        default:
            $stmt = $pdo->prepare("UPDATE CashShopData SET WCoinC = WCoinC + :reward WHERE AccountID = :acc");
    }
    $stmt->execute([':reward' => $reward, ':acc' => $accountId]);

    // Registrar en VOTES_LOG
    $stmt = $pdo->prepare("INSERT INTO VOTES_LOG (user_id, votesite_id, timestamp) VALUES (:uid, :sid, :ts)");
    $stmt->execute([':uid' => $accountId, ':sid' => $siteId, ':ts' => date('Y-m-d H:i:s')]);

    echo json_encode(['ok' => true, 'message' => "Recompensa de {$reward} {$rewardType} entregada."]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'Error de base de datos: ' . $e->getMessage()]);
}