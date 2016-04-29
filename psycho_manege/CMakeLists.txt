set(LIBS ${LIBS})
set(MODULES assimp opencl syphon rtmidi audio)
add_definitions(-D__MACOSX_CORE__)
KINSKI_ADD_SAMPLE(${itemName} ${folderItem} "${MODULES}")