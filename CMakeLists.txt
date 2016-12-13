cmake_minimum_required (VERSION 2.6)
project (cl-heatmap)

find_package (GSL)

link_directories ("/opt/amdgpu-pro/lib/x86_64-linux-gnu/")

add_executable (cl-heatmap src/main.c src/colormaps.c src/utils.c src/coords.c)
target_link_libraries (cl-heatmap bsd OpenCL json-c "${GSL_LIBRARIES}" m png proj)

install (TARGETS cl-heatmap DESTINATION bin)
install (PROGRAMS utils/bgeigie.py DESTINATION share/${CMAKE_PROJECT_NAME})
install (DIRECTORY kernels DESTINATION share/${CMAKE_PROJECT_NAME})
install (DIRECTORY web DESTINATION share/${CMAKE_PROJECT_NAME}
		 PATTERN REGEX "tiles/[0-9].*" EXCLUDE)
