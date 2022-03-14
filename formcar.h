#ifndef FORMCAR_H
#define FORMCAR_H

#include <QMainWindow>
#include <QWidget>
#include <QThread>
#include <memory>
#include <thread>
#include "controllers/CanController.h"

namespace Ui {
class FormCar;
}

class IPCthread : public QThread{
    Q_OBJECT
public:
    void run() override {
        do {
            laterCB();
            usleep(m_interval);
        } while (!isInterruptionRequested() && m_interval);
    }

    template <typename F, typename... Args>
    void registerCallback(F f, Args&&... args)
    {
        laterCB = [=] { f(args...); };
    }

    explicit IPCthread(unsigned long interval) : m_interval(interval) {};
    void setInterval(unsigned long interval) {
        m_interval = interval;
    };
private:
    std::function<void()> laterCB;
    unsigned long m_interval;
};

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

	void addThread( std::function<void(void)> f, unsigned long interval = 250000);
	void startThreads();
    void stopThreads();

    void write(const char *msg) override;

    std::unique_ptr<CanController> controller;
    std::vector<std::unique_ptr<IPCthread>> m_threads;
    std::unique_ptr<IPCthread> m_sym_init_t;

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
