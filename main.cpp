#include <QApplication>

#include "formcar.h"
#include "dialog.h"
#include "controllers/CanController.h"
#include "controllers/els27/QControllerEls27.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

    std::unique_ptr<Dialog>  init_dialog = std::make_unique<Dialog>();
    std::unique_ptr<FormCar> form;

    std::unique_ptr<CanController> controller;

    QObject::connect(init_dialog.get(), &Dialog::btnOk_click, [&]{

        auto settings_from_dialog = init_dialog->getSettings();
        sControllerSettings controller_settings { .port_name = settings_from_dialog.port_name,
                                                  .baud = static_cast<uint32_t>(!settings_from_dialog.autodetect ? settings_from_dialog.baudrate : 0),
                                                  .maximize = settings_from_dialog.maximize };
        controller = std::make_unique<QControllerEls27>(controller_settings);

        form = std::make_unique<FormCar>(std::move(controller));
        form->setWindowTitle("Ford Focus MK3 IPC Simulator");
        form->show();
    });

    init_dialog->setWindowTitle("Settings");
    init_dialog->show();

	return QApplication::exec();
}
