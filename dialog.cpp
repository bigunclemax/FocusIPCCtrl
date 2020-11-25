//
// Created by user on 29.10.2020.
//
#include <QSerialPortInfo>
#include "dialog.h"
#include "ui_dialog.h"
#include <iostream>

Dialog::Dialog(QWidget *parent)
    :QDialog(parent),
    ui(new Ui::Dialog)
{
    auto serialEnumerator = []() {
        const auto infos = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &info : infos) {
            QString s = QObject::tr("Port: ") + info.portName() + "\n"
                        + QObject::tr("Location: ") + info.systemLocation() + "\n"
                        + QObject::tr("Description: ") + info.description() + "\n"
                        + QObject::tr("Manufacturer: ") + info.manufacturer() + "\n"
                        + QObject::tr("Serial number: ") + info.serialNumber() + "\n"
                        + QObject::tr("Vendor Identifier: ") + (info.hasVendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString()) + "\n"
                        + QObject::tr("Product Identifier: ") + (info.hasProductIdentifier() ? QString::number(info.productIdentifier(), 16) : QString()) + "\n";

            std::cerr << s.toStdString() << std::endl;
        }
        return infos;
    };

    auto refreshComPortsList = [serialEnumerator, this]() {
        auto serial_ports = serialEnumerator();
        QStringList port_names;
        for(const auto &port : serial_ports) {
            port_names.push_back(port.portName());
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
