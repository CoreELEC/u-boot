# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

# SPDX-License-Identifier: Apache-2.0

########################################################
# Table of contents
########################################################
# 1. Amlogic-aware extensions
# 1.1. aml_*
# 1.2. aml_library_*
# 1.2.1 aml_interface_library_*
# 1.3. generate_inc_*
# 1.4. board_*
# 1.5. Misc.
# 2. Kconfig-aware extensions
# 2.1 *_if_kconfig
# 2.2 Misc
# 3. CMake-generic extensions
# 3.1. *_ifdef
# 3.2. *_ifndef
# 3.3. *_option compiler compatibility checks
# 3.4. Debugging CMake
# 3.5. File system management

########################################################
# 1. Amlogic-aware extensions
########################################################
# 1.1. aml_*
#
# The following methods are for modifying the CMake library[0] called
# "aml". aml is a catch-all CMake library for source files that
# can be built purely with the include paths, defines, and other
# compiler flags that all aml source files use.
# [0] https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html
#
# Example usage:
# aml_sources(
#   random_esp32.c
#   utils.c
# )
#
# Is short for:
# target_sources(aml PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/random_esp32.c
#   ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
# )

# https://cmake.org/cmake/help/latest/command/target_sources.html
function(aml_sources)
  foreach(arg ${ARGV})
    if(IS_ABSOLUTE ${arg})
      set(path ${arg})
    else()
      set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
    endif()

    if(IS_DIRECTORY ${path})
      message(FATAL_ERROR "aml_sources() was called on a directory")
    endif()

    target_sources(${TARGET_NAME} PRIVATE ${path})
  endforeach()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_include_directories.html
function(aml_include_directories)
  foreach(arg ${ARGV})
    if(IS_ABSOLUTE ${arg})
      set(path ${arg})
    else()
      set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
    endif()
    target_include_directories(aml_interface INTERFACE ${path})
  endforeach()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_include_directories.html
function(aml_system_include_directories)
  foreach(arg ${ARGV})
    if(IS_ABSOLUTE ${arg})
      set(path ${arg})
    else()
      set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
    endif()
    target_include_directories(aml_interface SYSTEM INTERFACE ${path})
  endforeach()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_compile_definitions.html
function(aml_compile_definitions)
  target_compile_definitions(aml_interface INTERFACE ${ARGV})
endfunction()

# https://cmake.org/cmake/help/latest/command/target_compile_options.html
function(aml_compile_options)
  target_compile_options(aml_interface INTERFACE ${ARGV})
endfunction()

# https://cmake.org/cmake/help/latest/command/target_link_libraries.html
function(aml_link_libraries)
  set(COLLECT_LINK_LIBRARIES ${CURRENT_LIBRARY} ${COLLECT_LINK_LIBRARIES} CACHE INTERNAL "")
#  target_link_libraries(${TARGET_NAME} ${CURRENT_LIBRARY})
endfunction()

function(aml_link_libraries_with_name lib_name)
  set(COLLECT_LINK_LIBRARIES ${lib_name} ${COLLECT_LINK_LIBRARIES} CACHE INTERNAL "")
#  target_link_libraries(${TARGET_NAME} ${CURRENT_LIBRARY})
endfunction()

function(aml_link_cpp_libraries_with_name lib_name)
  set(COLLECT_LINK_LIBRARIES ${lib_name} ${COLLECT_LINK_LIBRARIES} CACHE INTERNAL "")
  set(COLLECT_IS_CPP_LIBRARIES "1" CACHE INTERNAL "")
endfunction()

function(aml_link_interfaces)
  target_link_libraries(${TARGET_NAME} ${CURRENT_INTERFACE})
endfunction()

# See this file section 3.1. target_cc_option
function(aml_cc_option)
  foreach(arg ${ARGV})
    target_cc_option(aml_interface INTERFACE ${arg})
  endforeach()
endfunction()

function(aml_cc_option_fallback option1 option2)
    target_cc_option_fallback(aml_interface INTERFACE ${option1} ${option2})
endfunction()

function(aml_ld_options)
    target_ld_options(aml_interface INTERFACE ${ARGV})
endfunction()

