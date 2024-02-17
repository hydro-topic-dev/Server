cmake_minimum_required (VERSION 3.8)

set(CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 20)

if (MSVC)
  add_compile_options("/utf-8")
endif()

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Server")
add_subdirectory ("LocalHelper")
add_subdirectory ("StandardProgram")
