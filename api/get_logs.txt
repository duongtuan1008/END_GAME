<?php
header("Content-Type: application/json");
header("Access-Control-Allow-Origin: *");

// Kết nối database
$servername = "localhost";
$username = "root"; // Thay bằng username MySQL
$password = "100803"; // Thay bằng mật khẩu MySQL
$dbname = "home_monitoring"; // Tên database

$conn = new mysqli($servername, $username, $password, $dbname);

// Kiểm tra kết nối
if ($conn->connect_error) {
    die(json_encode(["error" => "Kết nối MySQL thất bại: " . $conn->connect_error]));
}

// Lấy 10 log gần nhất
$sql = "SELECT device_name, activity, status, timestamp FROM activity_log ORDER BY timestamp DESC LIMIT 10";
$result = $conn->query($sql);

$logs = [];
if ($result->num_rows > 0) {
    while ($row = $result->fetch_assoc()) {
        $logs[] = $row;
    }
}

$conn->close();
echo json_encode($logs);
?>
