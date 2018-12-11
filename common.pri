isEmpty(PREFIX) {
    PREFIX = /usr
}

isEmpty(MODULE_PATH) {
    # /usr/lib/deepin-screensaver/modules
    MODULE_PATH = $$PREFIX/lib/deepin-screensaver/modules
}

isEmpty(RESOURCE_PATH) {
    # /usr/lib/deepin-screensaver/modules
    RESOURCE_PATH = $$PREFIX/lib/deepin-screensaver/resources
}
