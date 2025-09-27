# Important: This is a "null backend" application, with no visible output or interaction!
# This is used for testing purpose and continuous integration, and has little use for end-user.
#

add_library(example_null STATIC)

target_sources(example_null PRIVATE main.cpp)

target_link_libraries(example_null PRIVATE imgui::imgui)
