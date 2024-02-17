add_executable (StandardProgram "standard_program.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET LocalHelper PROPERTY CXX_STANDARD 20)
endif()
