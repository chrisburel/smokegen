cmake_minimum_required(VERSION 2.8.11)
project(smokegenerator)

find_package(Qt5 COMPONENTS Core Xml REQUIRED)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
link_directories(${LLVM_LIBRARY_DIRS})

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(AddLLVM)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake )
set(SMOKE_VERSION_MAJOR 3)
set(SMOKE_VERSION_MINOR 0)
set(SMOKE_VERSION_PATCH 0)
set(SMOKE_VERSION ${SMOKE_VERSION_MAJOR}.${SMOKE_VERSION_MINOR}.${SMOKE_VERSION_PATCH})


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

if (NOT LIB_INSTALL_DIR)
    set (LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
endif (NOT LIB_INSTALL_DIR)

include_directories(${CMAKE_BINARY_DIR})

set(generator_SRC
    astconsumer.cpp
    astvisitor.cpp
    frontendaction.cpp
    defaultargvisitor.cpp
    main.cpp
    options.cpp
    ppcallbacks.cpp
    type.cpp
)

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_MACOSX_RPATH 1)

# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in config.h @ONLY )

# Embed the clang header into the binary:
file(GLOB BUILTINS_HEADERS "${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_PACKAGE_VERSION}/include/*.h")
foreach(BUILTIN_HEADER ${BUILTINS_HEADERS})
    file(READ ${BUILTIN_HEADER} BINARY_DATA HEX)
    string(REGEX REPLACE "(..)" "\\\\x\\1" BINARY_DATA "${BINARY_DATA}")
    string(REPLACE "${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_VERSION}/include/" "/builtins/" FN "${BUILTIN_HEADER}"  )
    set(EMBEDDED_DATA "${EMBEDDED_DATA} { \"${FN}\" , \"${BINARY_DATA}\" } , ")
endforeach()

configure_file(embedded_includes.h.in embedded_includes.h)

add_executable(smokegen ${generator_SRC})

llvm_map_components_to_libnames(
    llvm_libs
    bitreader
    core
    mc
    mcparser
    option
    profiledata
    support
)

target_link_libraries(smokegen
    Qt5::Core
    Qt5::Xml
    clangAST
    clangAnalysis
    clangBasic
    clangDriver
    clangEdit
    clangFrontend
    clangFrontendTool
    clangLex
    clangParse
    clangSema
    clangEdit
    clangASTMatchers
    clangRewrite
    clangRewriteFrontend
    clangStaticAnalyzerFrontend
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    clangSerialization
    clangToolingCore
    clangTooling
    ${llvm_libs}
)

set_target_properties(smokegen PROPERTIES COMPILE_DEFINITIONS __GENERATOR_BUILDING ENABLE_EXPORTS TRUE)
set(LLVM_REQUIRES_RTTI ON)
llvm_update_compile_flags(smokegen)
target_compile_features(smokegen PRIVATE cxx_nullptr)
set_property(
    SOURCE
        astconsumer.cpp
        frontendaction.cpp
        main.cpp
        ppcallbacks.cpp
    APPEND
    PROPERTY COMPILE_FLAGS -fno-rtti
)

install(TARGETS smokegen LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

if (WIN32)
    target_link_libraries(smokegen
        version
    )

    # Copy Qt dlls to the build directory, so the generator works without
    # installation
    add_custom_command(
        TARGET smokegen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt5::Core>
            $<TARGET_FILE:Qt5::Xml>
            $<TARGET_FILE_DIR:smokegen>
    )

	# Realign the stack, for compatibility with an older ABI
	if(CMAKE_COMPILER_IS_GNUCXX)
		set_target_properties(smokegen PROPERTIES COMPILE_FLAGS -mstackrealign)
	endif()

    # Get rid of the "lib" prefix on archives/DLLs in Windows.
    set_target_properties(smokegen PROPERTIES PREFIX "" IMPORT_PREFIX "")

    # Make sure the dependent Qt dlls are copied to the smokegen executable
    # directory
    install(FILES
        $<TARGET_FILE:Qt5::Core>
        $<TARGET_FILE:Qt5::Xml>
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )
endif (WIN32)

install(FILES options.h type.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/smokegen)
install(FILES smoke.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include )

add_subdirectory(cmake)
add_subdirectory(generators)
add_subdirectory(smokeapi)
add_subdirectory(smokebase)
add_subdirectory(deptool)
