<?php
header('Content-Type: image/png');
require_once __DIR__ . '/../config/config.php';

$guildName = $_GET['name'] ?? '';
$size = isset($_GET['size']) ? intval($_GET['size']) : 40;

if (!$guildName) {
    // Imagen por defecto (gris)
    $img = imagecreatetruecolor($size, $size);
    $gray = imagecolorallocate($img, 128, 128, 128);
    imagefill($img, 0, 0, $gray);
    imagepng($img);
    imagedestroy($img);
    exit;
}

try {
    $dsn = "dblib:host=" . DB_HOST . ";dbname=" . DB_NAME;
    $pdo = new PDO($dsn, DB_USER, DB_PASS);
    
    $stmt = $pdo->prepare("SELECT G_Mark FROM Guild WHERE G_Name = :name");
    $stmt->execute([':name' => $guildName]);
    $row = $stmt->fetch(PDO::FETCH_ASSOC);
    
    $binaryData = $row['G_Mark'] ?? '';
    
    // Si está vacío, usar valor por defecto
    if (empty($binaryData)) {
        $binaryData = '1111111111111111111111111114411111144111111111111111111111111111';
    }
    
    // Si es binario, convertir a hexadecimal
    if (is_string($binaryData) && strlen($binaryData) === 32) {
        $binaryData = bin2hex($binaryData);
    }
    
    // Si no tiene 64 caracteres, usar por defecto
    if (strlen($binaryData) !== 64) {
        $binaryData = '1111111111111111111111111114411111144111111111111111111111111111';
    }
    
    $hex = $binaryData;
    $pixelSize = $size / 8;
    
    // Mapa de colores (sacado del Web Engine)
    $colorMap = [
        '0' => '#111111', '1' => '#000000', '2' => '#808080', '3' => '#ffffff',
        '4' => '#fe0000', '5' => '#ff7f00', '6' => '#ffff00', '7' => '#80ff00',
        '8' => '#00ff01', '9' => '#00fe81', 'A' => '#00ffff', 'B' => '#0080ff',
        'C' => '#0000fe', 'D' => '#7f00ff', 'E' => '#ff00fe', 'F' => '#ff0080'
    ];
    
    // Decodificar cuadrícula 8x8
    $grid = [];
    for ($y = 0; $y < 8; $y++) {
        for ($x = 0; $x < 8; $x++) {
            $grid[$y][$x] = strtoupper(substr($hex, ($y * 8) + $x, 1));
        }
    }
    
    // Crear imagen escalada
    $img = imagecreatetruecolor($size, $size);
    
    for ($y = 0; $y < $size; $y++) {
        for ($x = 0; $x < $size; $x++) {
            $gridX = intval($x / $pixelSize);
            $gridY = intval($y / $pixelSize);
            $colorHex = $colorMap[$grid[$gridY][$gridX]] ?? '#000000';
            
            $r = hexdec(substr($colorHex, 1, 2));
            $g = hexdec(substr($colorHex, 3, 2));
            $b = hexdec(substr($colorHex, 5, 2));
            
            $color = imagecolorallocate($img, $r, $g, $b);
            imagesetpixel($img, $x, $y, $color);
        }
    }
    
    // Hacer transparente el color #111111
    $transparent = imagecolorallocate($img, 17, 17, 17);
    imagecolortransparent($img, $transparent);
    
    imagepng($img);
    imagedestroy($img);
    
} catch (PDOException $e) {
    $img = imagecreatetruecolor($size, $size);
    $gray = imagecolorallocate($img, 128, 128, 128);
    imagefill($img, 0, 0, $gray);
    imagepng($img);
    imagedestroy($img);
}