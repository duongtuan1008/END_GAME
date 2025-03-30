package com.example.myapplication;
import androidx.core.content.ContextCompat;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.squareup.picasso.Picasso;
import java.util.List;
import android.util.Log;
import java.util.ArrayList;
import com.squareup.picasso.Callback;
import com.squareup.picasso.MemoryPolicy;
import com.squareup.picasso.NetworkPolicy;
import android.content.Intent;

import retrofit2.Call;
import retrofit2.Response;

public class UserAdapter extends RecyclerView.Adapter<UserAdapter.UserViewHolder> {
    private List<User> userList;
    private Context context;

    public UserAdapter(List<User> userList, Context context) {
        this.userList = userList;
        this.context = context;
    }

    @NonNull
    @Override
    public UserViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_user, parent, false);
        return new UserViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull UserViewHolder holder, int position) {
        User user = userList.get(position);

        // 📝 Log thông tin user để kiểm tra
        Log.d("USER_ADAPTER", "Đang hiển thị user: " + user.getUsername() + " - Vị trí: " + position);

        // 🖋️ Hiển thị tên user
        holder.userName.setText(user.getUsername());

        // 🖼️ Kiểm tra danh sách ảnh và lấy ảnh đầu tiên
        if (user.getImagePaths() != null && !user.getImagePaths().isEmpty()) {
            String baseUrl = "http://192.168.137.88/";  // URL gốc của máy chủ Apache
            String imagePath = user.getImagePaths().get(0);

            // 🛠️ Kiểm tra và điều chỉnh đường dẫn ảnh
            String imageUrl;
            if (imagePath.startsWith("dataset/")) {
                imageUrl = baseUrl + imagePath;
            } else {
                imageUrl = baseUrl + "dataset/" + imagePath;
            }

            // 📝 Log URL ảnh để kiểm tra
            Log.d("USER_ADAPTER", "Ảnh của " + user.getUsername() + ": " + imageUrl);

            // 🖼️ Tải ảnh bằng Picasso
            Picasso.get()
                    .load(imageUrl)
                    .networkPolicy(NetworkPolicy.NO_CACHE, NetworkPolicy.NO_STORE) // Không dùng cache
                    .memoryPolicy(MemoryPolicy.NO_CACHE, MemoryPolicy.NO_STORE) // Xóa cache bộ nhớ
                    .placeholder(R.drawable.ic_user) // Ảnh mặc định khi đang tải
                    .error(R.drawable.ic_user) // Ảnh lỗi
                    .into(holder.userImage, new Callback() { // 🔥 Đặt Callback vào đây
                        @Override
                        public void onSuccess() {
                            Log.d("PICASSO", "✅ Tải ảnh thành công: " + imageUrl);
                        }

                        @Override
                        public void onError(Exception e) {
                            Log.e("PICASSO", "❌ Lỗi tải ảnh: " + imageUrl, e);
                        }
                    });

        } else {
            // Nếu không có ảnh, hiển thị ảnh mặc định
            holder.userImage.setImageResource(R.drawable.ic_user);
        }

        // 🔍 Hiển thị trạng thái vân tay
        if (user.getFingerprintId() != null && !user.getFingerprintId().isEmpty()) {
            holder.fingerprintStatus.setText("Vân tay đã đăng ký");
            holder.fingerprintStatus.setTextColor(ContextCompat.getColor(context, R.color.green));
        } else {
            holder.fingerprintStatus.setText("Vân tay: Chưa đăng ký");
            holder.fingerprintStatus.setTextColor(ContextCompat.getColor(context, R.color.red));
        }

        // 🖱️ **Bắt sự kiện click để mở chi tiết user**
        holder.itemView.setOnClickListener(v -> {
            Intent intent = new Intent(context, UserDetailActivity.class);
            intent.putExtra("user_data", user);  // ✅ Truyền user qua Intent
            context.startActivity(intent);
        });
    }

    @Override
    public int getItemCount() {
        Log.d("USER_ADAPTER", "Số lượng user trong adapter: " + (userList != null ? userList.size() : 0));
        return userList != null ? userList.size() : 0;
    }



    public static class UserViewHolder extends RecyclerView.ViewHolder {
        TextView userName;
        ImageView userImage;
        TextView fingerprintStatus;  // Thêm TextView để hiển thị vân tay

        public UserViewHolder(@NonNull View itemView) {
            super(itemView);
            userName = itemView.findViewById(R.id.textViewUsername);
            userImage = itemView.findViewById(R.id.imageViewUser);
            fingerprintStatus = itemView.findViewById(R.id.textViewFingerprintStatus); // Ánh xạ TextView hiển thị vân tay
        }
    }
    public void updateUsers(List<User> newUsers) {
        userList.clear();
        userList.addAll(newUsers);
        notifyDataSetChanged();  // 🔥 Cập nhật giao diện
    }



}
