#!/bin/bash
# One command to rebuild the film from source (fixed seeds; resumable — finished frames are skipped, and a
# resumed run first re-renders a few unsaved frames so the temporal accumulation is warm again).
# Needs: macOS on Apple Silicon (Metal), clang++, ffmpeg, python3 with numpy / scipy / Pillow.
set -e
cd "$(dirname "$0")"
mkdir -p work/frames
./build.sh                                                                   # 1. compile the renderer host
./dronecam --timeline work/timeline.json                                     # 2. event timeline for the soundtrack
[ -f work/meter.txt ] || ./dronecam --w 320 --h 180 --spp 6 --bounce 3 --from 0 --to 1259 --step 2 --meter work/meter.txt   # 3. metering pre-pass
python3 src/expo.py work/meter.txt work/expo.txt 1260 30                     #    -> lagging auto-exposure curve
./dronecam --w 1920 --h 1080 --spp 16 --bounce 3 --from 0 --to 1259 --expo work/expo.txt --out work/frames   # 4. the frames
python3 src/audio.py work/timeline.json work/audio.wav                       # 5. synthesise the soundtrack
ffmpeg -y -loglevel error -framerate 30 -i work/frames/f_%05d.png -i work/audio.wav \
  -c:v libx264 -preset slow -crf 19 -pix_fmt yuv420p -colorspace bt709 -color_primaries bt709 -color_trc bt709 \
  -c:a aac -b:a 256k -shortest -movflags +faststart starlite_lanes_fpv.mp4   # 6. encode + mux
python3 src/qc.py starlite_lanes_fpv.mp4 | tee qc_report.txt                 # 7. bad-frame check
