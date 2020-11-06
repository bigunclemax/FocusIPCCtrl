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
    [[noreturn]] void run() override {
        while (true) {
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

private:
	Ui::FormCar *ui;

	void setupSimulator();

    std::unique_ptr<CanController> controller;
    IPCthread       *t_ignition_doors{};
    IPCthread       *t_speed_rpm{};
    IPCthread       *t_eng_temp{};
    IPCthread       *t_fuel_temp{};
    IPCthread       *t_turn{};
    int  g_rpm          =0;
    int  g_speed        =0;
    int  g_eng_temp     =0;
    int  g_fuel         =0;
    bool g_turn_l       = false;
    bool g_turn_r       = false;
    bool g_turn_flag    = false;
    bool g_drv_door     = false;
    bool g_psg_door     = false;
    bool g_rdrv_door    = false;
    bool g_rpsg_door    = false;
    bool g_hood         = false;
    bool g_boot         = false;

};

#endif // FORMCAR_H
