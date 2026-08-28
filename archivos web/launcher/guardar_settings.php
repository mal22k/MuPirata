<?php
header('Content-Type: application/json');
require_once __DIR__ . '/../config/config.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['ok' => false, 'error' => 'M¨¦todo no permitido']));
}

$input = json_decode(file_get_contents('php://input'), true);
$user = trim($input['username'] ?? '');

// Verificar que sea el administrador
if (!defined('ADMIN_USER') || $user !== ADMIN_USER) {
    http_response_code(403);
    exit(json_encode(['ok' => false, 'error' => 'Acceso denegado. Solo administradores.']));
}

// Cargar configuraci¨®n actual
$settingsFile = __DIR__ . '/settings.json';
$currentSettings = [];
if (file_exists($settingsFile)) {
    $currentSettings = json_decode(file_get_contents($settingsFile), true) ?: [];
}

// Construir nueva configuraci¨®n (fusionar con valores actuales)
$settings = [
    'gameName'          => trim($input['gameName'] ?? $currentSettings['gameName'] ?? 'FORCE MU ONLINE'),
    'backgroundImage'   => trim($input['backgroundImage'] ?? $currentSettings['backgroundImage'] ?? ''),
    'noticias'          => $input['noticias'] ?? $currentSettings['noticias'] ?? [],
    'launcherSubtitle'  => trim($input['launcherSubtitle'] ?? $currentSettings['launcherSubtitle'] ?? 'ONLINE LAUNCHER'),
    'seasonText'        => trim($input['seasonText'] ?? $currentSettings['seasonText'] ?? 'SEASON 3'),
    'logoUrl'           => trim($input['logoUrl'] ?? $currentSettings['logoUrl'] ?? ''),
    'radioStreamUrl'    => trim($input['radioStreamUrl'] ?? $currentSettings['radioStreamUrl'] ?? ''),
    'screenshots'       => $input['screenshots'] ?? $currentSettings['screenshots'] ?? [],
    'videos'            => $input['videos'] ?? $currentSettings['videos'] ?? [],
    'discordUrl'        => trim($input['discordUrl'] ?? $currentSettings['discordUrl'] ?? ''),
    'whatsappUrl'       => trim($input['whatsappUrl'] ?? $currentSettings['whatsappUrl'] ?? ''),
    'instagramUrl'      => trim($input['instagramUrl'] ?? $currentSettings['instagramUrl'] ?? ''),
	'showWC'            => filter_var($input['showWC'] ?? $currentSettings['showWC'] ?? true, FILTER_VALIDATE_BOOLEAN),
    'showWP'            => filter_var($input['showWP'] ?? $currentSettings['showWP'] ?? true, FILTER_VALIDATE_BOOLEAN),
    'showGP'            => filter_var($input['showGP'] ?? $currentSettings['showGP'] ?? true, FILTER_VALIDATE_BOOLEAN),
];

// Guardar en settings.json
if (file_put_contents($settingsFile, json_encode($settings, JSON_PRETTY_PRINT))) {
    echo json_encode(['ok' => true, 'message' => 'Ajustes guardados correctamente.']);
} else {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'No se pudo guardar el archivo de configuraci¨®n.']);
}