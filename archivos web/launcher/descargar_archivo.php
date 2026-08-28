<?php
error_reporting(E_ALL);
ini_set('display_errors', 0);

if (!isset($_GET['file'])) {
    http_response_code(400);
    exit('Falta parámetro file');
}

$baseDir = __DIR__ . '/../descargas/juego/';
$requestedPath = str_replace(['../', '..\\'], '', $_GET['file']);
$requestedPath = ltrim($requestedPath, '/');

// Buscar archivo ignorando mayúsculas/minúsculas
$foundPath = findFileCaseInsensitive($baseDir, $requestedPath);
if ($foundPath === false) {
    http_response_code(404);
    exit("Archivo no encontrado: " . htmlspecialchars($requestedPath));
}

header('Content-Type: application/octet-stream');
header('Content-Length: ' . filesize($foundPath));
readfile($foundPath);

function findFileCaseInsensitive($baseDir, $relativePath) {
    $parts = explode('/', $relativePath);
    $currentDir = rtrim($baseDir, '/');
    foreach ($parts as $part) {
        if ($part === '') continue;
        $found = false;
        if ($handle = opendir($currentDir)) {
            while (($entry = readdir($handle)) !== false) {
                if ($entry === '.' || $entry === '..') continue;
                if (strcasecmp($entry, $part) === 0) {
                    $currentDir .= '/' . $entry;
                    $found = true;
                    break;
                }
            }
            closedir($handle);
        }
        if (!$found) return false;
    }
    return is_file($currentDir) ? $currentDir : false;
}