# Getter functions for extracting build information from
# aml_interface. Returning lists, and strings is supported, as is
# requesting specific categories of build information (defines,
# includes, options).
#
# The naming convention follows:
# aml_get_${build_information}_for_lang${format}(lang x [SKIP_PREFIX])
# Where
#  the argument 'x' is written with the result
# and
#  ${build_information} can be one of
#   - include_directories           # -I directories
#   - system_include_directories    # -isystem directories
#   - compile_definitions           # -D'efines
#   - compile_options               # misc. compiler flags
# and
#  ${format} can be
#   - the empty string '', signifying that it should be returned as a list
#   - _as_string signifying that it should be returned as a string
# and
#  ${lang} can be one of
#   - C
#   - CXX
#   - ASM
#
# SKIP_PREFIX
#
# By default the result will be returned ready to be passed directly
# to a compiler, e.g. prefixed with -D, or -I, but it is possible to
# omit this prefix by specifying 'SKIP_PREFIX' . This option has no
# effect for 'compile_options'.
#
# e.g.
# aml_get_include_directories_for_lang(ASM x)
# writes "-Isome_dir;-Isome/other/dir" to x

function(aml_get_include_directories_for_lang_as_string lang i)
  aml_get_include_directories_for_lang(${lang} list_of_flags ${ARGN})

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(aml_get_system_include_directories_for_lang_as_string lang i)
  aml_get_system_include_directories_for_lang(${lang} list_of_flags ${ARGN})

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(aml_get_compile_definitions_for_lang_as_string lang i)
  aml_get_compile_definitions_for_lang(${lang} list_of_flags ${ARGN})

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(aml_get_compile_options_for_lang_as_string lang i)
  aml_get_compile_options_for_lang(${lang} list_of_flags)

  convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

  set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(aml_get_include_directories_for_lang lang i)
  get_property_and_add_prefix(flags aml_interface INTERFACE_INCLUDE_DIRECTORIES
    "-I"
    ${ARGN}
    )

  process_flags(${lang} flags output_list)

  set(${i} ${output_list} PARENT_SCOPE)
endfunction()

function(aml_get_system_include_directories_for_lang lang i)
  get_property_and_add_prefix(flags aml_interface INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
    "-isystem"
    ${ARGN}
    )

  process_flags(${lang} flags output_list)

  set(${i} ${output_list} PARENT_SCOPE)
endfunction()

function(aml_get_compile_definitions_for_lang lang i)
  get_property_and_add_prefix(flags aml_interface INTERFACE_COMPILE_DEFINITIONS
    "-D"
    ${ARGN}
    )

  process_flags(${lang} flags output_list)

  set(${i} ${output_list} PARENT_SCOPE)
endfunction()

function(aml_get_compile_options_for_lang lang i)
  get_property(flags TARGET aml_interface PROPERTY INTERFACE_COMPILE_OPTIONS)

  process_flags(${lang} flags output_list)

  set(${i} ${output_list} PARENT_SCOPE)
endfunction()

