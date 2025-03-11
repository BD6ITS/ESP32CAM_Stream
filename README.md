# ESP32-CAM 视频流项目

基于 ESP32-CAM 的实时视频流项目，支持网页端查看摄像头画面并调节参数。

![8b76072994eb27518ea57a4f7255345](https://github.com/user-attachments/assets/2c1d4cc7-ac30-469a-85db-c6d9d2662166)

## 功能特点

- 实时视频流传输
- 可调节视频分辨率（QVGA/VGA/SVGA/XGA）
- 可调节视频质量（高清/标准/流畅）
- 支持浅色/深色主题切换
- 移动端自适应界面

## 硬件要求

- ESP32-CAM 开发板
- USB-TTL 下载器

## 使用说明

1. 上传代码到 ESP32-CAM
2. 设备会创建名为 "ESP32CAM_AP" 的 WiFi 热点
3. 连接该 WiFi（密码：12345678）
4. 浏览器访问 http://192.168.4.1
