# cl-heatmap
A tool for generating OpenStreetMap overlay heatmap [tiles](https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames)
using OpenCL.

## What it does
The tool takes a set of geographical (WGS84) points with assigned values and generates output tiles based on the selected
kernel.

### Linear approximation of the coordinate transformations
As the kernels are written to run in cartesian coordinate system (though generalizing for using WGS84 directly should be
fairly easy), we need to compute a
[tile](https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames)->WGS84->[cartesian](http://proj4.org/) transform.
Computing this for each point is however unnecessarily computationally intensive.

cl-heatmap solves this by sampling the transformation a few hundred times and then [using linear regression to make a 
transformation matrix](https://github.com/atalax/cl-heatmap/blob/master/src/coords.c#L57). The transformation is then
applied by the GPU for each of the 65536 output pixels. This transformation is relatively precise (at least for
the ETRS89-TM33 transformation which was the primary transformation used during development), under one pixel for
most zoom levels.

## Available kernels

### heatmap.cl
This kernel computes weighted average between the input points, the weights decaying with distance. Also properly checks
the distance and makes output points which are too far away from any input point transparent.

### tdoa.cl
This kernel computes [multilateration](https://en.wikipedia.org/wiki/Multilateration) error function. The tool was originally
developed for this application, however given the small number of points involved it outgrew it rapidly. See `examples/tdoa`
for example input data.

## Demo

A demo map of the `examples/safecast` can be found [here](https://clheatmap.atx.name). This map was generated
using about a 1 million points (centered around Prague) from the [Safecast dataset](http://blog.safecast.org/data/).

[![](http://clheatmap.atx.name/github1.png)](https://clheatmap.atx.name)
[![](http://clheatmap.atx.name/github2.png)](https://clheatmap.atx.name)

## Command line

```
Usage: cl-heatmap [OPTION...]

  -b, --boundaries=BOUNDARIES   Boundaries in WGS84 '50.12,14.23,51.23,15.33'
  -c, --clargs=CLARGS        OpenCL compiler arguments
  -d, --device=DEVICE        OpenCL device to use (-d 0.0)
  -f, --prefilter=PREFILTER  Do not pass a point to the kernel if it is further
                             than PREFILTER
  -i, --input=INPUT          Input JSON
  -k, --kernel=KERNEL        Kernel to use
  -m, --colormap=COLORMAP    Colormap to use, available: ["heat"]
  -o, --outdir=OUTDIR        Output directory
  -p, --projection=PROJECTION   Proj4 specification of the cartesian projection
                             (default="+init=epsg:3045")
  -z, --zoom=ZOOM            Zoomlevel
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

## TODO:

 - [ ] WGS84 great circle distance support
 - [ ] Write an actual heatmap kernel (where the points _add_ instead of weighted averaging)
 - [ ] Add some timing output
 - [ ] Add custom loadable color palletes
 - [ ] Support for color gradients with more than 256 colors (currently limited by the PNG output)

## Note
This was my first foray to the realm of GPU accelerated computing, so don't expect any sort of stellar performance.
