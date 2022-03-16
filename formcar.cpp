#include "formcar.h"
#include <QRegExpValidator>
#include <QRegExp>
#include <utility>
#include <QTimer>
#include "ui_formcar.h"
#include "can_sym.h"

FormCar::FormCar(std::unique_ptr<CanController> controller, QWidget *parent):
        QMainWindow(parent),
        ui(new Ui::FormCar),
        controller(std::move(controller))
{
    connect(this, &FormCar::signalLog, this, &FormCar::slotLog, Qt::QueuedConnection);

    m_sym_init_t = std::thread([this](){
        setupSimulator();
        start();
    });

    setupGui();
}

FormCar::~FormCar()
{
    if (m_sym_init_t.joinable())
        m_sym_init_t.join();

    stop();
	delete ui;
}

void FormCar::setupSimulator() {

    controller->set_protocol(CanController::CAN_MS);

    /* ignition and miscellaneus */
    m_tm.addThread([&] {
        package_080(controller.get(),
                    g_drv_door, g_psg_door, g_rdrv_door, g_rpsg_door, g_hood, g_boot, g_head_lights,
                    g_cruise && g_cruise_on, g_cruise && g_cruise_standby, g_acc_on);
    });

    /* speed & rpm */
    m_tm.addThread([&] {
        package_110_EngineRpmAndSpeed(controller.get(), g_rpm, g_speed, g_speed_warning);
    });

    /* fuel */
    m_tm.addThread([&] {
        package_320_FuelLevel(controller.get(), g_fuel);
    });

    /* eng temp */
    m_tm.addThread([&] {
        package_360_EngineTemp(controller.get(), g_eng_temp);
    });

    /* Turns */
    m_tm.addThread([&] {
        package_03A(controller.get(), g_turn_flag && (g_turn_l || g_hazard),
                    g_turn_flag && (g_turn_r || g_hazard), g_cruise_speed);

        g_turn_flag = !g_turn_flag;
    });

    /* ACC Set Distance */
    m_tm.addThread([&] {
        package_070_accSetDistance(controller.get(), g_acc_distance, g_acc_distance2,
                                   g_cruise && g_acc_on, g_cruise_standby);
    });

    /* ACC Simulate Distance */
    m_tm.addThread([&] {
        package_020_accSimulateDistance(controller.get(),
                                        g_cruise && g_acc_on, g_cruise_standby);
    });

    /* Alarm Sound and ParkPilot status */
    m_tm.addThread([&] {
        package_300_playAlarm(controller.get(), g_alarm);
    });

    /* Brake status, lamps status, LCD Dimming(???) */
    m_tm.addThread([&] {
        package_290(controller.get(), g_dimming);
    });

    /* External temperature */
    m_tm.addThread([&] {
        package_1A4_2A0_OutsideTemp(controller.get(), g_external_temp);
    });

    /* DPF Manager */
    m_tm.addThread([&] {
        package_083_dpfStatus(controller.get(), g_dpf_full, g_dpf_regen);
    });

    /* Battery status */
    m_tm.addThread([&] {
        package_508(controller.get(), g_batt_fail);
    });

    /* Engine status */
    m_tm.addThread([&] {
        package_250(controller.get(), g_oil_fail, g_engine_fail);
    });

    /* Park brake */
    m_tm.addThread([&] {
        package_240(controller.get(), g_brake);
    });

    /* Airbag status */
    m_tm.addThread([&] {
        package_040(controller.get(), 0, 0, 0);
    });

    /* Immobilizer status */
    m_tm.addThread([&] {
        package_1E0(controller.get(), 0);
    });

    /* Hill assist status */
    m_tm.addThread([&] {
        package_1B0_Lim(controller.get(), g_cruise && g_limit_on,
                        g_cruise && g_cruise_standby, 0);
    });

    /* High beam, rear fog, Average\instant fuel, shift advice */
    m_tm.addThread([&] {
        package_1A8(controller.get(), g_high_beam, g_rear_fog, 0, 0, 0);
    });

    /* Send custom package */
    m_tm.addThread([&] {
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

            package_debug(controller.get(), id, out_hex);
        }
    });
}