# This function writes a dict to it's output parameter
# 'return_dict'. The dict has information about the parsed arguments,
#
# Usage:
#   aml_get_parse_args(foo ${ARGN})
#   print(foo_STRIP_PREFIX) # foo_STRIP_PREFIX might be set to 1
function(aml_get_parse_args return_dict)
  foreach(x ${ARGN})
    if(x STREQUAL STRIP_PREFIX)
      set(${return_dict}_STRIP_PREFIX 1 PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

function(process_flags lang input output)
  # The flags might contains compile language generator expressions that
  # look like this:
  # $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  #
  # Flags that don't specify a language like this apply to all
  # languages.
  #
  # See COMPILE_LANGUAGE in
  # https://cmake.org/cmake/help/v3.3/manual/cmake-generator-expressions.7.html
  #
  # To deal with this, we apply a regex to extract the flag and also
  # to find out if the language matches.
  #
  # If this doesn't work out we might need to ban the use of
  # COMPILE_LANGUAGE and instead partition C, CXX, and ASM into
  # different libraries
  set(languages C CXX ASM)

  set(tmp_list "")

  foreach(flag ${${input}})
    set(is_compile_lang_generator_expression 0)
    foreach(l ${languages})
      if(flag MATCHES "<COMPILE_LANGUAGE:${l}>:([^>]+)>")
        set(is_compile_lang_generator_expression 1)
        if(${l} STREQUAL ${lang})
          list(APPEND tmp_list ${CMAKE_MATCH_1})
          break()
        endif()
      endif()
    endforeach()

    if(NOT is_compile_lang_generator_expression)
      list(APPEND tmp_list ${flag})
    endif()
  endforeach()

  set(${output} ${tmp_list} PARENT_SCOPE)
endfunction()

function(convert_list_of_flags_to_string_of_flags ptr_list_of_flags string_of_flags)
  # Convert the list to a string so we can do string replace
  # operations on it and replace the ";" list separators with a
  # whitespace so the flags are spaced out
  string(REPLACE ";"  " "  locally_scoped_string_of_flags "${${ptr_list_of_flags}}")

  # Set the output variable in the parent scope
  set(${string_of_flags} ${locally_scoped_string_of_flags} PARENT_SCOPE)
endfunction()

macro(get_property_and_add_prefix result target property prefix)
  aml_get_parse_args(args ${ARGN})

  if(args_STRIP_PREFIX)
    set(maybe_prefix "")
  else()
    set(maybe_prefix ${prefix})
  endif()

  get_property(target_property TARGET ${target} PROPERTY ${property})
  foreach(x ${target_property})
    list(APPEND ${result} ${maybe_prefix}${x})
  endforeach()
endmacro()

# 1.2 aml_library_*
#
# Amlogic libraries use CMake's library concept and a set of
# assumptions about how aml code is organized to cut down on
# boilerplate code.
#
# A Amlogic library can be constructed by the function aml_library
# or aml_library_named. The constructors create a CMake library
# with a name accessible through the variable CURRENT_LIBRARY.
#
# The variable CURRENT_LIBRARY should seldomly be needed since
# the aml libraries have methods that modify the libraries. These
# methods have the signature: aml_library_<target-function>
#
# The methods are wrappers around the CMake target_* functions. See
# https://cmake.org/cmake/help/latest/manual/cmake-commands.7.html for
# documentation on the underlying target_* functions.
#
# The methods modify the CMake target_* API to reduce boilerplate;
#  PRIVATE is assumed
#  The target is assumed to be CURRENT_LIBRARY
#
# When a flag that is given through the aml_* API conflicts with
# the aml_library_* API then precedence will be given to the
# aml_library_* API. In other words, local configuration overrides
# global configuration.

# Constructor with a directory-inferred name
macro(aml_library)
  aml_library_get_current_dir_lib_name(lib_name)
  aml_library_named(${lib_name})
endmacro()

# Constructor with an explicitly given name.
macro(aml_library_named name)
  # This is a macro because we need add_library() to be executed
  # within the scope of the caller.
  set(CURRENT_LIBRARY ${name})
  add_library(${name} STATIC "")

  aml_append_cmake_library(${name})

  target_link_libraries(${name} PUBLIC aml_interface)
endmacro()

macro(redefine_file_macro)
  get_target_property(source_files "${CURRENT_LIBRARY}" SOURCES)
  foreach(sourcefile ${source_files})
    get_property(defs SOURCE "${sourcefile}" PROPERTY COMPILE_DEFINITIONS)
    get_filename_component(filepath "${sourcefile}" ABSOLUTE)
    string(REPLACE ${PROJECT_SOURCE_DIR}/ "" relpath ${filepath})
    list(APPEND defs "__FILE__=\"${relpath}\"")
    set_property(SOURCE "${sourcefile}" PROPERTY COMPILE_DEFINITIONS ${defs})
  endforeach()
endmacro()

macro(aml_add_library)
	STRING( REGEX REPLACE ".*/(.*)/.*" "\\1" LAYER ${CMAKE_CURRENT_SOURCE_DIR} )
	STRING( REGEX REPLACE ".*/(.*)" "\\1" MODULE ${CMAKE_CURRENT_SOURCE_DIR} )

  if(LAYER STREQUAL ARCH AND MODULE STREQUAL SPLIT_ARCH_DIR)
    STRING( REGEX REPLACE ".*/(.*)/.*/.*" "\\1" LAYER ${CMAKE_CURRENT_SOURCE_DIR} )
    STRING( REGEX REPLACE ".*/(.*)/.*" "\\1" MODULE ${CMAKE_CURRENT_SOURCE_DIR} )
  endif()

	set(CURRENT_LIBRARY_TYPE STATIC)
	set(CURRENT_LIBRARY ${LAYER}__${MODULE})
	message(STATUS "@@${CURRENT_LIBRARY_TYPE}: ${CURRENT_LIBRARY}")
	add_library(${CURRENT_LIBRARY} ${CURRENT_LIBRARY_TYPE} "")

	set_target_properties(${CURRENT_LIBRARY} PROPERTIES LIBRARY_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
endmacro()

macro(aml_add_library_spec CURRENT_LIBRARY CURRENT_LIBRARY_TYPE)
	add_library(${CURRENT_LIBRARY} ${CURRENT_LIBRARY_TYPE} "")
	set_target_properties(${CURRENT_LIBRARY} PROPERTIES LIBRARY_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
endmacro()

macro(aml_add_interface)
	STRING( REGEX REPLACE ".*/(.*)/.*" "\\1" LAYER ${CMAKE_CURRENT_SOURCE_DIR} )
	STRING( REGEX REPLACE ".*/(.*)" "\\1" MODULE ${CMAKE_CURRENT_SOURCE_DIR} )

	set(CURRENT_INTERFACE_TYPE INTERFACE)
	set(CURRENT_INTERFACE ${LAYER}__${MODULE})
	message(STATUS "@@${CURRENT_INTERFACE_TYPE}: ${CURRENT_INTERFACE}")
	add_library(${CURRENT_INTERFACE} ${CURRENT_INTERFACE_TYPE})
endmacro()

function(aml_link_interface interface)
  target_link_libraries(${interface} INTERFACE aml_interface)
endfunction()

#
# aml_library versions of normal CMake target_<func> functions
#
function(aml_library_sources source)
	#remove the base dir if the source is a full path
	string(REPLACE "${CMAKE_CURRENT_LIST_DIR}/" "" source ${source})
	#calculate the obj output full path
	set(FILE_NAME "${source}.obj")
	get_target_property(LIBRARY_BASE_PATH ${CURRENT_LIBRARY} LIBRARY_BASE_PATH)
	string(REPLACE ${LIBRARY_BASE_PATH} "${LIBRARY_BASE_PATH}/CMakeFiles/${CURRENT_LIBRARY}.dir"
		OBJ_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
	string(REPLACE ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/output/$ENV{ARCH}-$ENV{BOARD}-$ENV{PRODUCT}/$ENV{KERNEL}/obj"
		OBJ_OUTPUT_DIR ${OBJ_OUTPUT_DIR})
	set(OBJ_OUTPUT_PATH "${OBJ_OUTPUT_DIR}/${FILE_NAME}")
	#collect the obj output full path to COLLECT_LINK_OBJS
	set(COLLECT_LINK_OBJS ${OBJ_OUTPUT_PATH} ${COLLECT_LINK_OBJS} CACHE INTERNAL "")

	FOREACH(PART_PATH ${ARGN})
		string(REPLACE "${CMAKE_CURRENT_LIST_DIR}/" "" PART_PATH ${PART_PATH})
		set(FILE_NAME "${PART_PATH}.obj")
		set(OBJ_OUTPUT_PATH "${OBJ_OUTPUT_DIR}/${FILE_NAME}")
		#if the obj is exist in collection,ignore it
		string(FIND "${COLLECT_LINK_OBJS}" "${OBJ_OUTPUT_PATH}" ret)
		if(${ret} STREQUAL "-1")
			set(COLLECT_LINK_OBJS ${OBJ_OUTPUT_PATH} ${COLLECT_LINK_OBJS} CACHE INTERNAL "")
		endif()
	ENDFOREACH(PART_PATH)

	target_sources(${CURRENT_LIBRARY} PRIVATE ${source} ${ARGN})
endfunction()

function(aml_library_include_directories)
  target_include_directories(${CURRENT_LIBRARY} PUBLIC ${ARGN})
endfunction()

function(aml_library_include_directories_ifdef feature_toggle)
if(${${feature_toggle}})
  target_include_directories(${CURRENT_LIBRARY} PUBLIC ${ARGN})
endif()
endfunction()

function(aml_interface_include_directories)
  target_include_directories(${CURRENT_INTERFACE} INTERFACE ${ARGN})
endfunction()

function(aml_library_link_libraries item)
  target_link_libraries(${CURRENT_LIBRARY} PUBLIC ${item} ${ARGN})
endfunction()

function(aml_interface_link_interfaces item)
  target_link_libraries(${CURRENT_INTERFACE} INTERFACE ${item} ${ARGN})
endfunction()

function(aml_library_compile_definitions item)
  target_compile_definitions(${CURRENT_LIBRARY} PRIVATE ${item} ${ARGN})
endfunction()

function(aml_library_compile_options item)
  # The compiler is relied upon for sane behaviour when flags are in
  # conflict. Compilers generally give precedence to flags given late
  # on the command line. So to ensure that aml_library_* flags are
  # placed late on the command line we create a dummy interface
  # library and link with it to obtain the flags.
  #
  # Linking with a dummy interface library will place flags later on
  # the command line than the the flags from aml_interface because
  # aml_interface will be the first interface library that flags
  # are taken from.

  string(MD5 uniqueness ${item})
  set(lib_name options_interface_lib_${uniqueness})

  if (TARGET ${lib_name})
    # ${item} already added, ignoring duplicate just like CMake does
    return()
  endif()

  add_library(           ${lib_name} INTERFACE)
  target_compile_options(${lib_name} INTERFACE ${item} ${ARGN})

  target_link_libraries(${CURRENT_LIBRARY} PRIVATE ${lib_name})
endfunction()

function(aml_library_cc_option)
  foreach(option ${ARGV})
    string(MAKE_C_IDENTIFIER check${option} check)
    aml_check_compiler_flag(C ${option} ${check})

    if(${check})
      aml_library_compile_options(${option})
    endif()
  endforeach()
endfunction()

# 1.2.1 aml_interface_library_*

# 1.3 generate_inc_*

# 1.4. board_*

# 1.5. Misc.

########################################################
# 2. Kconfig-aware extensions
########################################################
#
# Kconfig is a configuration language developed for the Linux
# kernel. The below functions integrate CMake with Kconfig.
#
# 2.1 *_if_kconfig
#
# Functions for conditionally including directories and source files
# that have matching KConfig values.
#
# aml_library_sources_if_kconfig(fft.c)
# is the same as
# aml_library_sources_ifdef(CONFIG_FFT fft.c)
#
# add_subdirectory_if_kconfig(serial)
# is the same as
# add_subdirectory_ifdef(CONFIG_SERIAL serial)
function(add_subdirectory_if_kconfig dir)
  string(TOUPPER config_${dir} UPPER_CASE_CONFIG)
  add_subdirectory_ifdef(${UPPER_CASE_CONFIG} ${dir})
endfunction()

function(target_sources_if_kconfig target scope item)
  get_filename_component(item_basename ${item} NAME_WE)
  string(TOUPPER CONFIG_${item_basename} UPPER_CASE_CONFIG)
  target_sources_ifdef(${UPPER_CASE_CONFIG} ${target} ${scope} ${item})
endfunction()

function(aml_library_sources_if_kconfig item)
  get_filename_component(item_basename ${item} NAME_WE)
  string(TOUPPER CONFIG_${item_basename} UPPER_CASE_CONFIG)
  aml_library_sources_ifdef(${UPPER_CASE_CONFIG} ${item})
endfunction()

function(aml_sources_if_kconfig item)
  get_filename_component(item_basename ${item} NAME_WE)
  string(TOUPPER CONFIG_${item_basename} UPPER_CASE_CONFIG)
  aml_sources_ifdef(${UPPER_CASE_CONFIG} ${item})
endfunction()

# 2.2 Misc
#
# Parse a KConfig fragment (typically with extension .config) and
# introduce all the symbols that are prefixed with 'prefix' into the
# CMake namespace
function(import_kconfig prefix kconfig_fragment)
  # Parse the lines prefixed with 'prefix' in ${kconfig_fragment}
  file(
    STRINGS
    ${kconfig_fragment}
    DOT_CONFIG_LIST
    REGEX "^${prefix}"
    ENCODING "UTF-8"
  )

  foreach (CONFIG ${DOT_CONFIG_LIST})
    # CONFIG could look like: CONFIG_NET_BUF=y

    # Match the first part, the variable name
    string(REGEX MATCH "[^=]+" CONF_VARIABLE_NAME ${CONFIG})

    # Match the second part, variable value
    string(REGEX MATCH "=(.+$)" CONF_VARIABLE_VALUE ${CONFIG})
    # The variable name match we just did included the '=' symbol. To just get the
    # part on the RHS we use match group 1
    set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})

    if("${CONF_VARIABLE_VALUE}" MATCHES "^\"(.*)\"$") # Is surrounded by quotes
      set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})
    endif()

    set("${CONF_VARIABLE_NAME}" "${CONF_VARIABLE_VALUE}" PARENT_SCOPE)
  endforeach()
