# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\Qt-fighting-games_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Qt-fighting-games_autogen.dir\\ParseCache.txt"
  "Qt-fighting-games_autogen"
  )
endif()
