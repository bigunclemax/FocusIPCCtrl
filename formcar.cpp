#include "formcar.h"

#include <utility>
#include "ui_formcar.h"
#include "can_sym.h"

FormCar::FormCar(std::unique_ptr<CanController> controller, QWidget *parent):
        QWidget(parent),
        ui(new Ui::FormCar),
        controller(std::move(controller))
{
    setupGui();
    setupSimulator();
    start();
}

FormCar::~FormCar()
{
    stop();
	delete ui;
}

void FormCar::addThread(std::function<void(void)> f, unsigned long interval) {
    auto& t = m_threads.emplace_back(std::make_unique<IPCthread>(interval));
    t->registerCallback(std::move(f));
}

void FormCar::startThreads() {
    for(auto &t :m_threads)
        t->start();
}

void FormCar::stopThreads() {
    for(auto &t :m_threads) {
        t->requestInterruption();
        t->wait();
    }
}

void FormCar::setupSimulator() {

    controller->set_protocol(CanController::CAN_MS);

    /* ignition and miscellaneus */
    addThread([&] {
        fakeIgnitionMiscellaneous(static_cast<CanController*>(controller.get()), g_drv_door, g_psg_door, g_rdrv_door, g_rpsg_door, g_hood, g_boot,
                                  g_acc_status, g_acc_standby);
    });

    /* speed & rpm */
    addThread([&] {
        fakeEngineRpmAndSpeed(static_cast<CanController*>(controller.get()), g_rpm, g_speed, g_speed_warning);
    });

    /* fuel */
    addThread([&] {
        fakeFuel(static_cast<CanController*>(controller.get()), g_fuel);
    });

    /* eng temp */
    addThread([&] {
        fakeEngineTemp(static_cast<CanController*>(controller.get()), g_eng_temp);
    });

    /* Turns */
    addThread([&] {
        if(g_turn_flag)
            fakeTurn(static_cast<CanController*>(controller.get()), g_turn_l, g_turn_r, g_cruise);
        else
            fakeTurn(static_cast<CanController*>(controller.get()), false, false, g_cruise);

        g_turn_flag = !g_turn_flag;
    });

    /* ACC Set Distance */
    addThread([&] {
        accSetDistance(static_cast<CanController*>(controller.get()), g_acc_distance, g_acc_distance2, g_acc_status, g_acc_standby);
    });

    /* ACC Simulate Distance */
    addThread([&] {
        accSimulateDistance(static_cast<CanController*>(controller.get()), g_acc_status, g_acc_standby);
    });

    /* Play Alarm Sound */
    addThread([&] {
        playAlarm(static_cast<CanController*>(controller.get()), g_alarm);
    });

    /* LCD Dimming */
    addThread([&] {
        changeDimming(static_cast<CanController*>(controller.get()), g_dimming);
    });

    /* External temperature */
    addThread([&] {
        fakeExternalTemp(static_cast<CanController*>(controller.get()), g_external_temp);
    });

    /* DPF Manager */
    addThread([&] {
        dpfStatus(static_cast<CanController*>(controller.get()), g_dpf_full, g_dpf_regen);
    });
}

void FormCar::setupGui() {

    ui->setupUi(this);

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

    /* DPF Full */
    connect(ui->pushButton_dpfFull, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_dpf_full = toggled; });

    /* DPF Regen */
    connect(ui->pushButton_dpfRegen, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_dpf_regen = toggled; });
}

void FormCar::start() {
    /* reset dash */
    resetDash(static_cast<CanController*>(static_cast<CanController*>(controller.get())));
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    /* start threads */
    startThreads();
}

void FormCar::stop() {
    /* start threads */
    stopThreads();
}
