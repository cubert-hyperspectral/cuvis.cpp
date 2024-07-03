cmake_minimum_required(VERSION 3.25.0)
include(GNUInstallDirs)
find_library(
    CuvisCpp_LIBRARY
    NAMES "cuvis"
    HINTS "/lib/cuvis" "$ENV{PROGRAMFILES}/Cuvis/bin")

find_path(CuvisCpp_INCLUDE_DIR
  NAMES cuvis.h
  HINTS "/usr/include/" "$ENV{PROGRAMFILES}/Cuvis/sdk/cuvis_c")

include(FindPackageHandleStandardArgs)

mark_as_advanced(CuvisCpp_LIBRARY CuvisCpp_INCLUDE_DIR)

if(NOT CuvisCpp_LIBRARY)
	message(FATAL_ERROR "Could not locate cuvis library")
else()
  if(NOT TARGET cuvis::cpp)
	  add_library(cuvis::cpp STATIC IMPORTED)
	  
	  #simmilar to the c library, we use the cuvis.dll, howver we add 
	  #the cpp interface file as well as force the utilizing target to switch to c++17
	  set_target_properties(
		cuvis::cpp
		PROPERTIES
		  INTERFACE_INCLUDE_DIRECTORIES "${CuvisCpp_INCLUDE_DIR};${CMAKE_CURRENT_LIST_DIR}/interface;${CMAKE_CURRENT_LIST_DIR}/auxiliary/include"
		  IMPORTED_LOCATION ${CuvisCpp_LIBRARY})
		target_compile_features(cuvis::cpp INTERFACE cxx_std_17)
 
	  find_package(Doxygen)
	  option(DOXYGEN_BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" TRUE)
			
	  if(DOXYGEN_BUILD_DOCUMENTATION)
		  if(NOT DOXYGEN_FOUND)
			 message(status "Doxygen is needed to build the documentation.")
	      else()
			  if(NOT TARGET cuvis_cpp_doxygen)
			  
				  file(MAKE_DIRECTORY  ${CMAKE_BINARY_DIR}/doc )
					
					set(MAINPAGE ${CMAKE_CURRENT_LIST_DIR}/doc/mainpage.hpp)
					set(CPPSDK_INTERFACE_DIR ${CMAKE_CURRENT_LIST_DIR}/interface)
					set(CPPSDK_AUX_DIR ${CMAKE_CURRENT_LIST_DIR}/auxiliary/include)
					set(DOXYGEN_IN ${CMAKE_CURRENT_LIST_DIR}/doxygen/doxyfile_iface.in)
					set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxyfile_iface)

					# request to configure the file
					configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)	
					
					add_custom_target(cuvis_cpp_doxygen 
					COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxyfile_iface
									  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
									  COMMENT "Generating API documentation with Doxygen (iface)"
									  VERBATIM
					)
					
					
			  
				endif()
		  endif()
		  
		  
		add_dependencies(cuvis::cpp cuvis_cpp_doxygen)
	 
		endif()
		
		if (NOT WIN32)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
		endif()
		
  endif()		
	
  # Function to extract version from DLL
  function(get_library_version LIB_PATH OUTPUT_VARIABLE)
    set(GET_VERSION_SOURCE "${CMAKE_CURRENT_LIST_DIR}/helper/get_version.cpp")

	try_run(
		GET_VERSION_RUN_RESULT GET_VERSION_COMPILE_RESULT
        ${CMAKE_BINARY_DIR}/try_compile
        SOURCES ${GET_VERSION_SOURCE}
		RUN_OUTPUT_VARIABLE  ${OUTPUT_VARIABLE}
		LINK_LIBRARIES cuvis::cpp
		COMPILE_OUTPUT_VARIABLE GET_VERSION_COMPILE_INFO
        CMAKE_FLAGS
            -DINCLUDE_DIRECTORIES=${CuvisCpp_INCLUDE_DIR}
            -DLINK_LIBRARIES=${CuvisCpp_LIBRARY}
		)
		if (NOT GET_VERSION_COMPILE_RESULT)
			message("Failed to compile and run get_version_executable")
			message(FATAL_ERROR "${GET_VERSION_COMPILE_INFO}")
		endif()
		set(${OUTPUT_VARIABLE} "${${OUTPUT_VARIABLE}}" CACHE INTERNAL "${OUTPUT_VARIABLE}")

  endfunction()

  # Get the version of the library
  get_library_version("${CuvisCpp_LIBRARY}" LIB_VERSION)

  # Parse the version components
  string(REGEX MATCH "v\\. ([0-9]+)\\.([0-9]+)\\.([0-9]+)" LIB_VERSION_MATCH "${LIB_VERSION}")
  # Check if the version was successfully extracted
  if (NOT LIB_VERSION_MATCH)
      message(FATAL_ERROR "Failed to extract version from the library")
  endif()

  set(lib_version_MAJOR ${CMAKE_MATCH_1})
  set(lib_version_MINOR ${CMAKE_MATCH_2})
  set(lib_version_PATCH ${CMAKE_MATCH_3})

  # Check the library version against the required version
  set(CUVIS_VERSION_STRING "${lib_version_MAJOR}.${lib_version_MINOR}.${lib_version_PATCH}")

  # Provide configuration information for find_package(Cuvis)

  set(CuvisCpp_FOUND TRUE CACHE INTERNAL "")
  set(CuvisCpp_VERSION "${lib_version_MAJOR}.${lib_version_MINOR}.${lib_version_PATCH}" CACHE INTERNAL "")
  set(CuvisCpp_INCLUDE_DIRS "${CuvisCpp_INCLUDE_DIR}" CACHE INTERNAL "")
  set(CuvisCpp_LIBRARIES "cuvis::cpp" CACHE INTERNAL "")

  find_package_handle_standard_args(CuvisCpp
		REQUIRED_VARS CuvisCpp_LIBRARY CuvisCpp_INCLUDE_DIR
		VERSION_VAR CuvisCpp_VERSION)

endif()
