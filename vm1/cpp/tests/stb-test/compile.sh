rm stb_app.out

g++ -std=c++20 -Iinclude -g \
 main.cpp \
 source/Context.cpp \
 source/KeyboardController.cpp \
 source/StbRenderer.cpp \
 source/Gui.cpp \
 source/GuiStateMachine.cpp \
 source/GuiItems.cpp \
 source/GuiRenderer.cpp \
 source/AppEventHandler.cpp \
 -o stb_app.out

./stb_app.out