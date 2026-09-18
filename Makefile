.PHONY: run build upload monitor reset clean preprocess-video

BAUD_RATE ?= 115200
PORT ?=

run: upload monitor

build:
	pio run

upload:
	pio run -t upload

monitor:
	@if [ -n "$(PORT)" ]; then \
		pio device monitor -b $(BAUD_RATE) -p $(PORT); \
	else \
		pio device monitor -b $(BAUD_RATE); \
	fi

reset:
	pio run -t reset
	$(MAKE) monitor

clean:
	pio run -t clean

preprocess-video:
	bash tools/preprocess_video.sh video.webm video.rgb565 audio.pcm
