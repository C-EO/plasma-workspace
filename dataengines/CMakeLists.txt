# TODO use modern compiler settings when dataengines are ported
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_TO_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

add_subdirectory(applicationjobs)
add_subdirectory(executable)
add_subdirectory(notifications)

add_subdirectory(time)
