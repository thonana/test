project(base)

set(HDF5_USE_STATIC_LIBRARIES ON)

find_package(spdlog REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C CXX)

add_library(base
    STATIC
        src/ui/dialog.cc
        src/ui/file.cc
        src/ui/form.cc
        src/ui/frame.cc
        src/ui/panel.cc
        src/camera.cc
        src/camera_peak.cc
        src/style.cc
        src/image.cc
)

target_include_directories(base
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ds>
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)

target_link_libraries(base
    PUBLIC
        general spdlog::spdlog
        general ${wxWidgets_LIBRARIES}
        general ${HDF5_LIBRARIES}
)

add_subdirectory(demo)
add_subdirectory(test)