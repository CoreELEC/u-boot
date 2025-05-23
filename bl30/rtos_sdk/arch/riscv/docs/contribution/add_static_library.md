How to Add a New Static Library {#add_static_library}
===========================================

Let's take ***blueaml*** library for example.

### Step 1: Prepare your library and include files. ###
Path of blueaml library is ***wcn/blueaml***, and files tree as below:

	.
	├── arm
	│   └── wcn__blueaml_rel.a
	├── arm64
	│   └── wcn__blueaml_rel.a
	├── include
	│   ├── blueaml_api.h
	│   ├── blueaml_port.h
	│   ├── bt_log.h
	│   └── bt_types.h
	├── LICENSE

### Step 2: Add ***Kconfig***. ###
Write ***Kconfig*** for your library.

@code
menu "Blueaml"

config BLUEAML
    bool "Blueaml"
    select ULOG
    help
          Enable Blueaml.

if BLUEAML
    config BT_SDP
        bool "SDP"
        default y
        help
            Enable SDP Function
	......

endif # BLUEAML
endmenu

@endcode

Then add the following line to root ***Kconfig***.
@code
source "wcn/blueaml/Kconfig"
@endcode

### Step 3: Add ***CMakeLists.txt***. ###
Write ***CMakeLists.txt*** for your library.
@code
if(CONFIG_BLUEAML)
	STRING( REGEX REPLACE ".*/(.*)/.*" "\\1" LAYER ${CMAKE_CURRENT_SOURCE_DIR} ) #get LAYER name(wcn)
	STRING( REGEX REPLACE ".*/(.*)" "\\1" MODULE ${CMAKE_CURRENT_SOURCE_DIR} ) #get MODULE name (blueaml)

	set(LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/wcn__blueaml_rel.a)
	set(CURRENT_LIBRARY_TYPE STATIC)
	set(CURRENT_LIBRARY ${LAYER}__${MODULE})         # set current library name is "wcn__blueaml"
	set(inc_dir "${CMAKE_CURRENT_LIST_DIR}/include")
	message(STATUS "@@${CURRENT_LIBRARY_TYPE}: ${CURRENT_LIBRARY}")

	add_library(${CURRENT_LIBRARY} ${CURRENT_LIBRARY_TYPE} IMPORTED GLOBAL)
	set_property(TARGET ${CURRENT_LIBRARY} PROPERTY
				 IMPORTED_LOCATION "${LIBRARY_PATH}")

	target_include_directories(${CURRENT_LIBRARY} INTERFACE "${inc_dir}") # declaration include path

endif()
@endcode

Then add the following line to root ***CMakeLists.txt***.
@code
add_subdirectory(wcn/blueaml)
@endcode

### Step 3: Use the static lirary. ###
Let's take ***example/btapp*** for example.
Add the following line to ***example/btapp/CMakeLists.txt***.

@code
if (CONFIG_BLUEAML)
	aml_library_link_libraries(wcn__blueaml) #depends on library of wcn__blueaml
endif()
@endcode

Right now, user can include the header file of ***wcn/blueaml/include*** in bt_app code.

### Step 4: Notice. ###
The CONFIG_BLUEAML is disable by default, therefore, user need enable CONFIG_BLUEAML,

for example add code in ***example/btapp/Kconfig*** as below:
@code
config BTAPP
	bool
	select BLUEAML #enable CONFIG_BLUEAML
	help
	  Enalbe Bluetooth applications.
@endcode
