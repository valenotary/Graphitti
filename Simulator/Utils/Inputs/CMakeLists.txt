# Get all header and source files from current directory
file(GLOB Inputs ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.h *.cu)

# Create a library from the list of files gathered
add_library(Inputs ${Inputs})