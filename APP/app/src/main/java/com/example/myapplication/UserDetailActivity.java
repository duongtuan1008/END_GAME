package com.example.myapplication;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import retrofit2.Callback;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import retrofit2.Callback;
import com.squareup.picasso.MemoryPolicy;
import com.squareup.picasso.NetworkPolicy;
import android.widget.LinearLayout;
import com.squareup.picasso.Picasso;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class UserDetailActivity extends AppCompatActivity {
    private ImageView userImage;
    private EditText edtUsername;
    private TextView txtFingerprintStatus;
    private Button btnUpdate, btnDelete;
    private ImageView userImageDetail;

    private User user;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_user_detail);

        edtUsername = findViewById(R.id.edtUsername);
        txtFingerprintStatus = findViewById(R.id.txtFingerprintStatus);
        btnUpdate = findViewById(R.id.btnUpdate);
        btnDelete = findViewById(R.id.btnDelete);

        LinearLayout imageContainer = findViewById(R.id.imageContainer);

        user = (User) getIntent().getSerializableExtra("user_data");

        if (user != null) {
            edtUsername.setText(user.getUsername());
            txtFingerprintStatus.setText(
                    (user.getFingerprintId() != null && !user.getFingerprintId().isEmpty())
                            ? "Vân tay đã đăng ký"
                            : "Vân tay chưa đăng ký"
            );

            if (user.getImagePaths() != null && !user.getImagePaths().isEmpty()) {
                Log.d("DEBUG", "✅ imageContainer found OK");
                Log.d("DEBUG", "Số ảnh nhận được: " + user.getImagePaths().size());

                for (String imagePath : user.getImagePaths()) {
                    Log.d("DEBUG", "Ảnh: " + imagePath);
                    String imageUrl = buildFullImageUrl(imagePath);

                    ImageView imageView = new ImageView(this);
                    LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                            LinearLayout.LayoutParams.MATCH_PARENT,
                            500
                    );
                    params.setMargins(0, 16, 0, 16);
                    imageView.setLayoutParams(params);
                    imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);

                    Picasso.get()
                            .load(imageUrl)
                            .placeholder(R.drawable.ic_user)
                            .error(R.drawable.ic_user)
                            .into(imageView);

                    imageContainer.addView(imageView);
                }

                Log.d("DEBUG", "Tổng số ảnh add vào layout: " + imageContainer.getChildCount());
            } else {
                Toast.makeText(this, "Người dùng không có ảnh", Toast.LENGTH_SHORT).show();
            }
        }

        btnUpdate.setOnClickListener(view -> updateUser());
        btnDelete.setOnClickListener(view -> deleteUser());
    }
    // 📝 Cập nhật thông tin người dùng
    private void updateUser() {
        String newUsername = edtUsername.getText().toString().trim();
        if (newUsername.isEmpty()) {
            Toast.makeText(this, "Tên không được để trống!", Toast.LENGTH_SHORT).show();
            return;
        }

        UserApi userApi = RetrofitClientRaspi.getClient().create(UserApi.class);
        Call<Void> call = userApi.updateUser(user.getId(), newUsername);

        call.enqueue(new Callback<Void>() {
            @Override
            public void onResponse(Call<Void> call, Response<Void> response) {
                if (response.isSuccessful()) {
                    Toast.makeText(UserDetailActivity.this, "Cập nhật thành công!", Toast.LENGTH_SHORT).show();
                    finish(); // Quay lại màn hình trước
                } else {
                    Toast.makeText(UserDetailActivity.this, "Lỗi cập nhật!", Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(Call<Void> call, Throwable t) {
                Log.e("API_ERROR", "Lỗi cập nhật: " + t.getMessage());
            }
        });
    }
    private String buildFullImageUrl(String imagePath) {
        String baseUrl = "http://192.168.137.88/";
        if (imagePath.startsWith("dataset/")) {
            return baseUrl + imagePath;
        } else {
            return baseUrl + "dataset/" + imagePath;
        }
    }
    // 🗑 Xóa User
    private void deleteUser() {
        UserApi userApi = RetrofitClientRaspi.getClient().create(UserApi.class);
        Call<Void> call = userApi.deleteUser(user.getId());

        call.enqueue(new Callback<Void>() {
            @Override
            public void onResponse(Call<Void> call, Response<Void> response) {
                if (response.isSuccessful()) {
                    Toast.makeText(UserDetailActivity.this, "Xóa thành công!", Toast.LENGTH_SHORT).show();
                    finish(); // Quay lại màn hình trước
                } else {
                    Toast.makeText(UserDetailActivity.this, "Lỗi xóa!", Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(Call<Void> call, Throwable t) {
                Log.e("API_ERROR", "Lỗi xóa: " + t.getMessage());
            }
        });
    }
}
