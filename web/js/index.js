function toggleColor(element) {
  // Kiểm tra nếu phần tử đã có class 'active' thì bỏ đi, nếu chưa có thì thêm vào
  element.classList.toggle("active");
}
function toggleActive(button) {
  button.classList.toggle("active");
}
document.addEventListener("DOMContentLoaded", function () {
  const modesContainer = document.querySelector(".modes");

  modesContainer.addEventListener("click", function (event) {
    const button = event.target.closest("button");

    if (!button) return; // Nếu không bấm vào nút thì bỏ qua

    // Xử lý riêng cho nút "In Bed"
    if (
      button.classList.contains("locked") ||
      button.classList.contains("unlocked")
    ) {
      if (button.classList.contains("unlocked")) {
        // Nếu đã mở khóa, click lần nữa sẽ khóa lại
        button.classList.remove("unlocked", "active");
        button.classList.add("locked");

        // Thêm lại icon khóa
        const lockIcon = document.createElement("i");
        lockIcon.classList.add("fa-solid", "fa-lock", "lock-icon");
        button.appendChild(lockIcon);
      } else {
        // Nếu đang khóa, mở khóa và đổi màu
        button.classList.remove("locked");
        button.classList.add("unlocked", "active");

        // Xóa icon khóa
        const lockIcon = button.querySelector(".lock-icon");
        if (lockIcon) lockIcon.remove();
      }
      return;
    }

    // Toggle class "active" cho tất cả các nút khác
    button.classList.toggle("active");
  });
});
document.addEventListener("DOMContentLoaded", function () {
  const securityLocks = document.querySelectorAll(".security .lock");

  securityLocks.forEach((lock) => {
    lock.addEventListener("click", function () {
      if (this.classList.contains("locked")) {
        // Mở khóa
        this.classList.remove("locked");
        this.classList.add("unlocked");
        this.innerHTML = `<i class="fa-solid fa-unlock"></i> <span>${this.innerText.replace(
          "Locked",
          "Unlocked"
        )}</span>`;
      } else {
        // Khóa lại
        this.classList.remove("unlocked");
        this.classList.add("locked");
        this.innerHTML = `<i class="fa-solid fa-lock"></i> <span>${this.innerText.replace(
          "Unlocked",
          "Locked"
        )}</span>`;
      }
    });
  });
});
function fetchSensorData() {
  fetch("/api/get_sensors_data.php")
    .then((response) => response.json())
    .then((data) => {
      console.log("Dữ liệu từ API:", data); // Debug xem có dữ liệu không

      if (!Array.isArray(data)) {
        console.error("Dữ liệu trả về không hợp lệ:", data);
        return;
      }

      data.forEach((sensor) => {
        if (
          !sensor.location ||
          sensor.temperature === "0" ||
          sensor.humidity === "0"
        ) {
          console.warn("Bỏ qua dữ liệu không hợp lệ:", sensor);
          return;
        }

        let roomElement = document.querySelector(
          `.room[data-room="${sensor.location}"]`
        );
        if (roomElement) {
          roomElement.querySelector(".temperature").textContent =
            sensor.temperature;
          roomElement.querySelector(".humidity").textContent = sensor.humidity;
        }
      });
    })
    .catch((error) => console.error("Lỗi tải dữ liệu cảm biến:", error));
}

// Cập nhật dữ liệu mỗi 5 giây
setInterval(fetchSensorData, 5000);
fetchSensorData();

// Hiển thị giờ Việt Nam (múi giờ Hà Nội)
function updateTime() {
  const now = new Date();
  const options = {
    timeZone: "Asia/Bangkok",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  };
  const timeString = now.toLocaleTimeString("vi-VN", options);
  document.getElementById("time-box").textContent = timeString;
}
setInterval(updateTime, 1000);
updateTime();

// Lấy dữ liệu thời tiết Hà Nội từ OpenWeatherMap
async function fetchWeather() {
  const apiKey = "c87db1d2580dc72154847239c6d6e75b"; // Thay bằng API key của bạn
  const city = "Hanoi";
  const url = `https://api.openweathermap.org/data/2.5/weather?q=${city}&appid=${apiKey}&units=metric&lang=vi`;

  try {
    const response = await fetch(url);
    const data = await response.json();

    if (data.cod === 200) {
      const temperature = data.main.temp;
      document.getElementById("temperature").textContent = `${temperature}°C`;
    } else {
      document.getElementById("temperature").textContent = "--°C";
    }
  } catch (error) {
    console.error("Lỗi khi lấy dữ liệu thời tiết:", error);
    document.getElementById("temperature").textContent = "--°C";
  }
}
fetchWeather();


