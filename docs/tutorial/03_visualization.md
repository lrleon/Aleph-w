# 03 — Visualization

Aleph-w can serialize a frame to many formats. The two styles are **one-shot
writers** (render a single frame) and **frame sinks** (stream every step to
disk).

## One-shot: a PNG of the current frame

`write_png(out, frame, mapper)` takes a colour **mapper** `state -> RGB8`:

```cpp
#include <fstream>
#include <ca-io.H>     // RGB8
#include <ca-png.H>    // write_png

using namespace Aleph;
using namespace Aleph::CA;

void snapshot(const auto &frame, const std::string &path)
{
  std::ofstream out(path, std::ios::binary);
  write_png(out, frame, [](int v)
  {
    return v != 0 ? RGB8{120, 220, 140}   // live cell: green
                  : RGB8{18, 18, 24};      // dead cell: near-black
  });
}
```

`write_png` requires a rank-2 frame. The default overload
`write_png(out, frame)` uses a black-and-white binary mapper.

## One-shot: SVG

`write_svg` renders crisp vector output (great for papers and slides):

```cpp
#include <ca-svg.H>

std::ofstream out("frame.svg");
write_svg(out, eng.frame());
```

SVG also supports hexagonal and triangular lattices.

## Streaming: frame sinks

A frame sink writes one image per step as the simulation runs. The
[`ca_visualization_gallery_example`](../../Examples/ca_visualization_gallery_example.cc)
shows the full set; the essentials:

```cpp
#include <ca-png.H>     // Png_Frame_Sink
#include <ca-gif.H>     // Gif_Frame_Sink, GIF_Write_Options

// A palette maps cell state -> RGB8; index 0 is "dead".
std::vector<RGB8> palette = { {18,18,24}, {120,220,140} };

// One PNG per step, file name templated by {step}, 4x pixel scale.
Png_Frame_Sink png("out/png/life_{step}.png", palette, 4);

// A single animated GIF, 5 centiseconds per frame, looping.
Gif_Frame_Sink gif("out/life.gif", palette, GIF_Write_Options{5, true});

for (std::size_t t = 0; t <= steps; ++t)
{
  png.accept(eng.frame(), t);
  gif.accept(eng.frame(), t);
  eng.step();
}
gif.finish();          // flush the animation
```

## Available backends

| Header | Output | Notes |
|--------|--------|-------|
| `ca-png.H` | PNG | `write_png`, `Png_Frame_Sink` |
| `ca-gif.H` | animated GIF | `write_gif`, `Gif_Frame_Sink` |
| `ca-svg.H` | SVG | rect / hex / triangular |
| `ca-ascii.H` | text | `render_ascii` (chapter 01) |
| `ca-npy.H` | NumPy `.npy` | for Python post-processing |
| `ca-vtk.H` | VTK | ParaView / scientific viz |
| `ca-html-player.H` | HTML | self-contained playable page |
| `ca-x11-viewer.H` | live X11 window | optional, POSIX only |
| `ca-ffmpeg-sink.H` | MP4 via ffmpeg | optional |

## Next

[Chapter 04 — Observability](04_observability.md): measure what the simulation
is doing while it runs.