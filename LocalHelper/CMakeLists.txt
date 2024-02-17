add_executable (LocalHelper "server.cpp" "server.h" "filesystem.h" "filesystem.cpp" "test.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET LocalHelper PROPERTY CXX_STANDARD 20)
endif()
