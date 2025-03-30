<?php
// Hiển thị lỗi để debug
error_reporting(E_ALL);
ini_set('display_errors', 1);

header("Content-Type: application/json");

// Kết nối database
$servername = "localhost";
$username = "root";
$password = "100803";
$dbname = "home_monitoring";

$conn = new mysqli($servername, $username, $password, $dbname);

// Kiểm tra kết nối
if ($conn->connect_error) {
    die(json_encode(["error" => "[ERROR] Kết nối MySQL thất bại: " . $conn->connect_error]));
}

// Kiểm tra dữ liệu gửi từ ESP32
if (!isset($_GET['device_name']) || !isset($_GET['activity']) || !isset($_GET['status'])) {
    die(json_encode(["error" => "[ERROR] Thiếu tham số GET"]));
}

// Nhận dữ liệu từ ESP32
$device = urldecode($_GET['device_name']);
$activity = urldecode($_GET['activity']);
$status = urldecode($_GET['status']);

// Chèn dữ liệu vào MySQL
$sql = "INSERT INTO activity_log (device_name, activity, status) VALUES ('$device', '$activity', '$status')";
$conn->query($sql);

// Lấy trạng thái mới nhất của tất cả các thiết bị
$sql_latest = "SELECT device_name, status FROM activity_log WHERE id IN (SELECT MAX(id) FROM activity_log GROUP BY device_name)";
$result = $conn->query($sql_latest);

$latest_status = [];
while ($row = $result->fetch_assoc()) {
    $latest_status[$row['device_name']] = $row['status'];
}

$conn->close();

// Trả về JSON chứa trạng thái mới nhất của các thiết bị
echo json_encode(["success" => true, "latest_status" => $latest_status]);
?>
