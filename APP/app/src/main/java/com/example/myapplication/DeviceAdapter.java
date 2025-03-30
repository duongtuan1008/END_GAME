package com.example.myapplication;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.github.angads25.toggle.widget.LabeledSwitch;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;
import java.util.List;
import java.util.Map;
import android.os.Handler;
import android.os.Looper;
import java.util.ArrayList;

public class DeviceAdapter extends RecyclerView.Adapter<DeviceAdapter.DeviceViewHolder> {
    private List<Device> deviceList;
    private APIService apiESP; // API dùng để giao tiếp với ESP32

    public DeviceAdapter(List<Device> deviceList, APIService apiESP) {
        this.deviceList = deviceList;
        this.apiESP = apiESP;
    }

    @NonNull
    @Override
    public DeviceViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_device, parent, false);
        return new DeviceViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull DeviceViewHolder holder, int position) {
        Device device = deviceList.get(position);

        // ✅ Cập nhật UI
        holder.txtDeviceName.setText(device.getName());
        holder.imgDeviceIcon.setImageResource(device.getIcon());
        holder.txtBatteryStatus.setText(device.getBatteryStatus());

        // 🚀 Đảm bảo `Switch` phản ánh trạng thái hiện tại
        holder.switchDevice.setOnToggledListener(null); // Xóa listener cũ
        holder.switchDevice.setOn(device.isOn()); // Cập nhật đúng trạng thái
        holder.switchDevice.setVisibility(View.VISIBLE); // Đảm bảo nút hiển thị

        // ✅ Xử lý sự kiện bật/tắt thiết bị
        holder.switchDevice.setOnToggledListener((toggleableView, isOn) -> {
            String deviceApiName = device.getApiName();

            // ✅ Gửi yêu cầu API đến ESP32
            apiESP.toggleDevice(deviceApiName).enqueue(new Callback<String>() {
                @Override
                public void onResponse(Call<String> call, Response<String> response) {
                    if (response.isSuccessful()) {
                        Toast.makeText(holder.itemView.getContext(),
                                "Đã điều khiển: " + device.getName(),
                                Toast.LENGTH_SHORT).show();
                    } else {
                        Toast.makeText(holder.itemView.getContext(),
                                "Lỗi khi điều khiển thiết bị!",
                                Toast.LENGTH_SHORT).show();

                        // 🚀 Nếu lỗi, khôi phục lại trạng thái trước đó
                        holder.switchDevice.setOnToggledListener(null);
                        holder.switchDevice.setOn(!isOn);
                        holder.switchDevice.setOnToggledListener((toggleableView1, isOn1) -> {});
                    }
                }

                @Override
                public void onFailure(Call<String> call, Throwable t) {
                    Log.e("DEVICE_CONTROL", "Lỗi kết nối: " + t.getMessage());
                    Toast.makeText(holder.itemView.getContext(),
                            "Lỗi kết nối API ESP32!",
                            Toast.LENGTH_SHORT).show();

                    // 🚀 Nếu lỗi, khôi phục lại trạng thái trước đó
                    holder.switchDevice.setOnToggledListener(null);
                    holder.switchDevice.setOn(!isOn);
                    holder.switchDevice.setOnToggledListener((toggleableView1, isOn1) -> {});
                }
            });
        });
    }


    @Override
    public int getItemCount() {
        return deviceList.size();
    }

    static class DeviceViewHolder extends RecyclerView.ViewHolder {
        TextView txtDeviceName, txtBatteryStatus;
        ImageView imgDeviceIcon;
        LabeledSwitch switchDevice;

        DeviceViewHolder(View itemView) {
            super(itemView);
            txtDeviceName = itemView.findViewById(R.id.txtDeviceName);
            txtBatteryStatus = itemView.findViewById(R.id.txtBatteryStatus);
            imgDeviceIcon = itemView.findViewById(R.id.imgDeviceIcon);
            switchDevice = itemView.findViewById(R.id.switchDevice);
        }
    }
    public void updateDevices(Map<String, String> devices) {
        List<Device> updatedList = new ArrayList<>();
        for (Map.Entry<String, String> entry : devices.entrySet()) {
            updatedList.add(new Device(entry.getKey(), R.drawable.ic_bulb, entry.getValue().equals("ON"), "Battery: 100%", entry.getKey()));
        }
        deviceList.clear();
        deviceList.addAll(updatedList);

        new Handler(Looper.getMainLooper()).post(this::notifyDataSetChanged); // Cập nhật UI trên MainThread
    }


}
