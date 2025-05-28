cd /home/pi/oled
g++ -g oled.c ssd1306_i2c.c ./MAX17043/MAX17043.c -o oled -lwiringPi -rdynamic
