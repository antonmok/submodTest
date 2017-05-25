# Minimum version for CMake
cmake_minimum_required(VERSION 2.8)

# Enable C++11
set (CMAKE_CXX_STANDARD 11)

# Project's name
project(pdfParser)

# Set the output folder
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# The following folder will be included
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/"
			"${CMAKE_CURRENT_SOURCE_DIR}/encoding"
			"${CMAKE_CURRENT_SOURCE_DIR}/parsing"
			"${CMAKE_CURRENT_SOURCE_DIR}/PDF-Writer/PDFWriter"
			"${CMAKE_CURRENT_SOURCE_DIR}/../tclap-1.2.1/include")


add_executable(pdfParser "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
                    	"${CMAKE_CURRENT_SOURCE_DIR}/parsing/collection-state.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/parsing/font-decoding.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/parsing/multi-dict-helper.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/parsing/pdf-interpreter.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/parsing/placements-extraction.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/CmdHandler.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/InsertLink.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/pdfParser.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/RetainMemory.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/SaveText.cpp")

#Link libraries
TARGET_LINK_LIBRARIES(pdfParser 
		"${CMAKE_CURRENT_SOURCE_DIR}/PDF-Writer/Build/PDFWriter/libPDFWriter.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/PDF-Writer/Build/FreeType/libFreeType.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/PDF-Writer/Build/LibAesgm/libLibAesgm.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/PDF-Writer/Build/LibJpeg/libLibJpeg.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/PDF-Writer/Build/LibTiff/libLibTiff.a"
		"${CMAKE_CURRENT_SOURCE_DIR}/PDF-Writer/Build/ZLib/libZlib.a")




