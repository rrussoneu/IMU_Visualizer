#include <QApplication>
#include "ui/main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    // Set default surface format
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);  // Set as default format
    qRegisterMetaType<imu_viz::Quaterniond>("Quaterniond");
    qRegisterMetaType<imu_viz::Vector3d>("Vector3d");


    imu_viz::MainWindow window;
    window.show();

    return app.exec();
}