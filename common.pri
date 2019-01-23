isEmpty(PREFIX) {
    PREFIX = /usr
}

isEmpty(LIB_PATH) {
    # /usr/lib/deepin-screensaver
    LIB_PATH = $$PREFIX/lib/deepin-screensaver
}

isEmpty(MODULE_PATH) {
    # /usr/lib/deepin-screensaver/modules
    MODULE_PATH = $$LIB_PATH/modules
}

isEmpty(RESOURCE_PATH) {
    # /usr/lib/deepin-screensaver/modules
    RESOURCE_PATH = $$LIB_PATH/resources
}
