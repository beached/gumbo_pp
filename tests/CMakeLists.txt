cmake_policy(SET CMP0065 NEW)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#add_subdirectory(extern)

add_library( gumbo-pp_test INTERFACE )
if (DAW_USE_PACKAGE_MANAGEMENT)
    target_link_libraries(gumbo-pp_test INTERFACE ${COMPILER_SPECIFIC_LIBS})
else ()
    target_link_libraries(gumbo-pp_test INTERFACE daw::daw-header-libraries daw::daw-gumbo-pp ${COMPILER_SPECIFIC_LIBS})
endif ()

target_include_directories(gumbo-pp_test INTERFACE include/)
target_compile_features(gumbo-pp_test INTERFACE cxx_std_17)

add_custom_target(${PROJECT_NAME}_full)

add_executable( test_bin src/test.cpp )
target_link_libraries( test_bin gumbo-pp_test )
add_test( test_test test_bin )

add_executable( table_scrape src/table_scrape.cpp )
target_link_libraries( table_scrape gumbo-pp_test )
add_test( table_scrape_test table_scrape )
