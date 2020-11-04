#include "formcar.h"
#include "ui_formcar.h"
#include "can_sym.h"

FormCar::FormCar(std::unique_ptr<CanController> controller, QWidget *parent):
        QWidget(parent),
        ui(new Ui::FormCar),
        controller(std::move(controller))
{
    ui->setupUi(this);
    this->show();
    setupSimulator();
}

FormCar::~FormCar()
{
	delete ui;
}

void FormCar::setupSimulator() {

    controller->set_protocol(CanController::CAN_MS);

    /* reset dash */
    resetDash(static_cast<CanController*>(static_cast<CanController*>(controller.get())));
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    /* ignition */
    t_ignition = new  IPCthread(250000);
    connect(t_ignition, &IPCthread::finished, t_ignition, &QObject::deleteLater);
    t_ignition->registerCallback(fakeIgnition, static_cast<CanController*>(controller.get()));

    /* speed & rpm */
    t_speed_rpm = new  IPCthread(250000);
    connect(t_speed_rpm, &IPCthread::finished, t_speed_rpm, &QObject::deleteLater);
    t_speed_rpm->registerCallback([&]{
        fakeEngineRpmAndSpeed(static_cast<CanController*>(controller.get()), g_rpm, g_speed);
    });

    /* fuel */
    t_fuel_temp = new  IPCthread(250000);
    connect(t_fuel_temp, &IPCthread::finished, t_fuel_temp, &QObject::deleteLater);
    t_fuel_temp->registerCallback([&]{
        fakeFuel(static_cast<CanController*>(controller.get()), g_fuel);
    });

    /* eng temp */
    t_eng_temp = new  IPCthread(250000);
    connect(t_eng_temp, &IPCthread::finished, t_eng_temp, &QObject::deleteLater);
    t_eng_temp->registerCallback([&]{
        fakeEngineTemp(static_cast<CanController*>(controller.get()), g_eng_temp);
    });

    /* turns */
    t_turn = new  IPCthread(250000);
    connect(t_turn, &IPCthread::finished, t_turn, &QObject::deleteLater);
    t_turn->registerCallback([&]{
        if(g_turn_flag)
            fakeTurn(static_cast<CanController*>(controller.get()), g_turn_l, g_turn_r);
        else
            fakeTurn(static_cast<CanController*>(controller.get()), false, false);

        g_turn_flag = !g_turn_flag;
    });

    /* Ignition on/off */
    t_ignition->start();
    t_speed_rpm->start();
    t_fuel_temp->start();
    t_eng_temp->start();
    t_turn->start();

    /* Speed */
    connect(ui->spinBox_Speed, QOverload<int>::of(&QSpinBox::valueChanged),
            [=](int i){ g_speed = i; });

    /* RPM */
    connect(ui->spinBox_RPM, QOverload<int>::of(&QSpinBox::valueChanged),
            [=](int i){ g_rpm = i; });

    /* Engine temp */
    connect(ui->spinBox_Temp, QOverload<int>::of(&QSpinBox::valueChanged),
            [=](int i){ g_eng_temp = i; });

    /* Fuel level */
    connect(ui->spinBox_Fuel, QOverload<int>::of(&QSpinBox::valueChanged),
            [=](int i){ g_fuel = i; });

    /* Turn left */
    connect(ui->pushButton_LeftTurn, QOverload<bool>::of(&QPushButton::toggled),
            [=](bool toggled){ g_turn_l = toggled; });

    /* Turn right */
    connect(ui->pushButton_LeftTurn, QOverload<bool>::of(&QPushButton::toggled),
            [=](bool toggled){ g_turn_r = toggled; });

}
