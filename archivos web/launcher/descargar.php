<?php
$archivo = '../descargas/cliente.zip'; // Ruta relativa al ZIP

if (!file_exists($archivo)) {
    http_response_code(404);
    exit('Archivo no encontrado');
}

$tamano = filesize($archivo);
$nombre = 'cliente.zip';

if (isset($_SERVER['HTTP_RANGE'])) {
    preg_match('/bytes=(\d+)-(\d*)/', $_SERVER['HTTP_RANGE'], $matches);
    $inicio = intval($matches[1]);
    $fin = ($matches[2] !== '') ? intval($matches[2]) : $tamano - 1;
    $longitud = $fin - $inicio + 1;
    header('HTTP/1.1 206 Partial Content');
    header("Content-Range: bytes $inicio-$fin/$tamano");
    header('Content-Length: ' . $longitud);
} else {
    $inicio = 0;
    $longitud = $tamano;
    header('Content-Length: ' . $tamano);
}

header('Content-Type: application/octet-stream');
header('Content-Disposition: attachment; filename="' . $nombre . '"');
header('Accept-Ranges: bytes');

$fp = fopen($archivo, 'rb');
fseek($fp, $inicio);
$enviado = 0;
while (!feof($fp) && $enviado < $longitud) {
    $bloque = min(8192, $longitud - $enviado);
    echo fread($fp, $bloque);
    $enviado += $bloque;
    ob_flush();
    flush();
}
fclose($fp);
?>