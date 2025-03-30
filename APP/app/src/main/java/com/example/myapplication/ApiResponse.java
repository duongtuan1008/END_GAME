package com.example.myapplication;

import java.util.List;

public class ApiResponse {
    private List<Device> devices;  // Thêm danh sách thiết bị
    private List<ActivityLog> logs;   // Lưu lịch sử hoạt động của thiết bị
    private List<SensorData> sensors; // Lưu dữ liệu cảm biến (temperature, humidity)
    private String status;
    private List<User> data;
    public List<ActivityLog> getLogs() {
        return logs;
    }
    public List<Device> getDevices() {  // Thêm phương thức lấy danh sách thiết bị
        return devices;
    }
    public List<SensorData> getSensors() {
        return sensors;
    }
    public String getStatus() {
        return status;
    }

    public List<User> getData() {
        return data;
    }

}
