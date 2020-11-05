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

    ui->setupUi(this);

    connect(ui->comboBox_adapterType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [serialEnumerator, this](int index)
    {
        m_selectedType = static_cast<enControllerType>(index);
        if(m_selectedType == els27) {

            auto serial_ports = serialEnumerator();
            QStringList port_names;
            for(const auto &port : serial_ports) {
                port_names.push_back(port.portName());
            }
            ui->comboBox_comPorts->addItems(port_names);
        }
    });

    connect(ui->pushButton_OK, &QPushButton::clicked, this, [this]() {
        this->close();
        if(m_selectedType == els27) {
            m_settings.port_name = ui->comboBox_comPorts->currentText().toStdString();
            m_settings.autodetect = ui->checkBox_autodetect->isChecked();
            m_settings.maximize = ui->checkBox_maximize->isChecked();
            m_settings.baudrate = ui->spinBox_baudrate->value();
        }
        emit btnOk_click();
    });

    connect(ui->checkBox_autodetect, &QCheckBox::toggled, this, [this](bool checked) {
        ui->spinBox_baudrate->setEnabled(!checked);
    });

    ui->spinBox_baudrate->setValue(38400);
    ui->comboBox_adapterType->addItem(ToString(els27));

}