let ESP32_IP = "http://192.168.137.50"; // Địa chỉ IP của ESP32

// ✅ Hàm cập nhật trạng thái từ ESP32
function updateStatus() {
  fetch(`${ESP32_IP}/status`)
    .then((response) => response.json())
    .then((data) => {
      Object.keys(data).forEach((device) => {
        let button = document.getElementById(`toggle-${device}`);
        if (button) {
          if (data[device] === "ON") {
            button.classList.add("active");
          } else {
            button.classList.remove("active");
          }
        }
      });
    })
    .catch((error) =>
      console.error("[ERROR] Không thể lấy trạng thái từ ESP32:", error)
    );
}

// ✅ Hàm gửi yêu cầu bật/tắt thiết bị
function toggleDevice(device) {
  fetch(`${ESP32_IP}/toggle/${device}`)
    .then(() => updateStatus())
    .catch((error) => console.error("[ERROR] Không thể gửi yêu cầu:", error));
}

// ✅ Cập nhật trạng thái mỗi 2 giây
setInterval(updateStatus, 2000);

// ✅ Gán sự kiện click vào các nút thiết bị
document.addEventListener("DOMContentLoaded", function () {
  document.querySelectorAll(".device").forEach((button) => {
    button.addEventListener("click", () => toggleDevice(button.dataset.device));
  });
  updateStatus();
});

// 🔄 Hàm cập nhật dữ liệu từ API
function updateActivityLog() {
  fetch("http://192.168.137.88/api/get_sensors_data.php")
    .then((response) => response.json())
    .then((data) => {
      let activityList = document.querySelector(".activities ul");
      activityList.innerHTML = ""; // Xóa dữ liệu cũ trước khi cập nhật

      // 🟢 Kiểm tra nếu có logs
      if (data.logs && data.logs.length > 0) {
        data.logs.forEach((log) => {
          let timeAgo = getTimeAgo(log.timestamp);
          let icon = getIcon(log.activity);

          let listItem = document.createElement("li");
          listItem.innerHTML = `${icon} ${log.device_name} - ${log.activity} (${log.status}) <span>${timeAgo}</span>`;
          activityList.appendChild(listItem);
        });
      } else {
        activityList.innerHTML = "<li>📌 Chưa có dữ liệu</li>";
      }

      // Sau khi cập nhật logs, cập nhật luôn nhiệt độ & độ ẩm
      if (data.sensors && data.sensors.length > 0) {
        updateSensorStatus(data.sensors);
      }
    })
    .catch((error) => console.error("❌ Lỗi tải dữ liệu:", error));
}

// ✅ Cập nhật Nhiệt độ & Độ ẩm lên `.status`
function updateSensorStatus(sensors) {
  if (!Array.isArray(sensors) || sensors.length === 0) {
    console.error("[ERROR] Dữ liệu cảm biến không hợp lệ hoặc trống:", sensors);
    return;
  }

  sensors.forEach((sensor) => {
    if (
      !sensor.location ||
      sensor.temperature == null ||
      sensor.humidity == null
    ) {
      console.warn("[WARNING] Bỏ qua dữ liệu không hợp lệ:", sensor);
      return;
    }

    let roomDiv = document.querySelector(
      `.room[data-room="${sensor.location}"]`
    );
    if (roomDiv) {
      roomDiv.querySelector(
        ".temperature"
      ).innerText = `${sensor.temperature}°F`;
      roomDiv.querySelector(".humidity").innerText = `${sensor.humidity}%`;
    } else {
      console.warn(
        "[WARNING] Không tìm thấy phần tử HTML cho:",
        sensor.location
      );
    }
  });
}

// 🕒 Hàm chuyển đổi timestamp thành dạng "X phút trước"
function getTimeAgo(timestamp) {
  let timeDiff = Math.floor((new Date() - new Date(timestamp)) / 1000);
  if (timeDiff < 60) return `${timeDiff} giây trước`;
  if (timeDiff < 3600) return `${Math.floor(timeDiff / 60)} phút trước`;
  if (timeDiff < 86400) return `${Math.floor(timeDiff / 3600)} giờ trước`;
  return `${Math.floor(timeDiff / 86400)} ngày trước`;
}

// 💡 Hàm chọn icon tương ứng với loại hoạt động
function getIcon(activity) {
  if (activity.includes("Touch Sensor")) return "💡"; // Icon đèn
  if (activity.includes("Web Control")) return "🌐"; // Icon web
  if (activity.includes("Fan")) return "🌀"; // Icon quạt
  return "🔹"; // Mặc định nếu không có icon phù hợp
}

// 🔄 Gọi API mỗi 5 giây để cập nhật log mới
setInterval(updateActivityLog, 5000);
updateActivityLog();
