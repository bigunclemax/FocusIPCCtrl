#include "formcar.h"
#include "ui_formcar.h"
#include "can_sym.h"

FormCar::FormCar(std::unique_ptr<CanController> controller, QWidget *parent):
        QWidget(parent),
        ui(new Ui::FormCar),
        controller(std::move(controller))
{
    ui->setupUi(this);
    setupSimulator();
    start();
}

FormCar::~FormCar()
{
    stop();
	delete ui;
}

void FormCar::setupSimulator() {

    controller->set_protocol(CanController::CAN_MS);

    /* ignition and miscellaneus */
    t_ignition_miscellaneous = std::make_unique<IPCthread>(250000);
    t_ignition_miscellaneous->registerCallback([&] {
            fakeIgnitionMiscellaneous(static_cast<CanController*>(controller.get()), g_drv_door, g_psg_door, g_rdrv_door, g_rpsg_door, g_hood, g_boot,
                                      g_acc_status, g_acc_standby);
    });

    /* speed & rpm */
    t_speed_rpm = std::make_unique<IPCthread>(250000);
    t_speed_rpm->registerCallback([&]{
        fakeEngineRpmAndSpeed(static_cast<CanController*>(controller.get()), g_rpm, g_speed, g_speed_warning);
    });

    /* fuel */
    t_fuel_temp = std::make_unique<IPCthread>(250000);
    t_fuel_temp->registerCallback([&]{
        fakeFuel(static_cast<CanController*>(controller.get()), g_fuel);
    });

    /* eng temp */
    t_eng_temp = std::make_unique<IPCthread>(250000);
    t_eng_temp->registerCallback([&]{
        fakeEngineTemp(static_cast<CanController*>(controller.get()), g_eng_temp);
    });

    /* Turns */
    t_turn = std::make_unique<IPCthread>(250000);
    t_turn->registerCallback([&]{
        if(g_turn_flag)
            fakeTurn(static_cast<CanController*>(controller.get()), g_turn_l, g_turn_r, g_cruise);
        else
            fakeTurn(static_cast<CanController*>(controller.get()), false, false, g_cruise);

        g_turn_flag = !g_turn_flag;
    });

    /* ACC Set Distance */
    t_acc = std::make_unique<IPCthread>(250000);
    t_acc->registerCallback([&] {
        accSetDistance(static_cast<CanController*>(controller.get()), g_acc_distance, g_acc_distance2, g_acc_status, g_acc_standby);
    });

    /* ACC Simulate Distance */
    t_acc2 = std::make_unique<IPCthread>(250000);
    t_acc2->registerCallback([&] {
        accSimulateDistance(static_cast<CanController*>(controller.get()), g_acc_status, g_acc_standby);
    });

    /* Play Alarm Sound */
    t_alarm = std::make_unique<IPCthread>(250000);
    t_alarm->registerCallback([&] {
        playAlarm(static_cast<CanController*>(controller.get()), g_alarm);
    });

    /* LCD Dimming */
    t_dimming = std::make_unique<IPCthread>(250000);
    t_dimming->registerCallback([&] {
        changeDimming(static_cast<CanController*>(controller.get()), g_dimming);
    });

    /* External temperature */
    t_external_temp = std::make_unique<IPCthread>(250000);
    t_external_temp->registerCallback([&] {
        fakeExternalTemp(static_cast<CanController*>(controller.get()), g_external_temp);
    });

    /* Ignition on/off */
    connect(ui->pushButton_Ignition, QOverload<bool>::of(&QPushButton::toggled),
            [this](bool i){ i ? start() : stop(); });

    /* Speed */
    connect(ui->spinBox_Speed, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i){ g_speed = i; });

    /* RPM */
    connect(ui->spinBox_RPM, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i){ g_rpm = i; });

    /* Speed Warning */
    connect(ui->pushButton_speedWarning, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_speed_warning = toggled; });

    /* Engine temp */
    connect(ui->spinBox_Temp, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i){ g_eng_temp = i; });

    /* Fuel level */
    connect(ui->spinBox_Fuel, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i){ g_fuel = i; });

    /* Turn left */
    connect(ui->pushButton_LeftTurn, QOverload<bool>::of(&QPushButton::toggled),
            [this](bool toggled){ g_turn_l = toggled; });

    /* Turn right */
    connect(ui->pushButton_RightTurn, QOverload<bool>::of(&QPushButton::toggled),
            [this](bool toggled){ g_turn_r = toggled; });

    /* Driver Door */
    connect(ui->pushButton_lfDoorButton, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_drv_door = toggled; });

    /* Passenger Door */
    connect(ui->pushButton_rfDoorButton, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_psg_door = toggled; });

    /* Rear Driver Door */
    connect(ui->pushButton_lrDoorButton, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_rdrv_door = toggled; });

    /* Rear Passenger Door */
    connect(ui->pushButton_rrDoorButton, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_rpsg_door = toggled; });
    
    /* Hood */
    connect(ui->pushButton_hoodButton, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_hood = toggled; });

    /* Boot */
    connect(ui->pushButton_bootButton, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_boot = toggled; });

    /* ACC Status */
    connect(ui->pushButton_accStatus, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_acc_status = toggled; });

    /* ACC Standby */
    connect(ui->checkBox_accStandby, QOverload<bool>::of(&QCheckBox::toggled),
        [this](bool toggled) { g_acc_standby = toggled; });

    /* ACC Simulate Distance */
    connect(ui->spinBox_accSimulateDistance, QOverload<int>::of(&QSpinBox::valueChanged),
        [this](int i) { g_acc_distance2 = i; });

    /* ACC Set Distance */
    connect(ui->spinBox_accSetDistance, QOverload<int>::of(&QSpinBox::valueChanged),
        [this](int i) { g_acc_distance = i; });

    /* Cruise and limit speed */
    connect(ui->spinBox_cruise, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i) { g_cruise = i; });

    /* Alarm */
    connect(ui->pushButton_alarmSound, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_alarm = toggled; });

    /* Dimming */
    connect(ui->dial_Dimming, QOverload<int>::of(&QSlider::valueChanged),
        [this](int i) { g_dimming = i; });

    /* External temperature */
    connect(ui->spinBox_externalTemp, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i) { g_external_temp = i; });
}

void FormCar::start() {
    /* reset dash */
    resetDash(static_cast<CanController*>(static_cast<CanController*>(controller.get())));
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    /* start threads */
    t_ignition_miscellaneous->start();
    t_speed_rpm->start();
    t_fuel_temp->start();
    t_eng_temp->start();
    t_turn->start();
    t_acc->start();
    t_acc2->start();
    t_alarm->start();
    t_dimming->start();
    t_external_temp->start();
}

void FormCar::stop() {
    /* start threads */
    t_ignition_miscellaneous->requestInterruption();
    t_speed_rpm->requestInterruption();
    t_fuel_temp->requestInterruption();
    t_eng_temp->requestInterruption();
    t_turn->requestInterruption();
    t_acc->requestInterruption();
    t_acc2->requestInterruption();
    t_alarm->requestInterruption();
    t_dimming->requestInterruption();
    t_external_temp->requestInterruption();

    t_ignition_miscellaneous->wait();
    t_speed_rpm->wait();
    t_fuel_temp->wait();
    t_eng_temp->wait();
    t_turn->wait();
    t_acc->wait();
    t_acc2->wait();
    t_alarm->wait();
    t_dimming->wait();
    t_external_temp->wait();
}
