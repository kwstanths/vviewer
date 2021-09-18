#include <iostream>

#include <qapplication.h>

#include "UI/MainWindow.hpp"

int main(int argc, char ** argv) {

    try {
        QApplication app(argc, argv);

        MainWindow mainWindow(nullptr);
        mainWindow.show();

        return app.exec();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}