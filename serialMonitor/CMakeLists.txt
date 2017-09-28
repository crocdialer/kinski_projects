set(MODULES dmx rtmidi)
if(APPLE)
    add_definitions(-D__MACOSX_CORE__)
else()
    add_definitions(-D__LINUX_ALSA__)
endif(APPLE)
KINSKI_ADD_SAMPLE(${itemName} ${folderItem} "${MODULES}")


