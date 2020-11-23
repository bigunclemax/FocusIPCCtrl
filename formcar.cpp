#include "formcar.h"
#include <QRegExpValidator>
#include <QRegExp>
#include <utility>
#include "ui_formcar.h"
#include "can_sym.h"

FormCar::FormCar(std::unique_ptr<CanController> controller, QWidget *parent):
        QMainWindow(parent),
        ui(new Ui::FormCar),
        controller(std::move(controller))
{
    connect(this, &FormCar::signalLog, this, &FormCar::slotLog, Qt::QueuedConnection);
    m_sym_init_t = std::make_unique<IPCthread>(0);
    m_sym_init_t->registerCallback([&] {
        setupSimulator();
        start();
    });
    m_sym_init_t->start();

    setupGui();
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
                                  g_acc_status, g_acc_standby, g_head_lights);
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

    /* Brake status, lamps status, LCD Dimming(???) */
    addThread([&] {
        package_290(static_cast<CanController *>(controller.get()), g_dimming, false);
    });

    /* External temperature */
    addThread([&] {
        fakeExternalTemp(static_cast<CanController*>(controller.get()), g_external_temp);
    });

    /* DPF Manager */
    addThread([&] {
        dpfStatus(static_cast<CanController*>(controller.get()), g_dpf_full, g_dpf_regen);
    });

    /* Battery status */
    addThread([&] {
        package_508(static_cast<CanController *>(controller.get()), g_batt_fail);
    });

    /* Engine status */
    addThread([&] {
        package_250(static_cast<CanController*>(controller.get()), g_oil_fail, g_engine_fail);
    });

    /* Park brake */
    addThread([&] {
        package_240(static_cast<CanController*>(controller.get()), g_brake);
    });

    /* Airbag status */
    addThread([&] {
        package_040(static_cast<CanController *>(controller.get()), 0, 0, 0);
    });

    /* Immobilizer status */
    addThread([&] {
        package_1e0(static_cast<CanController *>(controller.get()), 0);
    });

    /* Hill assist status */
    addThread([&] {
        package_1b0(static_cast<CanController *>(controller.get()), 0);
    });

    /* High beam, rear fog, Average\instant fuel, shift advice */
    addThread([&] {
        package_1a8(static_cast<CanController *>(controller.get()), g_high_beam, g_rear_fog, 0, 0, 0);
    });

    /* Average\instant fuel, shift advice */
    addThread([&] {
        if (g_debug) {
            bool bStatus = false;
            auto id = ui->lineEdit_debugID->text().toUInt(&bStatus,16);
            if(!bStatus) return;
            auto str_data = ui->lineEdit_debugData->text();
            if(str_data.length() != 16) return;
            auto in_str = str_data.toStdString();
            auto asciiHexToInt = [](uint8_t hexchar)->int {return (hexchar >= 'A') ? (hexchar - 'A' + 10) : (hexchar - '0');};
            std::vector<uint8_t> out_hex(8);
            out_hex[0] = asciiHexToInt(in_str[0]) << 4 | asciiHexToInt(in_str[1]);
            out_hex[1] = asciiHexToInt(in_str[2]) << 4 | asciiHexToInt(in_str[3]);
            out_hex[2] = asciiHexToInt(in_str[4]) << 4 | asciiHexToInt(in_str[5]);
            out_hex[3] = asciiHexToInt(in_str[6]) << 4 | asciiHexToInt(in_str[7]);
            out_hex[4] = asciiHexToInt(in_str[8]) << 4 | asciiHexToInt(in_str[9]);
            out_hex[5] = asciiHexToInt(in_str[10]) << 4 | asciiHexToInt(in_str[11]);
            out_hex[6] = asciiHexToInt(in_str[12]) << 4 | asciiHexToInt(in_str[13]);
            out_hex[7] = asciiHexToInt(in_str[14]) << 4 | asciiHexToInt(in_str[15]);

            package_debug(static_cast<CanController *>(controller.get()), id, out_hex);
        }
    });
}

void FormCar::slotLog(const QString &str) {
    ui->plainTextEdit_log->insertPlainText(str);
}

void FormCar::write(const char *msg) {
    emit signalLog(QString(msg));
}

void FormCar::setupGui() {

    ui->setupUi(this);

    auto *v = new QRegExpValidator(QRegExp("[a-fA-F0-9]*"), this);
    ui->lineEdit_debugID->setValidator(v);
    ui->lineEdit_debugData->setValidator(v);

    ui->plainTextEdit_log->setVisible(false);
    /* Show logs */
    connect(ui->actionShow_log, QOverload<bool>::of(&QAction::toggled),
            [this](bool showLog)
            {
                ui->plainTextEdit_log->setVisible(showLog);
                if(showLog) {
                    controller->set_logger(this);
                } else {
                    controller->remove_logger();
                }
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

    /* DPF Full */
    connect(ui->pushButton_dpfFull, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_dpf_full = toggled; });

    /* DPF Regen */
    connect(ui->pushButton_dpfRegen, QOverload<bool>::of(&QPushButton::toggled),
        [this](bool toggled) { g_dpf_regen = toggled; });

    /* Battery status */
    connect(ui->checkBox_battery, QOverload<bool>::of(&QCheckBox::toggled),
            [this](bool toggled) { g_batt_fail = toggled; });

    /* Oil status */
    connect(ui->checkBox_oil, QOverload<bool>::of(&QCheckBox::toggled),
            [this](bool toggled) { g_oil_fail = toggled; });

    /* Engine status */
    connect(ui->checkBox_engine, QOverload<bool>::of(&QCheckBox::toggled),
            [this](bool toggled) { g_engine_fail = toggled; });

    /* Brake status */
    connect(ui->pushButton_parkBrake, QOverload<bool>::of(&QCheckBox::toggled),
            [this](bool toggled) { g_brake = toggled; });

    /* Airbag status */
    connect(ui->checkBox_airbag, QOverload<bool>::of(&QCheckBox::toggled),
            [this](bool toggled) { g_airbag_fail = toggled; });

    /* High beam */
    connect(ui->pushButton_HighBeam, QOverload<bool>::of(&QPushButton::toggled),
            [this](bool toggled) { g_high_beam = toggled; });

    /* Rear fog */
    connect(ui->pushButton_RearFogLights, QOverload<bool>::of(&QPushButton::toggled),
            [this](bool toggled) { g_rear_fog = toggled; });

    /* Low beam */
    connect(ui->pushButton_HeadLights, QOverload<bool>::of(&QPushButton::toggled),
            [this](bool toggled) { g_head_lights = toggled; });

    /* Debug */
    connect(ui->pushButton_debugSend, QOverload<bool>::of(&QPushButton::toggled),
            [this](bool toggled) { g_debug = toggled; });
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
