package com.example.myapplication;

import retrofit2.Call;
import retrofit2.http.GET;
import retrofit2.http.Path;
import java.util.Map;

public interface APIService {
    // Lấy dữ liệu cảm biến từ Raspberry Pi
    @GET("api/get_sensors_data.php")
    Call<ApiResponse> getData();

    // Lấy trạng thái thiết bị từ ESP32
    @GET("/status")
    Call<Map<String, String>> getStatus();

    // Bật/tắt thiết bị trên ESP32
    @GET("toggle/{device}")
    Call<String> toggleDevice(@Path("device") String device);
}
