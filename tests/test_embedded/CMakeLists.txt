find_package(Catch 1.8.2 REQUIRED)

add_executable(test_embedded
  catch.cpp
  test_eval.cpp
  test_interpreter.cpp
)
target_include_directories(test_embedded PRIVATE ${CATCH_INCLUDE_DIR})
pybind11_enable_warnings(test_embedded)

if(NOT CMAKE_VERSION VERSION_LESS 3.0)
  target_link_libraries(test_embedded PRIVATE pybind11::embedded)
else()
  target_include_directories(test_embedded PRIVATE ${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
  target_compile_options(test_embedded PRIVATE ${PYBIND11_CPP_STANDARD})
  target_link_libraries(test_embedded PRIVATE ${PYTHON_LIBRARIES})
endif()

add_custom_target(cpptest COMMAND $<TARGET_FILE:test_embedded>
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
