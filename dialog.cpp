//
// Created by user on 29.10.2020.
//
#include "dialog.h"
#include "ui_dialog.h"
#include <iostream>
#include "serial/serial.h"

Dialog::Dialog(QWidget *parent)
    :QDialog(parent),
    ui(new Ui::Dialog)
{

    auto refreshComPortsList = [this]() {
        QStringList port_names;
        for(const auto &port : serial::list_ports()) {
            if ((port.port.find("USB") != std::string::npos) || (port.port.find("COM") != std::string::npos))
                port_names.push_back(port.port.c_str());
        }
        ui->comboBox_comPorts->clear();
        ui->comboBox_comPorts->addItems(port_names);
    };

    ui->setupUi(this);

    connect(ui->comboBox_adapterType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this, refreshComPortsList](int index)
    {
        m_selectedType = static_cast<enControllerType>(index);

        refreshComPortsList();

        if (m_selectedType == elm327) {
            ui->checkBox_maximize->setChecked(false);
            ui->checkBox_maximize->setEnabled(false);
            ui->checkBox_autodetect->setChecked(false);
            ui->checkBox_autodetect->setEnabled(false);
        } else {
            ui->checkBox_maximize->setEnabled(true);
            ui->checkBox_autodetect->setEnabled(true);
        }

    });

    connect(ui->pushButton_OK, &QPushButton::clicked, this, [this]() {
        this->close();
        m_settings.type = m_selectedType;
        m_settings.port_name = ui->comboBox_comPorts->currentText().toStdString();
        m_settings.autodetect = ui->checkBox_autodetect->isChecked();
        m_settings.maximize = ui->checkBox_maximize->isChecked();
        m_settings.baudrate = ui->spinBox_baudrate->value();
        emit btnOk_click();
    });

    connect(ui->checkBox_autodetect, &QCheckBox::toggled, this, [this](bool checked) {
        ui->spinBox_baudrate->setEnabled(!checked);
    });

    connect(ui->comboBox_comPorts, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index){ ui->pushButton_OK->setEnabled(index != -1);});

    connect(ui->pushButton_comRefresh, &QPushButton::clicked, this, [refreshComPortsList, this]() {
        refreshComPortsList();
    });

    ui->spinBox_baudrate->setValue(38400);
    ui->comboBox_adapterType->addItem(ToString(els27));
    ui->comboBox_adapterType->addItem(ToString(elm327));

}
