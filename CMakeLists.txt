cmake_minimum_required(VERSION 3.22)

project(edmo-di-ocr
    VERSION 0.1.0
    DESCRIPTION "Education Document Intelligence C++ OCR Engine v0.1"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/CompilerWarnings.cmake)
include(cmake/Dependencies.cmake)

# Options
option(EDMO_BUILD_TESTING "Build test suite" ON)
option(EDMO_BUILD_BENCHMARKS "Build benchmarks" ON)
option(EDMO_BUILD_EXAMPLES "Build examples" ON)

# Core Library Sources
set(EDMO_OCR_CORE_SOURCES
    src/core/Document.cpp
    src/core/OcrConfig.cpp
    src/core/OcrEngine.cpp
    src/ingestion/PdfInspector.cpp
    src/ingestion/PageClassifier.cpp
    src/ingestion/ProcessingStrategy.cpp
    src/ingestion/NativeTextExtractor.cpp
    src/ingestion/PageComplexityAnalyzer.cpp
    src/ingestion/PreflightReport.cpp
    src/ingestion/DocumentIngestor.cpp
    src/preprocessing/ImagePreprocessor.cpp
    src/preprocessing/Deskew.cpp
    src/preprocessing/OrientationDetector.cpp
    src/preprocessing/Denoise.cpp
    src/preprocessing/Contrast.cpp
    src/preprocessing/Binarization.cpp
    src/preprocessing/Sharpen.cpp
    src/preprocessing/TableGridDetector.cpp
    src/pdf/PdfRenderer.cpp
    src/ocr/TesseractOcrProvider.cpp
    src/ocr/OcrProviderFactory.cpp
    src/output/JsonSerializer.cpp
    src/representation/SourceReference.cpp
    src/representation/TextNormalizer.cpp
    src/representation/NoiseReducer.cpp
    src/representation/ReadingOrder.cpp
    src/representation/LineGrouper.cpp
    src/representation/BlockGrouper.cpp
    src/representation/SectionDetector.cpp
    src/representation/TableDetector.cpp
    src/representation/TableReconstructor.cpp
    src/representation/MultiPageContinuity.cpp
    src/representation/ConfidenceAggregator.cpp
    src/representation/SemanticCompressor.cpp
    src/representation/EdrJsonSerializer.cpp
    src/representation/DocumentRepresentationEngine.cpp
    src/context/SemanticInventoryBuilder.cpp
    src/context/SafeDeduplicator.cpp
    src/context/TableCompactor.cpp
    src/context/TextCompactor.cpp
    src/context/SemanticChunker.cpp
    src/context/LossAnalyzer.cpp
    src/context/LlmContextSerializer.cpp
    src/context/LlmContextBuilder.cpp
    src/logging/Logger.cpp
    src/utils/CoordinateTransform.cpp
    src/utils/FileUtils.cpp
    src/utils/ImageUtils.cpp
    src/utils/Timer.cpp
)

add_library(edmo_ocr_core STATIC ${EDMO_OCR_CORE_SOURCES})
add_library(edmo::ocr::core ALIAS edmo_ocr_core)

target_include_directories(edmo_ocr_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(edmo_ocr_core
    PUBLIC
        edmo_external_deps
)

set_project_warnings(edmo_ocr_core)

# CLI Applications
add_executable(edmo-ocr apps/edmo-ocr/main.cpp)
target_link_libraries(edmo-ocr PRIVATE edmo_ocr_core)
set_project_warnings(edmo-ocr)
set_target_properties(edmo-ocr PROPERTIES OUTPUT_NAME "edmo-ocr")

add_executable(edmo-ocr-analyze apps/edmo-ocr-analyze/main.cpp)
target_link_libraries(edmo-ocr-analyze PRIVATE edmo_ocr_core)
set_project_warnings(edmo-ocr-analyze)
set_target_properties(edmo-ocr-analyze PROPERTIES OUTPUT_NAME "edmo-ocr-analyze")

# Examples
if(EDMO_BUILD_EXAMPLES)
    add_executable(edmo_ocr_example_basic examples/basic.cpp)
    target_link_libraries(edmo_ocr_example_basic PRIVATE edmo_ocr_core)
    set_project_warnings(edmo_ocr_example_basic)

    add_executable(edmo_ocr_example_batch examples/batch.cpp)
    target_link_libraries(edmo_ocr_example_batch PRIVATE edmo_ocr_core)
    set_project_warnings(edmo_ocr_example_batch)
endif()

# Benchmarks
if(EDMO_BUILD_BENCHMARKS)
    add_executable(edmo_ocr_benchmark benchmarks/OcrBenchmark.cpp)
    target_link_libraries(edmo_ocr_benchmark PRIVATE edmo_ocr_core)
    set_project_warnings(edmo_ocr_benchmark)
endif()

# Tests
if(EDMO_BUILD_TESTING)
    enable_testing()

    set(EDMO_TEST_SOURCES
        tests/unit/BoundingBoxTests.cpp
        tests/unit/OrientationTests.cpp
        tests/unit/PreprocessingTests.cpp
        tests/unit/ConfigTests.cpp
        tests/unit/SerializationTests.cpp
        tests/ingestion/IngestionTests.cpp
        tests/representation/ReadingOrderTests.cpp
        tests/representation/LineGroupingTests.cpp
        tests/representation/BlockGroupingTests.cpp
        tests/representation/SectionDetectionTests.cpp
        tests/representation/TableDetectionTests.cpp
        tests/representation/TableReconstructionTests.cpp
        tests/representation/TextNormalizationTests.cpp
        tests/representation/ConfidenceTests.cpp
        tests/representation/CompressionTests.cpp
        tests/representation/EdrSerializationTests.cpp
        tests/representation/GoldenTranscriptTests.cpp
        tests/context/InventoryTests.cpp
        tests/context/DeduplicationTests.cpp
        tests/context/TableCompactionTests.cpp
        tests/context/ConfidenceTests.cpp
        tests/context/ChunkingTests.cpp
        tests/context/CoverageReportTests.cpp
        tests/context/SevenPageTranscriptRegressionTests.cpp
        tests/context/BostonTranscriptRegressionTests.cpp
        tests/context/ArtifactManagementTests.cpp
        tests/integration/OcrEngineTests.cpp
        tests/integration/PdfTests.cpp
        tests/integration/TranscriptTests.cpp
    )

    add_executable(edmo_ocr_tests ${EDMO_TEST_SOURCES})
    target_link_libraries(edmo_ocr_tests
        PRIVATE
            edmo_ocr_core
            GTest::gtest_main
    )
    set_project_warnings(edmo_ocr_tests)
    target_compile_definitions(edmo_ocr_tests PRIVATE
        EDMO_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )

    include(GoogleTest)
    gtest_discover_tests(edmo_ocr_tests)
endif() # EDMO_BUILD_TESTING

# Installation rules
include(GNUInstallDirs)
install(TARGETS edmo_ocr_core edmo-ocr edmo-ocr-analyze
    EXPORT EdmoOcrTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

