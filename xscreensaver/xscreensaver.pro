QT -= gui
CONFIG -= qt
TEMPLATE = aux

include(../common.pri)

cover.files += $$PWD/cover/*
cover.path = $$MODULE_PATH/cover

screensaver_list = blaster bouboule cage discoball dymaxionmap endgame epicycle \
    fireworkx flow flurry flyingtoasters fuzzyflakes galaxy glplanet hexadrop \
    klein lament lockward pacman polytopes vermiculate whirlwindwarp

isEmpty(XSCREENSAVER_DATA_PATH) {
    XSCREENSAVER_DATA_PATH = $$PREFIX/lib/xscreensaver
}

screensaver.path = $$MODULE_PATH
mkpath($$PWD/extras)

for(ss, screensaver_list) {
    system(ln -s $$XSCREENSAVER_DATA_PATH/$$ss $$PWD/extras/$$ss)
    screensaver.files += $$PWD/extras/$$ss
}

INSTALLS += cover screensaver
