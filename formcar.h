#ifndef FORMCAR_H
#define FORMCAR_H

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
        while (!isInterruptionRequested()) {
            laterCB();
            usleep(interval);
        }
    }

    template <typename F, typename... Args>
    void registerCallback(F f, Args&&... args)
    {
        laterCB = [=] { f(args...); };
    }

    explicit IPCthread(unsigned long interval) : interval(interval) {};
private:
    std::function<void()> laterCB;
    unsigned long interval;
};


class FormCar : public QWidget
{
	Q_OBJECT

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

    std::unique_ptr<CanController> controller;
    std::vector<std::unique_ptr<IPCthread>> m_threads;

    int g_rpm            = 0;
    int g_speed          = 0;
    int g_eng_temp       = 0;
    int g_fuel           = 0;
    int g_acc_distance   = 0;
    int g_acc_distance2  = 0;
    int g_cruise         = 0;
    int g_dimming        = 1;
    int g_external_temp  = 0;
    bool g_turn_l        = false;
    bool g_turn_r        = false;
    bool g_turn_flag     = false;
    bool g_drv_door      = false;
    bool g_psg_door      = false;
    bool g_rdrv_door     = false;
    bool g_rpsg_door     = false;
    bool g_hood          = false;
    bool g_boot          = false;
    bool g_acc_status    = false;
    bool g_acc_standby   = true;
    bool g_speed_warning = false;
    bool g_alarm         = false;
    bool g_dpf_full      = false;
    bool g_dpf_regen     = false;

};

#endif // FORMCAR_H
