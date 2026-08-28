<?php
header('Content-Type: application/json');
require_once __DIR__ . '/../config/config.php';

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME;
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    // Datos del castillo
    $stmt = $pdo->query("SELECT TOP 1 OWNER_GUILD, SIEGE_START_DATE, SIEGE_END_DATE FROM MuCastle_DATA ORDER BY SIEGE_START_DATE DESC");
    $castle = $stmt->fetch(PDO::FETCH_ASSOC);
    $ownerGuild = $castle['OWNER_GUILD'] ?? '';
    $nextSiege  = $castle['SIEGE_START_DATE'] ?? null;
    $siegeEnd   = $castle['SIEGE_END_DATE'] ?? null;

    // Datos de la guild
    $score = 0;
    $guildMaster = '';
    $members = [];
    $logoUrl = '';

    if ($ownerGuild) {
        // Obtener G_Master, G_Score
        $stmt = $pdo->prepare("SELECT G_Score, G_Master FROM Guild WHERE G_Name = :name");
        $stmt->execute([':name' => $ownerGuild]);
        $row = $stmt->fetch(PDO::FETCH_ASSOC);
        if ($row) {
            $score = $row['G_Score'] ?? 0;
            $guildMaster = $row['G_Master'] ?? '';
        }

        // Obtener miembros de la guild (tabla GuildMember – ajusta el nombre si es necesario)
        try {
            $stmt = $pdo->prepare("SELECT Name FROM GuildMember WHERE G_Name = :name ORDER BY CharName");
            $stmt->execute([':name' => $ownerGuild]);
            $members = $stmt->fetchAll(PDO::FETCH_COLUMN);
        } catch (Exception $e) {
            // Si la tabla no existe, devolvemos array vacío
            $members = [];
        }

        $logoUrl = "guild_logo.php?name=" . urlencode($ownerGuild) . "&size=128";
    }

    echo json_encode([
        'ownerGuild'   => $ownerGuild,
        'guildMaster'  => $guildMaster,
        'members'      => $members,
        'score'        => $score,
        'nextSiege'    => $nextSiege ? date('c', strtotime($nextSiege)) : null,
        'siegeEnd'     => $siegeEnd ? date('c', strtotime($siegeEnd)) : null,
        'logo'         => $logoUrl
    ]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['error' => 'Error de base de datos']);
}