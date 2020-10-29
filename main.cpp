#include <QApplication>

#include "formcar.h"
#include "dialog.h"
#include "controllers/CanController.h"
#include "controllers/els27/ControllerEls27.h"
#include "controllers/els27/QControllerEls27.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

    std::unique_ptr<Dialog>  init_dialog = std::make_unique<Dialog>();
    std::unique_ptr<FormCar> form;

    std::unique_ptr<CanController> controller;

    QObject::connect(init_dialog.get(), &Dialog::btnOk_click, [&]{

        auto type = init_dialog->getControllerType();
        if(Dialog::els27 == type) {
            auto settings_from_dialog = init_dialog->getSettings(type);
            /* pretty ugly part */
            QControllerEls27::settings controller_settings { .port_name = settings_from_dialog.port_name,
                                                             .baud = static_cast<uint32_t>(!settings_from_dialog.autodetect ? settings_from_dialog.baudrate : 0),
                                                             .maximize = settings_from_dialog.maximize };
            controller = std::make_unique<QControllerEls27>(controller_settings);
        } else {
            return;
        }

        controller->init();
        form = std::make_unique<FormCar>(std::move(controller));
    });

    init_dialog->show();

	return QApplication::exec();
}
