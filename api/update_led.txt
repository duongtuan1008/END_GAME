<?php
$servername = "localhost";
$username = "root";
$password = "your_password";
$dbname = "esp_data";

// Kết nối MySQL
$conn = new mysqli($servername, $username, $password, $dbname);

// Kiểm tra kết nối
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Kiểm tra nếu có dữ liệu gửi từ ESP32
if (isset($_GET['state'])) {
    $state = $_GET['state']; // Nhận trạng thái từ ESP32 (1 = ON, 0 = OFF)
    $device_name = "LED"; // Đặt tên thiết bị (có thể sửa nếu có nhiều thiết bị)
    
    // Xác định hoạt động của LED
    $activity = ($state == "1") ? "LED turned ON" : "LED turned OFF";
    $status = ($state == "1") ? "ON" : "OFF";

    // Cập nhật trạng thái LED vào bảng
    $sql = "INSERT INTO home_monitoring (device_name, activity, status) 
            VALUES ('$device_name', '$activity', '$status')";
    
    if ($conn->query($sql) === TRUE) {
        echo "OK"; // Trả về phản hồi thành công cho ESP32
    } else {
        echo "Error: " . $conn->error; // Báo lỗi nếu có
    }
} else {
    echo "Bad Request"; // Nếu không có dữ liệu "state", báo lỗi
}

// Đóng kết nối MySQL
$conn->close();
?>
