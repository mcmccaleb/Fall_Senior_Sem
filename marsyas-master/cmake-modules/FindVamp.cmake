find_path(VAMP_INCLUDE_DIR vamp-sdk/Plugin.h)
find_library(VAMP_LIBRARY NAMES vamp-sdk)

if (VAMP_INCLUDE_DIR AND VAMP_LIBRARY)
	set(VAMP_FOUND TRUE)
endif (VAMP_INCLUDE_DIR AND VAMP_LIBRARY)

if (VAMP_FOUND)
	if (NOT VAMP_FIND_QUIETLY)
		message (STATUS "Found VAMP: ${VAMP_LIBRARY}")
	endif (NOT VAMP_FIND_QUIETLY)
else (VAMP_FOUND)
	if (VAMP_FIND_REQUIRED)
		message (FATAL_ERROR "Could not find: VAMP")
	endif (VAMP_FIND_REQUIRED)
endif (VAMP_FOUND)


