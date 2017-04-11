set(LIBS ${LIBS})
set(MODULES assimp opencl syphon rtmidi audio)

if(APPLE)
    add_definitions(-D__MACOSX_CORE__)
else()
    add_definitions(-D__LINUX_ALSA__)
endif()

KINSKI_ADD_SAMPLE(${itemName} ${folderItem} "${MODULES}")