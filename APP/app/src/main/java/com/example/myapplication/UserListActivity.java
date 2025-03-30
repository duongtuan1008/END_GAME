package com.example.myapplication;

import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import java.util.List;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;
import java.util.ArrayList;
import com.google.gson.Gson;


public class UserListActivity extends AppCompatActivity {
    private RecyclerView recyclerView;
    private UserAdapter adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_user_list);

        recyclerView = findViewById(R.id.recyclerViewUsers);
        recyclerView.setLayoutManager(new LinearLayoutManager(this)); // 🔥 Cực kỳ quan trọng

        adapter = new UserAdapter(new ArrayList<>(), this);
        recyclerView.setAdapter(adapter);

        fetchUsers(); // Gọi API sau khi RecyclerView đã khởi tạo
    }

    private void fetchUsers() {
        Retrofit retrofit = RetrofitClientRaspi.getClient();

        UserApi userApi = retrofit.create(UserApi.class);
        Call<ApiResponse> call = userApi.getUsers();

        call.enqueue(new Callback<ApiResponse>() {
            @Override
            public void onResponse(Call<ApiResponse> call, Response<ApiResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                    List<User> userList = response.body().getData();
                    Log.d("API_RESPONSE", "Dữ liệu nhận được: " + new Gson().toJson(userList));

                    if (userList != null && !userList.isEmpty()) {
                        adapter.updateUsers(userList);
                        recyclerView.setAdapter(adapter);
                    } else {
                        Log.w("USER_LIST_SIZE", "Danh sách user rỗng!");
                    }
                } else {
                    try {
                        String errorResponse = response.errorBody().string(); // Đọc lỗi
                        Log.e("API_ERROR", "Lỗi phản hồi từ server: " + errorResponse);
                    } catch (Exception e) {
                        Log.e("API_ERROR", "Không thể đọc lỗi từ server", e);
                    }
                }
            }


            @Override
            public void onFailure(Call<ApiResponse> call, Throwable t) {
                Log.e("API_ERROR", "Lỗi kết nối: " + t.getMessage());
            }
        });
    }
}
