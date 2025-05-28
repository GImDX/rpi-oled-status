// oled.cpp
// 导入wiringPi/I2C库 
#include <wiringPi.h>
#include <wiringPiI2C.h>

// 导入oled显示屏库
#include "ssd1306_i2c.h"

#include "MAX17043/MAX17043.h"

// 导入文件控制函数库
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
// 读取IP库
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
// 读取磁盘库
#include <sys/vfs.h>
#include <unistd.h>

#include <sysexits.h>
#include <glob.h>
#include <linux/input.h>
#include <fcntl.h>

#define MAX_SIZE 32
#define CAPACITY_COMPENSATE 0
#define AUTO_REFRESH 1000

int kbd_fd = -1;


void init_keyboard_device(void);
int keycode_of_key_being_pressed(void);

int main(void)
{
        char LAN_path[2][2 * MAX_SIZE];
        FILE *fd_temp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
        FILE *loadavg = fopen("/proc/loadavg", "r");
        FILE *stat = fopen("/proc/stat", "r");
        FILE *tx_bytes = NULL;
        FILE *rx_bytes = NULL;
        FILE *config = fopen("/home/pi/oled/oled.config", "r");
        short flag_start = 0xff;

        // get system usage / info
        struct sysinfo sys_info;
        struct statfs disk_info;
        sysinfo(&sys_info);
        unsigned long ProgTime_Start = sys_info.uptime;

        char RamInfo[MAX_SIZE];
        unsigned long totalRam;
        unsigned long freeRam;

        char IPInfo[MAX_SIZE];
        char addressBuffer[INET_ADDRSTRLEN];

        float temp = 0;
        char CPUInfo[MAX_SIZE];
        char TasksInfo[MAX_SIZE];
        char CPUInfo_tmp[2 * MAX_SIZE + 3];
        char CPUTemp[2 * MAX_SIZE + 3 + 17];

        char DiskInfo[MAX_SIZE];
        unsigned long long totalBlocks;
        unsigned long long totalSize;
        size_t mbTotalsize;
        unsigned long long freeDisk;
        size_t mbFreedisk;

        char UptimeInfo_d[MAX_SIZE];
        char UptimeInfo_h[MAX_SIZE];
        char UptimeInfo_m[MAX_SIZE];
        char UptimeInfo_s[MAX_SIZE];
        int d;
        int h;
        int m;
        int s;

        float CPULoad[4];
        int LoadBarLen[4];
        char CPULoadInfo[4][MAX_SIZE];
        int i;
        unsigned long work_over_period[4];
        unsigned long total_over_period[4];
        unsigned long jiffies[7];

        char SysTime[MAX_SIZE];
        char wday[MAX_SIZE] = "";
        char mon_mday[MAX_SIZE] = "";
        char hour_min_sec[MAX_SIZE] = "";
        char year[MAX_SIZE] = "";
        time_t timep;

        char ProgTimeInfo_d[MAX_SIZE];
        char ProgTimeInfo_h[MAX_SIZE];
        char ProgTimeInfo_m[MAX_SIZE];
        char ProgTimeInfo_s[MAX_SIZE];
        int ProgTime_Now = sys_info.uptime - ProgTime_Start;
        int ProgTime_d;
        int ProgTime_h;
        int ProgTime_m;
        int ProgTime_s;

        struct timespec last_tv;
        struct timespec current_tv;
        last_tv.tv_sec = 0;
        float delta_msec = 0;
        unsigned long long last_tx_bytes = 0;
        unsigned long long current_tx_bytes = 0;
        unsigned long long delta_tx_bytes = 0;
        unsigned long long last_rx_bytes = 0;
        unsigned long long current_rx_bytes = 0;
        unsigned long long delta_rx_bytes = 0;
        float network_speed[2] = {0, 0};
        char NetworkInfo[3][MAX_SIZE];

        unsigned long total_jiffies_1[4];
        unsigned long work_jiffies_1[4];
        unsigned long total_jiffies_2[4];
        unsigned long work_jiffies_2[4];

        struct ifaddrs *ifAddrStruct = NULL;
        struct ifaddrs *ifa_copy = NULL;
        void *tmpAddrPtr = NULL;

        MAX17043 max17043;
        float BusVoltage_V;
        float Percent;
        char BusVoltage_VInfo[MAX_SIZE];
        char PercentInfo[MAX_SIZE];
        char BatteryIcon[] = "-[                 ]-";

        unsigned int sw = 0x10;
        int kbd = -1;
        int key = -1;
        unsigned int config_sw = 0xFF;

        ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, FALSE);
        max17043.init(MAX17043_ADDRESS);
        max17043.reset();
        max17043.quickStart();

        int refresh_count = 0;

        // ssd1306_display();
        // ssd1306_clearDisplay();
        // delay(500);

        // Set stdout unbuffered
        setvbuf(stdout, NULL, _IONBF, 0);

        printf("Running!\n");

        while (1)
        {
                // 读取系统信息
                sysinfo(&sys_info);
                // 清除屏幕内容
                ssd1306_clearDisplay();

                // 运行内存占用率，剩余/总内存
                totalRam = sys_info.totalram >> 8;
                freeRam = sys_info.freeram >> 8;
                sprintf(RamInfo, "R:    %ld/%ldMB", freeRam, totalRam);

                // 获取IP地址
                getifaddrs(&ifAddrStruct);
                ifa_copy = ifAddrStruct;  // 保留原始指针
                while (ifAddrStruct != NULL)
                {
                        if (ifAddrStruct->ifa_addr->sa_family == AF_INET)
                        { // check it is IP4 is a valid IP4 Address
                                tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
                                inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

                                if (strcmp(ifAddrStruct->ifa_name, "eth0") == 0)
                                {
                                        sprintf(IPInfo, "E:%15s", addressBuffer);
                                        strcpy(LAN_path[0], "/sys/class/net/eth0/statistics/tx_bytes");
                                        strcpy(LAN_path[1], "/sys/class/net/eth0/statistics/rx_bytes");
                                        sprintf(NetworkInfo[2], "ETH0");
                                        if (!tx_bytes && !rx_bytes && (flag_start & 0x01))
                                        {
                                                flag_start &= 0xfe;
                                                tx_bytes = fopen(LAN_path[0], "r");
                                                rx_bytes = fopen(LAN_path[1], "r");
                                        }
                                        else
                                        {
                                                freopen(LAN_path[0], "r", tx_bytes);
                                                freopen(LAN_path[1], "r", rx_bytes);
                                        }
                                        break;

                                }
                                else if (strcmp(ifAddrStruct->ifa_name, "wlan0") == 0)
                                {
                                        sprintf(IPInfo, "W:%15s", addressBuffer);
                                        strcpy(LAN_path[0], "/sys/class/net/wlan0/statistics/tx_bytes");
                                        strcpy(LAN_path[1], "/sys/class/net/wlan0/statistics/rx_bytes");
                                        sprintf(NetworkInfo[2], "WLAN0");
                                        if (!tx_bytes && !rx_bytes && (flag_start & 0x01))
                                        {
                                                flag_start &= 0xfe;
                                                tx_bytes = fopen(LAN_path[0], "r");
                                                rx_bytes = fopen(LAN_path[1], "r");
                                        }
                                        else
                                        {
                                                freopen(LAN_path[0], "r", tx_bytes);
                                                freopen(LAN_path[1], "r", rx_bytes);
                                        }
                                        break;

                                }
                                else
                                {
                                        sprintf(IPInfo, "X:%15s", "Disconnected");
                                        sprintf(NetworkInfo[2], "Disconnected");
                                        if (!flag_start & 0x01)
                                        {
                                                freopen(LAN_path[0], "r", tx_bytes);
                                                freopen(LAN_path[1], "r", rx_bytes);
                                        }
                                }
                        }
                        ifAddrStruct = ifAddrStruct->ifa_next;
                }
                if (ifa_copy != NULL) {
                        freeifaddrs(ifa_copy);
                }

                // 读取CPU温度
                freopen("/sys/class/thermal/thermal_zone0/temp", "r", fd_temp);
                fscanf(fd_temp, "%f", &temp);
                temp = temp / 1000.0;

                // 进程信息
                freopen("/proc/loadavg", "r", loadavg);
                fscanf(loadavg, "%*[^ ] %*[^ ] %*[^ ] %s", TasksInfo);

                // CPU信息
                sprintf(CPUTemp, "%.1fC", temp);
                sprintf(CPUInfo_tmp, "%s T:%s", TasksInfo, CPUTemp);
                sprintf(CPUInfo, "P:%15s", CPUInfo_tmp);

                // 读取磁盘空间，剩余/总空间
                statfs("/", &disk_info);
                totalBlocks = disk_info.f_bsize;
                totalSize = totalBlocks * disk_info.f_blocks;
                mbTotalsize = totalSize >> 20;
                freeDisk = disk_info.f_bfree * totalBlocks;
                mbFreedisk = freeDisk >> 20;
                sprintf(DiskInfo, "D:  %d/%dMB", mbFreedisk, mbTotalsize);

                // 读取运行时间
                d = sys_info.uptime / 86400;
                h = sys_info.uptime % 86400 / 3600;
                m = sys_info.uptime % 86400 % 3600 / 60;
                s = sys_info.uptime % 86400 % 3600 % 60;
                sprintf(UptimeInfo_d, "|%02xd", d);
                sprintf(UptimeInfo_h, "|%02dh", h);
                sprintf(UptimeInfo_m, "|%02dm", m);
                sprintf(UptimeInfo_s, "|%02ds", s);

                // 读取CPU利用率
                freopen("/proc/stat", "r", stat);
                fscanf(stat, "%*[^\n]\n");
                if (flag_start & 0x02)
                {
                        for (i = 0; i <= 3; i++)
                        {
                                fscanf(stat, "%*[^ ] %ld %ld %ld %ld %ld %ld %ld %*[^\n]\n", &jiffies[0], &jiffies[1], &jiffies[2], &jiffies[3], &jiffies[4], &jiffies[5], &jiffies[6]);
                                total_jiffies_1[i] = jiffies[0] + jiffies[1] + jiffies[2] + jiffies[3] + jiffies[4] + jiffies[5] + jiffies[6];
                                work_jiffies_1[i] = jiffies[0] + jiffies[1] + jiffies[2];
                        }
                        flag_start &= 0xfd;
                }
                else
                {
                        for (i = 0; i <= 3; i++)
                        {
                                fscanf(stat, "%*[^ ] %ld %ld %ld %ld %ld %ld %ld %*[^\n]\n", &jiffies[0], &jiffies[1], &jiffies[2], &jiffies[3], &jiffies[4], &jiffies[5], &jiffies[6]);
                                total_jiffies_2[i] = jiffies[0] + jiffies[1] + jiffies[2] + jiffies[3] + jiffies[4] + jiffies[5] + jiffies[6];
                                work_jiffies_2[i] = jiffies[0] + jiffies[1] + jiffies[2];

                                work_over_period[i] = work_jiffies_2[i] - work_jiffies_1[i];
                                total_over_period[i] = total_jiffies_2[i] - total_jiffies_1[i];
                                CPULoad[i] = (float)work_over_period[i] / (float)total_over_period[i] * 100;
                                if (CPULoad[i] > 99.9)
                                {
                                        CPULoad[i] = 99.9;
                                }

                                work_jiffies_1[i] = work_jiffies_2[i];
                                total_jiffies_1[i] = total_jiffies_2[i];

                                LoadBarLen[i] = (int)(0.68 * CPULoad[i]);
                                sprintf(CPULoadInfo[i], "CPU%1d            %04.1f%%", i, CPULoad[i]);
                        }
                }

                // 读取系统时间
                time(&timep);
                sprintf(SysTime, "%s", asctime(localtime(&timep)));
                strncpy(wday, SysTime + 0, 3);
                strncpy(mon_mday, SysTime + 4, 6);
                strncpy(hour_min_sec, SysTime + 11, 8);
                strncpy(year, SysTime + 20, 4);

                // 程序运行时间
                ProgTime_Now = sys_info.uptime - ProgTime_Start;
                ProgTime_d = ProgTime_Now / 86400;
                ProgTime_h = ProgTime_Now % 86400 / 3600;
                ProgTime_m = ProgTime_Now % 86400 % 3600 / 60;
                ProgTime_s = ProgTime_Now % 86400 % 3600 % 60;
                sprintf(ProgTimeInfo_d, "|%02dd", ProgTime_d);
                sprintf(ProgTimeInfo_h, "|%02dh", ProgTime_h);
                sprintf(ProgTimeInfo_m, "|%02dm", ProgTime_m);
                sprintf(ProgTimeInfo_s, "|%02ds", ProgTime_s);

                // 绘图
                if (sw == 0x10)
                {
                        // 第一屏，下同
                        ssd1306_drawText(0, 0, CPUInfo);
                        ssd1306_drawText(0, 8, RamInfo);
                        ssd1306_drawText(0, 16, DiskInfo);
                        ssd1306_drawText(0, 24, IPInfo);
                        ssd1306_drawText(104, 0, UptimeInfo_d);
                        ssd1306_drawText(104, 8, UptimeInfo_h);
                        ssd1306_drawText(104, 16, UptimeInfo_m);
                        ssd1306_drawText(104, 24, UptimeInfo_s);
                        delay(AUTO_REFRESH);

                }
                if (sw == 0x08)
                {
                        ssd1306_drawText(0, 0, CPULoadInfo[0]);
                        ssd1306_fillRect(24, 0, LoadBarLen[0], 8, WHITE);
                        ssd1306_drawText(0, 8, CPULoadInfo[1]);
                        ssd1306_fillRect(24, 8, LoadBarLen[1], 8, WHITE);
                        ssd1306_drawText(0, 16, CPULoadInfo[2]);
                        ssd1306_fillRect(24, 16, LoadBarLen[2], 8, WHITE);
                        ssd1306_drawText(0, 24, CPULoadInfo[3]);
                        ssd1306_fillRect(24, 24, LoadBarLen[3], 8, WHITE);
                        delay(AUTO_REFRESH);
                }
                if (sw == 0x04)
                {
                        ssd1306_drawText(55, 0, wday);
                        ssd1306_drawText(46, 8, mon_mday);
                        ssd1306_drawText(40, 16, hour_min_sec);
                        ssd1306_drawText(52, 24, year);
                        ssd1306_drawText(104, 0, ProgTimeInfo_d);
                        ssd1306_drawText(104, 8, ProgTimeInfo_h);
                        ssd1306_drawText(104, 16, ProgTimeInfo_m);
                        ssd1306_drawText(104, 24, ProgTimeInfo_s);
                        delay(AUTO_REFRESH);

                }
                if (sw == 0x02)
                {
                        // 网络收发速率
                        if (!last_tv.tv_sec && tx_bytes)
                        {
                                clock_gettime(CLOCK_REALTIME, &last_tv);
                                fscanf(tx_bytes, "%lld", &last_tx_bytes);
                                fscanf(rx_bytes, "%lld", &last_rx_bytes);

                                delay(AUTO_REFRESH);

                                freopen(LAN_path[0], "r", tx_bytes);
                                freopen(LAN_path[1], "r", rx_bytes);
                                clock_gettime(CLOCK_REALTIME, &current_tv);
                                fscanf(tx_bytes, "%llu", &current_tx_bytes);
                                fscanf(rx_bytes, "%llu", &current_rx_bytes);

                                delta_msec = (current_tv.tv_sec - last_tv.tv_sec) * 1e3 + (current_tv.tv_nsec - last_tv.tv_nsec) / 1e6;
                                delta_tx_bytes = current_tx_bytes - last_tx_bytes;
                                delta_rx_bytes = current_rx_bytes - last_rx_bytes;

                                network_speed[0] = 8 * delta_tx_bytes / delta_msec;
                                network_speed[1] = 8 * delta_rx_bytes / delta_msec;
                                sprintf(NetworkInfo[0], "tx:%13.2f kbps", network_speed[0]);
                                sprintf(NetworkInfo[1], "rx:%13.2f kbps", network_speed[1]);

                                last_tv = current_tv;
                                last_tx_bytes = current_tx_bytes;
                                last_rx_bytes = current_rx_bytes;
                                delay(AUTO_REFRESH);
                        }
                        else if (tx_bytes)
                        {
                                clock_gettime(CLOCK_REALTIME, &current_tv);
                                freopen(LAN_path[0], "r", tx_bytes);
                                freopen(LAN_path[1], "r", rx_bytes);
                                fscanf(tx_bytes, "%llu", &current_tx_bytes);
                                fscanf(rx_bytes, "%llu", &current_rx_bytes);
                                delta_msec = (current_tv.tv_sec - last_tv.tv_sec) * 1e3 + (current_tv.tv_nsec - last_tv.tv_nsec) / 1e6;
                                delta_tx_bytes = current_tx_bytes - last_tx_bytes;
                                delta_rx_bytes = current_rx_bytes - last_rx_bytes;
                                network_speed[0] = 8 * delta_tx_bytes / delta_msec;
                                network_speed[1] = 8 * delta_rx_bytes / delta_msec;
                                sprintf(NetworkInfo[0], "tx:%13.2f kbps", network_speed[0]);
                                sprintf(NetworkInfo[1], "rx:%13.2f kbps", network_speed[1]);
                                last_tv = current_tv;
                                last_tx_bytes = current_tx_bytes;
                                last_rx_bytes = current_rx_bytes;

                                printf("%.1f\n", delta_msec);

                                delay(AUTO_REFRESH);
                        }
                        else
                        {
                                clock_gettime(CLOCK_REALTIME, &current_tv);
                                network_speed[0] = 0;
                                network_speed[1] = 0;
                                delta_msec = (current_tv.tv_sec - last_tv.tv_sec) * 1e3 + (current_tv.tv_nsec - last_tv.tv_nsec) / 1e6;
                                sprintf(NetworkInfo[0], "tx:%13.2f kbps", network_speed[0]);
                                sprintf(NetworkInfo[1], "rx:%13.2f kbps", network_speed[1]);
                                last_tv = current_tv;
                                last_tx_bytes = current_tx_bytes;
                                last_rx_bytes = current_rx_bytes;

                                printf("%.1f\n", delta_msec);

                                delay(AUTO_REFRESH);
                        }
                        ssd1306_drawText(0, 0, NetworkInfo[2]);
                        ssd1306_drawText(0, 8, NetworkInfo[0]);
                        ssd1306_drawText(0, 16, NetworkInfo[1]);
                }
                if (sw == 0x01)
                {
                        // UPS电量信息
                        BusVoltage_V = max17043.getVCell();
                        Percent = max17043.getSoC() + CAPACITY_COMPENSATE;

                        sprintf(BusVoltage_VInfo, "%12.3f  V", BusVoltage_V);
                        sprintf(PercentInfo, "%12.1f  %%", Percent);

                        ssd1306_drawText(0, 0, BusVoltage_VInfo);
                        ssd1306_drawText(0, 16, PercentInfo);
                        ssd1306_drawText(0, 24, BatteryIcon);
                        ssd1306_fillRect(13, 24, Percent * 1.02, 8, WHITE);
                        delay(AUTO_REFRESH);

                }

                // 循环切换显示
                if (sw >= 0x20)
                {
                        sw = 0x01;
                }
                if (sw <= 0x00)
                {
                        sw = 0x10;
                }

                //config文件控制切换显示，config == 0xff时无效
                freopen("/home/pi/oled/oled.config", "r", config);
                fscanf(config, "config_sw = %x", &config_sw);
                if (config_sw != 0xff)
                {
                        sw = config_sw;
                }



                // 显示切换 BT键盘无效
                if (kbd < 0)
                {
                        kbd = keycode_of_key_being_pressed();
                }
                else if (kbd > 0)
                {
                        key = kbd;
                        kbd = keycode_of_key_being_pressed();
                        if (kbd < 0)
                        {
                                if (key == 104)
                                {
                                        last_tv.tv_sec = 0;
                                        ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, TRUE);
                                        sw = sw << 1;
                                }
                                if (key == 109)
                                {
                                        last_tv.tv_sec = 0;
                                        ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, TRUE);
                                        sw = sw >> 1;
                                }
                        }
                }

                // 刷新显示
                ssd1306_display();

                refresh_count++;
                if (refresh_count >= 10)
                {
                        refresh_count = 0;
                        // sw = 0x01;
                        // ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, TRUE);
                        ssd1306_clearDisplay();
                }
        }
        if (fd_temp) fclose(fd_temp);
        if (loadavg) fclose(loadavg);
        if (stat) fclose(stat);
        if (tx_bytes) fclose(tx_bytes);
        if (rx_bytes) fclose(rx_bytes);
        if (config) fclose(config);
        if (kbd_fd >= 0) close(kbd_fd);
        return 0;
}

void init_keyboard_device() {
    glob_t kbddev;
    glob("/dev/input/by-path/*-kbd", 0, 0, &kbddev);
    if (kbddev.gl_pathc > 0) {
        kbd_fd = open(kbddev.gl_pathv[0], O_RDONLY | O_NONBLOCK);
        if (kbd_fd < 0) {
            perror("open keyboard failed");
        }
    }
    globfree(&kbddev);
}

int keycode_of_key_being_pressed() {
    if (kbd_fd < 0) return -1;

    char key_map[KEY_MAX / 8 + 1] = {0};
    if (ioctl(kbd_fd, EVIOCGKEY(sizeof(key_map)), key_map) < 0) {
        perror("ioctl failed");
        return -1;
    }

    for (int k = 0; k < KEY_MAX; k++) {
        if (key_map[k / 8] & (1 << (k % 8))) {
            return k;
        }
    }

    return -1;
}
