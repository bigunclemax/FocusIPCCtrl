#ifndef FORMCAR_H
#define FORMCAR_H

#include <QMainWindow>
#include <QWidget>
#include <QThread>
#include <memory>
#include <thread>
#include "controllers/CanController.h"

#include "threads_manager.h"
#include "scheduler.h"

namespace Ui {
class FormCar;
}

class FormCar : public QMainWindow, public CanLogger
{
	Q_OBJECT
signals:
    void signalLog(QString str);

private slots:
    void slotLog(const QString &str);

public:
	explicit FormCar(std::unique_ptr<CanController> controller, QWidget *parent = nullptr);
	~FormCar() override;
	void start();
    void stop();

private:
	Ui::FormCar *ui;

    void setupGui();
	void setupSimulator();

    void write(const char *msg) override;

    ThreadsManager m_tm;
    std::unique_ptr<CanController> m_controller;
    std::unique_ptr<Scheduler> m_scheduler;
    std::thread m_sym_init_t;

    int g_rpm            = 0;
    int g_speed          = 0;
    int g_eng_temp       = 0;
    int g_fuel           = 0;
    int g_dimming        = 1;
    int g_external_temp  = 0;
    /* turns */
    bool g_turn_l        = false;
    bool g_turn_r        = false;
    bool g_hazard        = false;
    bool g_turn_flag     = false;
    /* doors */
    bool g_drv_door      = false;
    bool g_psg_door      = false;
    bool g_rdrv_door     = false;
    bool g_rpsg_door     = false;
    bool g_hood          = false;
    bool g_boot          = false;
    /* limit and cruise */
    bool g_cruise        = false;
    bool g_cruise_standby = false;
    bool g_acc_on    = false;
    bool g_cruise_on     = true;
    bool g_limit_on      = false;
    int g_cruise_speed   = 0;
    int g_acc_distance   = 0;
    int g_acc_distance2  = 0;

    bool g_speed_warning = false;
    bool g_alarm         = false;
    bool g_dpf_full      = false;
    bool g_dpf_regen     = false;
    bool g_batt_fail     = false;
    bool g_oil_fail      = false;
    bool g_engine_fail   = false;
    bool g_airbag_fail   = false;
    bool g_brake         = false;
    /* lights */
    bool g_rear_fog      = false;
    bool g_high_beam     = false;
    bool g_head_lights   = false;

    bool g_debug         = false;

};

#endif // FORMCAR_H
