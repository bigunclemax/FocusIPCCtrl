//
// Created by user on 29.10.2020.
//

#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#include <QtWidgets>

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog
{
Q_OBJECT

public:

    enum enControllerType {
        els27  =0,
    };

    inline const char* ToString(enControllerType v)
    {
        switch (v)
        {
            case els27:   return "els27";
            default:      return "";
        }
    }

    struct settings {
        std::string port_name;
        int         baudrate{};
        bool        maximize{};
        bool        autodetect{};
    };

    explicit Dialog(QWidget *parent = nullptr);
    settings getSettings(enControllerType type) const { return m_settings; };
    enControllerType getControllerType() const { return els27; };

private:
    Ui::Dialog *ui;
    enControllerType m_selectedType;
    settings m_settings;

signals:
    void btnOk_click();

};

#endif //GUI_DIALOG_H
