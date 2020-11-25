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
        elm327 =1
    };

    inline const char* ToString(enControllerType v)
    {
        switch (v)
        {
            case els27:   return "els27";
            case elm327:  return "elm327";
            default:      return "";
        }
    }

    struct settings {
        enControllerType type;
        std::string port_name;
        int         baudrate{};
        bool        maximize{};
        bool        autodetect{};
    };

    explicit Dialog(QWidget *parent = nullptr);
    [[nodiscard]] settings getSettings() const { return m_settings; };

private:
    Ui::Dialog *ui;
    enControllerType m_selectedType;
    settings m_settings;

signals:
    void btnOk_click();

};

#endif //GUI_DIALOG_H
