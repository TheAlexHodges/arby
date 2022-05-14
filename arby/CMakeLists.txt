file(GLOB_RECURSE arby_source_files CONFIGURE_DEPENDS "*.cpp" "*.hpp")
list(FILTER arby_source_files EXCLUDE REGEX "^.*\\.spec\\.[ch]pp$")
list(FILTER arby_source_files EXCLUDE REGEX "^.*/main\\.cpp$")

#message(FATAL_ERROR "${arby_source_files}")
add_library(arby ${arby_source_files})
add_library(Arby::arby ALIAS arby)
target_include_directories(arby PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(arby_required_libs Boost::system Boost::thread Boost::filesystem
        Threads::Threads
        OpenSSL::SSL OpenSSL::Crypto
        Arby::asioex Arby::util Arby::network Arby::config)

target_link_libraries(arby PUBLIC ${arby_required_libs})


add_executable(arby_exec main.cpp)
target_link_libraries(arby_exec PRIVATE Arby::arby)

configure_file(testing/test_environment.spec.cpp.in testing/test_environment.spec.cpp)
file(GLOB_RECURSE arby_test_sources CONFIGURE_DEPENDS "*.spec.cpp" "*.spec.hpp")
add_executable(arby_test ${arby_test_sources} ${arby_source_files} testing/test_environment.spec.cpp)
target_link_libraries(arby_test PUBLIC ${arby_required_libs} doctest::doctest)
target_include_directories(arby_test PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME arby_test COMMAND arby_test)

