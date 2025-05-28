# OLED Monitor for Raspberry Pi

这是一个基于 Raspberry Pi 的 OLED 屏幕实时系统状态监视器，使用 I2C 接口驱动 0.96 英寸 SSD1306 显示屏，显示 IP 地址、CPU 负载、内存使用、电池状态、网络流量等信息。

## 特性

- 支持 `eth0` / `wlan0` 自动识别网络状态
- 低 CPU 占用，适合长时间运行
- 内存占用可控，支持通过 `freopen` 追踪文件更新
- 支持按键输入切换页面（可选）
- 支持电池电量读取（如接入 MAX17043）

## 使用的开源库

本程序基于以下开源组件构建：

- [wiringPi](http://wiringpi.com/)：用于 GPIO 和 I2C 操作
- [SSD1306 I2C 驱动](https://github.com/adafruit/Adafruit_SSD1306)（本地改写版）
- [MAX17043](https://github.com/lucadentella/ArduinoLib_MAX17043) 电池电量传感器驱动（移植为 C++）

## 编译方式

确保已安装 `wiringPi`：

```bash
sudo apt install wiringpi
```

使用以下命令编译：

```bash
g++ -g oled.cpp ssd1306_i2c.cpp ./MAX17043/MAX17043.cpp -o oled -lwiringPi -rdynamic
```

## 备注

- 程序使用 `freopen()` 追踪文件内容变化，避免重启时内存泄漏
- 若连接电池传感器 MAX17043，需确保 I2C 总线上地址设置正确

---

MIT License © 2025
