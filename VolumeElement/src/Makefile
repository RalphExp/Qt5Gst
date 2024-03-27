.PHONY: all

BIN=VolumeElement
SHELL = /bin/bash
CC = g++
CFLAGS = -g -std=c++17 $(shell echo `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0`)
LDFLAGS = $(shell echo `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0`)

all: $(BIN)

VolumeElement: volume.o
	$(CC) -o$@ $< $(LDFLAGS)

%.o: %.cpp
	$(CC) -o$@ -c $< $(CFLAGS) 

clean:
	@echo cleaning...
	rm -rf $(BIN)
	rm -rf *.dSYM;
	rm -rf *.o  
	@echo done
	
