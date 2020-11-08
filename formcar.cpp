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
}

FormCar::~FormCar()
{
    stop();
	delete ui;
}

void FormCar::showEvent( QShowEvent* event ) {
    QWidget::showEvent( event );
    start();
}

void FormCar::setupSimulator() {

    controller->set_protocol(CanController::CAN_MS);

    /* ignition and doors */
    t_ignition_doors = std::make_unique<IPCthread>(250000);
    t_ignition_doors->registerCallback([&] {
            fakeIgnitionDoors(static_cast<CanController*>(controller.get()), g_drv_door, g_psg_door, g_rdrv_door, g_rpsg_door, g_hood, g_boot);
    });

    /* speed & rpm */
    t_speed_rpm = std::make_unique<IPCthread>(250000);
    t_speed_rpm->registerCallback([&]{
        fakeEngineRpmAndSpeed(static_cast<CanController*>(controller.get()), g_rpm, g_speed);
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

    /* turns */
    t_turn = std::make_unique<IPCthread>(250000);
    t_turn->registerCallback([&]{
        if(g_turn_flag)
            fakeTurn(static_cast<CanController*>(controller.get()), g_turn_l, g_turn_r);
        else
            fakeTurn(static_cast<CanController*>(controller.get()), false, false);

        g_turn_flag = !g_turn_flag;
    });

    /* ACC Distance */
    t_acc = std::make_unique<IPCthread>(250000);
    t_acc->registerCallback([&] {
        accDistance(static_cast<CanController*>(controller.get()), g_acc_distance, g_acc_status);
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

    /* ACC Distance */
    connect(ui->spinBox_accDistance, QOverload<int>::of(&QSpinBox::valueChanged),
        [this](int i) { g_acc_distance = i; });
}

void FormCar::start() {
    /* reset dash */
    resetDash(static_cast<CanController*>(static_cast<CanController*>(controller.get())));
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    /* start threads */
    t_ignition_doors->start();
    t_speed_rpm->start();
    t_fuel_temp->start();
    t_eng_temp->start();
    t_turn->start();
    t_acc->start();
}

void FormCar::stop() {
    /* start threads */
    t_ignition_doors->requestInterruption();
    t_speed_rpm->requestInterruption();
    t_fuel_temp->requestInterruption();
    t_eng_temp->requestInterruption();
    t_turn->requestInterruption();
    t_acc->requestInterruption();

    t_ignition_doors->wait();
    t_speed_rpm->wait();
    t_fuel_temp->wait();
    t_eng_temp->wait();
    t_turn->wait();
    t_acc->wait();
}
