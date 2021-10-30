cmake_policy(SET CMP0065 NEW)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#add_subdirectory(extern)

if (DAW_USE_PACKAGE_MANAGEMENT)
    target_link_libraries(gumbo_pp_test PUBLIC ${COMPILER_SPECIFIC_LIBS})
else ()
    target_link_libraries(gumbo_pp_test PUBLIC daw::daw-header-libraries daw::gumbo_pp ${COMPILER_SPECIFIC_LIBS})
endif ()

target_include_directories(gumbo_pp_test PUBLIC include/)
target_compile_features(gumbo_pp_test PUBLIC cxx_std_17)

add_custom_target(${PROJECT_NAME}_full)

