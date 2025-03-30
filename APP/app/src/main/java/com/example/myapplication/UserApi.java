package com.example.myapplication;
import retrofit2.Call;
import retrofit2.http.Field;
import retrofit2.http.FormUrlEncoded;
import retrofit2.http.POST;
import retrofit2.http.DELETE;
import retrofit2.http.Path;
import retrofit2.http.GET;

public interface UserApi {
    @GET("getUsers.php") // Đúng API
    Call<ApiResponse> getUsers();

    @FormUrlEncoded
    @POST("update_user.php")
    Call<Void> updateUser(@Field("id") String id, @Field("username") String username);

    @DELETE("delete_user.php/{id}")
    Call<Void> deleteUser(@Path("id") String id);
}
