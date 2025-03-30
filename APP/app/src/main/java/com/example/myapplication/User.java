package com.example.myapplication;

import com.google.gson.annotations.SerializedName;
import java.io.Serializable;
import java.util.List;

public class User implements Serializable {
    @SerializedName("id")
    private String id;

    @SerializedName("username")
    private String username;

    @SerializedName("image_path")  // 💡 JSON có tên "image_path", sửa lại cho đúng!
    private List<String> imagePaths;

    @SerializedName("fingerprint_id")  // 💡 JSON có tên "fingerprint_id"
    private String fingerprintId;

    // Constructor
    public User(String id, String username, List<String> imagePaths, String fingerprintId) {
        this.id = id;
        this.username = username;
        this.imagePaths = imagePaths;
        this.fingerprintId = fingerprintId;
    }

    // Getters
    public String getId() {
        return id;
    }

    public String getUsername() {
        return username;
    }

    public List<String> getImagePaths() {
        return imagePaths;
    }

    public String getFingerprintId() {
        return fingerprintId;
    }

    // Setters
    public void setUsername(String username) {
        this.username = username;
    }

    public void setImagePaths(List<String> imagePaths) {
        this.imagePaths = imagePaths;
    }

    public void setFingerprintId(String fingerprintId) {
        this.fingerprintId = fingerprintId;
    }
}
