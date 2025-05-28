cd /home/pi/oled
g++ -g oled.cpp ssd1306_i2c.cpp ./MAX17043/MAX17043.cpp -o oled -lwiringPi -rdynamic
