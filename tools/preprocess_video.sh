#!/usr/bin/env bash
set -euo pipefail

INPUT="${1:-video.webm}"
VIDEO_OUT="${2:-video.rgb565}"
AUDIO_OUT="${3:-audio.pcm}"

[[ -f "$INPUT" ]] || { echo "Arquivo nao encontrado: $INPUT" >&2; exit 1; }

ffmpeg -hide_banner -loglevel warning -y -i "$INPUT" -map 0:v:0 -an \
  -vf "fps=10,scale=320:240:force_original_aspect_ratio=increase,crop=320:240,format=rgb565le" \
  -f rawvideo "$VIDEO_OUT"

# 16 kHz estéreo cabe na PSRAM do ESP32-S3; durante a reprodução não há
# nenhuma leitura de áudio no SD, então vídeo e som não disputam o SPI.
ffmpeg -hide_banner -loglevel warning -y -i "$INPUT" -map 0:a:0 -vn \
  -af "aresample=async=1:first_pts=0" -ar 16000 -ac 2 \
  -c:a pcm_s16le -f s16le "$AUDIO_OUT"
