set(LIBS ${LIBS})
set(MODULES media sensors)
if(APPLE)
#link_directories(/usr/local/opt/opencv)
endif()
KINSKI_ADD_SAMPLE(${itemName} ${folderItem} "${MODULES}")

