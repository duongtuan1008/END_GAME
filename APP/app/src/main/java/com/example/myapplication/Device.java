package com.example.myapplication;

public class Device {
    private String name;
    private int icon;
    private boolean isOn;
    private String batteryStatus;  // ✅ Thêm biến này
    private String apiName;
    private String id; // hoặc int id
    public Device(String name, int icon, boolean isOn, String batteryStatus, String apiName) {
        this.name = name;
        this.icon = icon;
        this.isOn = isOn;
        this.batteryStatus = batteryStatus;  // ✅ Gán giá trị pin
        this.apiName = apiName;
    }
    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }
    public String getName() {
        return name;
    }

    public int getIcon() {
        return icon;
    }

    public boolean isOn() {
        return isOn;
    }

    public String getBatteryStatus() {  // ✅ Thêm phương thức này
        return batteryStatus;
    }

    public String getApiName() {
        return apiName;
    }
    public void setOn(boolean state) {
        this.isOn = state;
    }

}
