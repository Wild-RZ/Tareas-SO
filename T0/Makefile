CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g

CAMERAS = camera_normal camera_short camera_silent camera_stubborn camera_crash

.PHONY: all clean cameras

all: fazwatch cameras

fazwatch: fazwatch.c protocol.h
	$(CC) $(CFLAGS) fazwatch.c -o fazwatch

cameras: $(CAMERAS)

camera_normal: cameras/camera_normal.c protocol.h
	$(CC) $(CFLAGS) cameras/camera_normal.c -o camera_normal

camera_short: cameras/camera_short.c protocol.h
	$(CC) $(CFLAGS) cameras/camera_short.c -o camera_short

camera_silent: cameras/camera_silent.c protocol.h
	$(CC) $(CFLAGS) cameras/camera_silent.c -o camera_silent

camera_stubborn: cameras/camera_stubborn.c protocol.h
	$(CC) $(CFLAGS) cameras/camera_stubborn.c -o camera_stubborn

camera_crash: cameras/camera_crash.c protocol.h
	$(CC) $(CFLAGS) cameras/camera_crash.c -o camera_crash

clean:
	rm -f fazwatch $(CAMERAS)
