if(KINSKI_RASPI)
    find_library(WIRINGPI NAMES wiringPi)
    set(LIBS ${LIBS} ${WIRINGPI})
    message("${LIBS}")
endif()
set(LIBS ${LIBS} ${WIRINGPI})
set(MODULES media dmx)
KINSKI_ADD_SAMPLE(${itemName} ${folderItem} "${MODULES}")

