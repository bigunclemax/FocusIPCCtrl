#ifndef FORMCAR_H
#define FORMCAR_H

#include <QWidget>
#include <QThread>
#include <memory>
#include <thread>
#include <unistd.h>
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
    IPCthread       *t_ignition{};
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

};

#endif // FORMCAR_H