endfunction()

########################################################
# 3. CMake-generic extensions
########################################################
#
# These functions extend the CMake API in a way that is not particular
# to Amlogic. Primarily they work around limitations in the CMake
# language to allow cleaner build scripts.

# 3.1. *_ifdef
#
# Functions for conditionally executing CMake functions with oneliners
# e.g.
#
# if(CONFIG_FFT)
#   aml_library_source(
#     fft_32.c
#     fft_utils.c
#     )
# endif()
#
# Becomes
#
# aml_source_ifdef(
#   CONFIG_FFT
#   fft_32.c
#   fft_utils.c
#   )
#
# More Generally
# "<function-name>_ifdef(CONDITION args)"
# Becomes
# """
# if(CONDITION)
#   <function-name>(args)
# endif()
# """
#
# ifdef functions are added on an as-need basis. See
# https://cmake.org/cmake/help/latest/manual/cmake-commands.7.html for
# a list of available functions.
function(add_subdirectory_ifdef feature_toggle dir)
  if(${${feature_toggle}})
    add_subdirectory(${dir})
  endif()
endfunction()

function(target_sources_ifdef feature_toggle target scope item)
  if(${${feature_toggle}})
    target_sources(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

function(target_compile_definitions_ifdef feature_toggle target scope item)
  if(${${feature_toggle}})
    target_compile_definitions(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

function(target_include_directories_ifdef feature_toggle target scope item)
  if(${${feature_toggle}})
    target_include_directories(${target} ${scope} ${item} ${ARGN})
  endif()
endfunction()

function(target_link_libraries_ifdef feature_toggle target item)
  if(${${feature_toggle}})
    target_link_libraries(${target} ${item} ${ARGN})
  endif()
endfunction()

function(add_compile_option_ifdef feature_toggle option)
  if(${${feature_toggle}})
    add_compile_options(${option})
  endif()
endfunction()

function(target_compile_option_ifdef feature_toggle target scope option)
  if(${feature_toggle})
    target_compile_options(${target} ${scope} ${option})
  endif()
endfunction()

function(target_cc_option_ifdef feature_toggle target scope option)
  if(${feature_toggle})
    target_cc_option(${target} ${scope} ${option})
  endif()
endfunction()

function(aml_library_sources_ifdef feature_toggle source)
  if(${${feature_toggle}})
    aml_library_sources(${source} ${ARGN})
  endif()
endfunction()

function(aml_library_sources_ifndef feature_toggle source)
  if(NOT ${feature_toggle})
    aml_library_sources(${source} ${ARGN})
  endif()
endfunction()

function(aml_sources_ifdef feature_toggle)
  if(${${feature_toggle}})
    aml_sources(${ARGN})
  endif()
endfunction()

function(aml_sources_ifndef feature_toggle)
   if(NOT ${feature_toggle})
    aml_sources(${ARGN})
  endif()
endfunction()

function(aml_cc_option_ifdef feature_toggle)
  if(${${feature_toggle}})
    aml_cc_option(${ARGN})
  endif()
endfunction()

function(aml_ld_option_ifdef feature_toggle)
  if(${${feature_toggle}})
    aml_ld_options(${ARGN})
  endif()
endfunction()

function(aml_link_libraries_ifdef feature_toggle)
  if(${${feature_toggle}})
    aml_link_libraries(${ARGN})
  endif()
endfunction()

function(aml_compile_options_ifdef feature_toggle)
  if(${${feature_toggle}})
    aml_compile_options(${ARGN})
  endif()
endfunction()

function(aml_compile_definitions_ifdef feature_toggle)
  if(${${feature_toggle}})
    aml_compile_definitions(${ARGN})
  endif()
endfunction()

function(aml_include_directories_ifdef feature_toggle)
  if(${${feature_toggle}})
    aml_include_directories(${ARGN})
  endif()
endfunction()

function(aml_library_compile_definitions_ifdef feature_toggle item)
  if(${${feature_toggle}})
    aml_library_compile_definitions(${item} ${ARGN})
  endif()
endfunction()

function(aml_library_compile_options_ifdef feature_toggle item)
  if(${${feature_toggle}})
    aml_library_compile_options(${item} ${ARGN})
  endif()
endfunction()

function(aml_link_interface_ifdef feature_toggle interface)
  if(${${feature_toggle}})
    target_link_libraries(${interface} INTERFACE aml_interface)
  endif()
endfunction()

function(aml_library_link_libraries_ifdef feature_toggle item)
  if(${${feature_toggle}})
     aml_library_link_libraries(${item})
  endif()
endfunction()

macro(list_append_ifdef feature_toggle list)
  if(${${feature_toggle}})
    list(APPEND ${list} ${ARGN})
  endif()
endmacro()

# 3.4. Debugging CMake

# Usage:
#   print(BOARD)
#
# will print: "BOARD: nrf52_pca10040"
function(print arg)
  message(STATUS "${arg}: ${${arg}}")
endfunction()

# will cause a FATAL_ERROR and print an error message if the first
# expression is false
macro(assert test comment)
  if(NOT ${test})
    message(FATAL_ERROR "Assertion failed: ${comment}")
  endif()
endmacro()

# Usage:
#   assert_not(FLASH_SCRIPT "FLASH_SCRIPT has been removed; use BOARD_FLASH_RUNNER")
#
# will cause a FATAL_ERROR and print an errorm essage if the first
# espression is true
macro(assert_not test comment)
  if(${test})
    message(FATAL_ERROR "Assertion failed: ${comment}")
  endif()
endmacro()

# Usage:
#   assert_exists(CMAKE_READELF)
#
# will cause a FATAL_ERROR if there is no file or directory behind the
# variable
macro(assert_exists var)
  if(NOT EXISTS ${${var}})
    message(FATAL_ERROR "No such file or directory: ${var}: '${${var}}'")
  endif()
endmacro()

# 3.5. extend files management
function(compiler_generate_binary_output TARGET)
  add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_OBJCOPY_COMPILER} ARGS -O binary $<TARGET_FILE:${TARGET}> $<TARGET_FILE_DIR:${TARGET}>/$ENV{KERNEL}.bin)
endfunction()

function(compiler_generate_lst_output TARGET)
  #Generate disassembly file intermix source code
  #llvm-objdump -S will warning can't find source file which in the prebuilt library,
  #then use 2>/dev/null to ignore standard error output
  add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_OBJDUMP_COMPILER} ARGS -S $<TARGET_FILE:${TARGET}> > $<TARGET_FILE_DIR:${TARGET}>/$ENV{KERNEL}.lst 2>/dev/null)
endfunction()

function(generate_module_info_output TARGET)
    set(map_file_path ${PROJECT_SOURCE_DIR}/output/$ENV{ARCH}-$ENV{BOARD}-$ENV{PRODUCT}/$ENV{KERNEL}/${TARGET_NAME}.map)
    add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND COMPILER=$ENV{COMPILER} ARCH=$ENV{ARCH} ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/map_analyzer_$ENV{COMPILER}.py --combine ${map_file_path} > $<TARGET_FILE_DIR:${TARGET}>/$ENV{KERNEL}_module_info.txt)
endfunction()

