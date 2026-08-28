<?php
session_start();
header('Content-Type: application/json');

$num1 = rand(1, 10);
$num2 = rand(1, 10);
$_SESSION['captcha_answer'] = $num1 + $num2;

echo json_encode([
    'pregunta' => "¿Cuánto es {$num1} + {$num2}?",
    'token'    => session_id()
]);