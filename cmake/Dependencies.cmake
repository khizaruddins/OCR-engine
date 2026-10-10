include(FetchContent)
include(FindPkgConfig)

# 1. PkgConfig dependencies
find_package(PkgConfig REQUIRED)

# Tesseract
pkg_check_modules(Tesseract REQUIRED tesseract)
message(STATUS "Found Tesseract: ${Tesseract_VERSION} (include: ${Tesseract_INCLUDE_DIRS}, libs: ${Tesseract_LIBRARIES})")

# Leptonica
pkg_check_modules(Leptonica REQUIRED lept)
message(STATUS "Found Leptonica: ${Leptonica_VERSION} (include: ${Leptonica_INCLUDE_DIRS}, libs: ${Leptonica_LIBRARIES})")

# Poppler C++
pkg_check_modules(PopplerCpp REQUIRED poppler-cpp)
message(STATUS "Found Poppler-Cpp: ${PopplerCpp_VERSION} (include: ${PopplerCpp_INCLUDE_DIRS}, libs: ${PopplerCpp_LIBRARIES})")

# 2. nlohmann_json
find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "Fetching nlohmann_json via FetchContent...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# 3. spdlog
find_package(spdlog 1.12 QUIET)
if(NOT spdlog_FOUND)
    message(STATUS "Fetching spdlog via FetchContent...")
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# 4. GoogleTest (for testing)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "Fetching GoogleTest via FetchContent...")
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# 5. OpenCV (core, imgproc, imgcodecs)
find_package(OpenCV QUIET COMPONENTS core imgproc imgcodecs)
if(OpenCV_FOUND)
    message(STATUS "Found OpenCV: ${OpenCV_VERSION}")
    add_library(edmo_opencv INTERFACE)
    target_link_libraries(edmo_opencv INTERFACE ${OpenCV_LIBS})
    target_include_directories(edmo_opencv INTERFACE ${OpenCV_INCLUDE_DIRS})
else()
    message(STATUS "Fetching minimal OpenCV via FetchContent...")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_LIST "core,imgproc,imgcodecs" CACHE STRING "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_PERF_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_apps OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_video OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_dnn OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_ml OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_photo OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_gapi OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_highgui OFF CACHE BOOL "" FORCE)
    set(WITH_CUDA OFF CACHE BOOL "" FORCE)
    set(WITH_IPP OFF CACHE BOOL "" FORCE)
    set(WITH_FFMPEG OFF CACHE BOOL "" FORCE)
    set(WITH_GSTREAMER OFF CACHE BOOL "" FORCE)
    set(WITH_GTK OFF CACHE BOOL "" FORCE)
    set(WITH_WEBP OFF CACHE BOOL "" FORCE)
    set(WITH_OPENEXR OFF CACHE BOOL "" FORCE)
    set(WITH_JASPER OFF CACHE BOOL "" FORCE)
    set(WITH_OPENJPEG OFF CACHE BOOL "" FORCE)
    set(WITH_1394 OFF CACHE BOOL "" FORCE)
    set(WITH_PROTOBUF OFF CACHE BOOL "" FORCE)
    set(WITH_QUIRC OFF CACHE BOOL "" FORCE)
    set(WITH_TIFF ON CACHE BOOL "" FORCE)
    set(WITH_PNG ON CACHE BOOL "" FORCE)
    set(WITH_JPEG ON CACHE BOOL "" FORCE)
    set(BUILD_PNG ON CACHE BOOL "" FORCE)
    set(BUILD_JPEG ON CACHE BOOL "" FORCE)
    set(BUILD_TIFF ON CACHE BOOL "" FORCE)
    set(BUILD_ZLIB ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        opencv
        GIT_REPOSITORY https://github.com/opencv/opencv.git
        GIT_TAG 4.9.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(opencv)

    add_library(edmo_opencv INTERFACE)
    target_link_libraries(edmo_opencv INTERFACE opencv_core opencv_imgproc opencv_imgcodecs)
    target_include_directories(edmo_opencv INTERFACE
        ${opencv_SOURCE_DIR}/include
        ${opencv_SOURCE_DIR}/modules/core/include
        ${opencv_SOURCE_DIR}/modules/imgproc/include
        ${opencv_SOURCE_DIR}/modules/imgcodecs/include
        ${opencv_BINARY_DIR}
        ${CMAKE_BINARY_DIR}
    )
endif()

# Consolidated dependency target for easy linkage
add_library(edmo_external_deps INTERFACE)
target_link_libraries(edmo_external_deps INTERFACE
    edmo_opencv
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    ${Tesseract_LIBRARIES}
    ${Leptonica_LIBRARIES}
    ${PopplerCpp_LIBRARIES}
)
target_include_directories(edmo_external_deps INTERFACE
    ${Tesseract_INCLUDE_DIRS}
    ${Leptonica_INCLUDE_DIRS}
    ${PopplerCpp_INCLUDE_DIRS}
)
