cmake_minimum_required (VERSION 2.6)
project (cl-heatmap)

add_executable (cl-heatmap src/main.c src/colormaps.c src/utils.c src/coords.c)
target_link_libraries (cl-heatmap bsd OpenCL json-c gsl m png proj)
