file(GLOB_RECURSE alex_testing_source_files CONFIGURE_DEPENDS "*.cpp" "*.hpp")
list(FILTER alex_testing_source_files EXCLUDE REGEX "^.*\\.spec\\.[ch]pp$")
list(FILTER alex_testing_source_files EXCLUDE REGEX "^.*/main\\.cpp$")

add_executable(alex_testing ${alex_testing_source_files} main.cpp)
target_include_directories(alex_testing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(alex_testing_required_libs )
#  Arby::asioex Arby::util Arby::network Arby::config


target_link_libraries(alex_testing PRIVATE Arby::arby)
