#find_package(OpenCV)
#include_directories(${OpenCV_INCLUDE_DIRS})
#link_directories(${OpenCV_LIBRARY_DIRS})
#set(LIBS ${LIBS} ${OpenCV_LIBRARIES})

set(MODULES media sensors)
KINSKI_ADD_SAMPLE(${itemName} ${folderItem} "${MODULES}")

