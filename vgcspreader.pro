QT += widgets concurrent

INCLUDEPATH += include include/gui
SOURCES += \
    source/main.cpp \
    source/modifier.cpp \
    source/move.cpp \
    source/pokemon.cpp \
    source/pokemondb.cpp \
    source/stats.cpp \
    source/turn.cpp \
    source/gui/mainwindow.cpp \
    source/gui/resultwindow.cpp \
    source/item.cpp \
    source/gui/alertwindow.cpp \
    source/defenseresult.cpp \
    source/attackresult.cpp \
    source/gui/defensemovewindow.cpp \
    source/gui/attackmovewindow.cpp \
    source/gui/presetwindow.cpp \
    source/gui/addpresetwindow.cpp \
    source/gui/savedcalcwindow.cpp \
    source/tinyxml2.cpp

HEADERS += \
    include/abilities.hpp \
    include/items.hpp \
    include/modifier.hpp \
    include/move.hpp \
    include/moves.hpp \
    include/pokemon.hpp \
    include/pokemondb.hpp \
    include/stats.hpp \
    include/turn.hpp \
    include/types.hpp \
    include/gui/mainwindow.hpp \
    include/gui/resultwindow.hpp \
    include/item.hpp \
    include/gui/alertwindow.hpp \
    include/defenseresult.hpp \
    include/attackresult.hpp \
    include/gui/defensemovewindow.hpp \
    include/gui/attackmovewindow.hpp \
    include/gui/presetwindow.hpp \
    include/gui/addpresetwindow.hpp \
    include/gui/savedcalcwindow.hpp \
    include/tinyxml2.h

RESOURCES += \
    resources.qrc

win32 {
    RC_ICONS = vgcspreader.ico
}

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 12.0
    ICON = macos/vgcspreader.icns
    QMAKE_INFO_PLIST = macos/Info.plist
}
