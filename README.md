# Code Video

Short films made entirely by code: every frame rendered by a path tracer written for the film, every sound
synthesised, no AI generation, photos, textures, models or sample libraries. Each film lives in its own folder
with the finished video, the full source, and a README describing how it was made.

| Film | Folder | Length | Summary |
|---|---|---|---|
| Starlite Lanes | [`bowling-alley/`](bowling-alley/) | 42 s | First-person drone flight through a 1960s bowling alley, one continuous shot |

## Layout of a film folder

```
<film>/
├── README.md        how it was made: route, camera, engine, sound, timings, QC, known shortcomings
├── <film>.mp4       the finished video
├── preview.jpg      cover frame
├── src/             all source code
├── render.sh        one-command rebuild
└── *_report.txt     QC reports
```

Large intermediates (rendered frames, scratch files) live in `work/` and are git-ignored.
