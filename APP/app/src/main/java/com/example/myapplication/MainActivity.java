package com.example.myapplication;

import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import okhttp3.*;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.HashMap;
import android.os.Handler;
import com.google.android.material.button.MaterialButton;
import android.content.Intent;
import android.view.View;

public class MainActivity extends AppCompatActivity {
    private APIService apiRaspi, apiESP;
    private RecyclerView recyclerDevices;
    private DeviceAdapter deviceAdapter;
    private List<Device> deviceList;
    private TextView txtTemperature, txtHumidity;
    private OkHttpClient client;
    private WebSocket webSocket;
    private Map<String, String> lastDeviceStatus = new HashMap<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        MaterialButton btnLivingRoom = findViewById(R.id.btnLivingRoom);

        // Gán sự kiện click để mở LivingActivity
        btnLivingRoom.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, LivingActivity.class);
                startActivity(intent);
            }
        });
        MaterialButton btnOpenUserList = findViewById(R.id.userList);
        btnOpenUserList.setOnClickListener(new View.OnClickListener() { @Override
           public void onClick(View v) {
               Intent intent = new Intent(MainActivity.this, UserListActivity.class);
               startActivity(intent);
           }
        });
        // Ánh xạ UI
        txtTemperature = findViewById(R.id.txtTemperature);
        txtHumidity = findViewById(R.id.txtHumidity);
        recyclerDevices = findViewById(R.id.recyclerDevices);
        recyclerDevices.setLayoutManager(new LinearLayoutManager(this));

        // Khởi tạo API
        apiRaspi = RetrofitClientRaspi.getClient().create(APIService.class);
        apiESP = RetrofitClientESP.getInstance().create(APIService.class);

        // Khởi tạo danh sách thiết bị
        deviceList = new ArrayList<>();
        deviceAdapter = new DeviceAdapter(deviceList, apiESP);
        recyclerDevices.setAdapter(deviceAdapter);

        // Gọi API lấy dữ liệu từ Raspberry Pi & ESP32
        fetchSensorData();
        connectWebSocket();
        fetchDeviceStatus();
        startAutoRefresh();   // Kiểm tra API mỗi 5 giây
    }
    private void connectWebSocket() {
        client = new OkHttpClient();
        Request request = new Request.Builder().url("ws://192.168.137.50/ws").build();
        webSocket = client.newWebSocket(request, new WebSocketListener() {
            public void onOpen(WebSocket webSocket, Response response) {
                Log.d("WebSocket", "Kết nối WebSocket thành công!");
            }

            @Override
            public void onMessage(WebSocket webSocket, String text) {
                Log.d("WebSocket", "Dữ liệu WebSocket: " + text);

                try {
                    JSONObject jsonObject = new JSONObject(text);
                    String deviceName = jsonObject.getString("device");
                    String status = jsonObject.getString("status");

                    runOnUiThread(() -> updateDeviceStatus(deviceName, status));
                } catch (JSONException e) {
                    Log.e("WebSocket", "Lỗi xử lý JSON: " + e.getMessage());
                }
            }

            public void onFailure(WebSocket webSocket, Throwable t, Response response) {
                Log.e("WebSocket", "Lỗi WebSocket: " + t.getMessage());
            }
        });
    }

    private void updateDeviceStatus(String deviceId, String status) {
        for (Device device : deviceList) {
            if (device.getId().equals(deviceId)) {
                boolean newStatus = status.equals("ON");
                if (device.isOn() != newStatus) {
                    device.setOn(newStatus);
                    runOnUiThread(() -> deviceAdapter.notifyItemChanged(deviceList.indexOf(device)));
                    Log.d("STATUS_UPDATE", "⚡ Đã cập nhật trạng thái thiết bị: " + device.getId());
                }
                return;
            }
        }
    }

    private void fetchDeviceStatus() {
        Call<Map<String, String>> callStatus = apiESP.getStatus();
        callStatus.enqueue(new Callback<Map<String, String>>() {
            @Override
            public void onResponse(Call<Map<String, String>> call, Response<Map<String, String>> response) {
                if (response.isSuccessful() && response.body() != null) {
                    Map<String, String> newDeviceStatus = response.body();

                    // 🚀 So sánh dữ liệu mới với dữ liệu cũ
                    if (!newDeviceStatus.equals(lastDeviceStatus)) {
                        Log.d("API_DEVICE", "Thay đổi trạng thái! Cập nhật UI.");
                        lastDeviceStatus = new HashMap<>(newDeviceStatus); // Cập nhật trạng thái mới
                        deviceAdapter.updateDevices(newDeviceStatus);
                    } else {
                        Log.d("API_DEVICE", "Không có thay đổi. Bỏ qua cập nhật.");
                    }
                } else {
                    Log.e("API_DEVICE", "Lỗi API: " + response.code());
                }
            }

            @Override
            public void onFailure(Call<Map<String, String>> call, Throwable t) {
                Log.e("API_DEVICE", "Lỗi kết nối API ESP32: " + t.getMessage());
            }
        });
    }
    private void startAutoRefresh() {
        Handler handler = new Handler();
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                fetchDeviceStatus(); // Gọi API để lấy trạng thái mới
                handler.postDelayed(this, 5000); // Kiểm tra lại sau 5 giây
            }
        };
        handler.post(runnable);
    }

    private void fetchSensorData() {
//        // ✅ Gọi API trạng thái thiết bị từ ESP32
//        Call<Map<String, String>> callStatus = apiESP.getStatus();
//        callStatus.enqueue(new Callback<Map<String, String>>() {
//            @Override
//            public void onResponse(Call<Map<String, String>> call, Response<Map<String, String>> response) {
//                if (response.isSuccessful() && response.body() != null) {
//                    Map<String, String> devices = response.body();
//                    Log.d("API_DEVICE", "Dữ liệu nhận từ ESP32: " + devices.toString());
//                    deviceAdapter.updateDevices(devices);
//                } else {
//                    Log.e("API_DEVICE", "Lỗi API: " + response.code());
//                }
//            }
//
//            @Override
//            public void onFailure(Call<Map<String, String>> call, Throwable t) {
//                Log.e("API_DEVICE", "Lỗi kết nối API ESP32: " + t.getMessage());
//            }
//        });

        // ✅ Gọi API cảm biến (nhiệt độ, độ ẩm) từ Raspberry Pi
        Call<ApiResponse> callSensor = apiRaspi.getData();
        callSensor.enqueue(new Callback<ApiResponse>() {
            @Override
            public void onResponse(Call<ApiResponse> call, Response<ApiResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                    List<SensorData> sensors = response.body().getSensors();
                    if (sensors != null && !sensors.isEmpty()) {
                        updateSensorUI(sensors.get(0));
                    } else {
                        Log.e("API_SENSOR", "Lỗi: response.getSensors() trả về null hoặc rỗng!");
                    }
                } else {
                    Log.e("API_SENSOR", "Lỗi API: " + response.code());
                }
            }

            @Override
            public void onFailure(Call<ApiResponse> call, Throwable t) {
                Log.e("API_SENSOR", "Lỗi kết nối API cảm biến: " + t.getMessage());
            }
        });
    }


    private void updateSensorUI(SensorData sensor) {
        if (sensor != null) {
            Log.d("UPDATE_UI", "Cập nhật UI: Nhiệt độ = " + sensor.getTemperature() + "°C, Độ ẩm = " + sensor.getHumidity() + "%");
            txtTemperature.setText(sensor.getTemperature() + "°C");
            txtHumidity.setText(sensor.getHumidity() + "%");
        } else {
            Log.e("UPDATE_UI", "Dữ liệu sensor bị null!");
        }
    }

    private void loadDevices() {
        deviceList = new ArrayList<>();
        deviceList.add(new Device("Living Room Light", R.drawable.ic_bulb, false, "Battery: 100%", "livingroom"));
        deviceList.add(new Device("Desk Lamp", R.drawable.ic_bulb, false, "Battery: 80%", "desklamp"));
        deviceList.add(new Device("Fan", R.drawable.ic_air_conditioner, false, "Battery: 60%", "fan"));
        deviceList.add(new Device("Kitchen Light", R.drawable.ic_bulb, false, "Battery: 90%", "kitchen"));
        deviceList.add(new Device("Bedroom Light", R.drawable.ic_bulb, false, "Battery: 75%", "bedroom"));
        deviceList.add(new Device("Garage Door", R.drawable.ic_bulb, false, "Battery: 50%", "garage"));

        // Gán Adapter vào RecyclerView
        deviceAdapter = new DeviceAdapter(deviceList, apiESP);
        recyclerDevices.setAdapter(deviceAdapter);
    }

}