void FormCar::slotLog(const QString &str) {
    static bool f;
    ui->plainTextEdit_log->setTextBackgroundColor((f = !f, f ? QColor(230,230,230) : Qt::white));
    ui->plainTextEdit_log->append(str);
}

void FormCar::write(const char *msg) {
    emit signalLog(QString(msg));
}

void FormCar::setupGui() {

    ui->setupUi(this);

    ui->groupBox_log->setVisible(false);
    ui->groupBox_debug->setVisible(false);
    ui->groupBox_failLamps->setVisible(false);

    adjustSize();

    auto *v = new QRegExpValidator(QRegExp("[a-fA-F0-9]*"), this);
    ui->lineEdit_debugID->setValidator(v);
    ui->lineEdit_debugData->setValidator(v);

    /* Debug */
    connect(ui->actionShow_dbg, QOverload<bool>::of(&QAction::toggled),
            [this](bool showDebug)
            {
                ui->groupBox_debug->setVisible(showDebug);
                if(!showDebug) {
                    ui->pushButton_debugSend->setChecked(false);
                    QTimer::singleShot(0, this, &FormCar::adjustSize);
                }
            });

    /* Log show */
    ui->plainTextEdit_log->setFont(QFont("monospace"));

    connect(ui->actionShow_log, QOverload<bool>::of(&QAction::toggled),
            [this](bool showLog)
            {
                ui->groupBox_log->setVisible(showLog);
                if(!showLog) {
                    ui->pushButton_logEnable->toggle();
                    QTimer::singleShot(0, this, &FormCar::adjustSize);
                }
            });

    /* Log enable */
    connect(ui->pushButton_logEnable, QOverload<bool>::of(&QPushButton::toggled),[this](bool enableLog)
    {
        if(enableLog) {
            controller->set_logger(this);
        } else {
            controller->remove_logger();
        }
    });

    /* Log clear */
    connect(ui->pushButton_logClear, QOverload<bool>::of(&QPushButton::clicked),[this](bool clearLog)
    {
        ui->plainTextEdit_log->clear();
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

    /* Hazard lights */
    connect(ui->pushButton_hazardLights, QOverload<bool>::of(&QPushButton::toggled),
            [this](bool toggled){ g_hazard = toggled; });

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

    /* Cruise and limit */
    connect(ui->pushButton_cruiseEnable, QOverload<bool>::of(&QPushButton::toggled),
            [this](bool toggled) { g_cruise = toggled; });

    connect(ui->checkBox_cruiseStandby, QOverload<bool>::of(&QCheckBox::toggled),
            [this](bool toggled) { g_cruise_standby = toggled; });

    connect(ui->radioButton_cruise, QOverload<bool>::of(&QRadioButton::toggled),
            [this](bool toggled) { g_cruise_on = toggled; });

    connect(ui->radioButton_limit, QOverload<bool>::of(&QRadioButton::toggled),
            [this](bool toggled) { g_limit_on = toggled; });

    /* ACC Status */
    connect(ui->checkBox_accStatus, QOverload<bool>::of(&QCheckBox::toggled),
            [this](bool toggled) { g_acc_on = toggled; });

    /* ACC Simulate Distance */
    connect(ui->spinBox_accSimulateDistance, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i) { g_acc_distance2 = i; });

    /* ACC Set Distance */
    connect(ui->spinBox_accSetDistance, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i) { g_acc_distance = i; });

    /* Cruise and limit speed */
    connect(ui->spinBox_cruiseSpeed, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int i) { g_cruise_speed = i; });

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
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // measure time
    const int test_cnt = 5;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_cnt; i++) {
        std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        controller->transaction(0x000, data);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto us_int = duration_cast<std::chrono::microseconds>(t2 - t1);

    auto avg_send_package_time = us_int.count() / test_cnt;
    auto thread_interval = avg_send_package_time * (m_tm.threadsCount());

    m_tm.setThreadsInterval(std::chrono::microseconds(thread_interval) + 10ms);

    printf("AVG send package time %ld usec, thread interval %ld usec, thread count %ld\n", avg_send_package_time,
           thread_interval, m_tm.threadsCount());

    /* start threads */
    m_tm.startThreads();
}

void FormCar::stop() {
    m_tm.stopThreads();
}
