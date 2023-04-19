
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    m_ecgSeries = nullptr;
    m_ecgChart = nullptr;

    m_scgSeries = nullptr;
    m_scgChart = nullptr;

    m_serialPort = nullptr;
    m_udpSocket = nullptr;

    isdeviceconnect = false;
    iscollectingsignal = false;

    ui->setupUi(this);

    m_ecgSeries = new QLineSeries();
    m_ecgSeries->append(1, 1);
    m_ecgSeries->append(2, 2);
    m_ecgSeries->append(3, 3);
    m_ecgSeries->append(4, 4);
    m_ecgChart = new QChart();
    m_ecgChart->addSeries(m_ecgSeries);
    ui->m_ecgChartView->setChart(m_ecgChart);

    m_scgSeries = new QLineSeries();
    m_scgSeries->append(4, 1);
    m_scgSeries->append(3, 2);
    m_scgSeries->append(2, 3);
    m_scgSeries->append(1, 4);
    m_scgChart = new QChart();
    m_scgChart->addSeries(m_scgSeries);
    m_scgChart->createDefaultAxes();
    ui->m_scgChartView->setChart(m_scgChart);

    connect(ui->refreshPushButton, &QPushButton::clicked, this, &MainWindow::refreshSerialDevice);
    connect(ui->connectPushButton, &QPushButton::clicked, this, &MainWindow::openSerialPort);
    ui->refreshPushButton->click();
    connect(ui->collectPushButton, &QPushButton::clicked, this, &MainWindow::openUdpPort);

    connect(ui->rebootPushButton, &QPushButton::clicked, this, [this]() {
        sendSerialData('r');
    });
    connect(ui->sendingPushButton, &QPushButton::clicked, this, [this]() {
        sendSerialData('s');
    });
    connect(ui->stopPushButton, &QPushButton::clicked, this, [this]() {
        sendSerialData('e');
    });
    connect(ui->batteryPushButton, &QPushButton::clicked, this, [this]() {
        sendSerialData('b');
    });
    connect(ui->sysInfoPushButton, &QPushButton::clicked, this, [this]() {
        sendSerialData('?');
    });
    connect(ui->clearPushButton, &QPushButton::clicked, this, [this]() {
        ui->textBrowser->clear();
        m_ecgSeries->append(QRandomGenerator::global()->bounded(1, 101), QRandomGenerator::global()->bounded(1, 101));
    });
}

MainWindow::~MainWindow() {
    if(m_ecgSeries) {
        delete m_ecgSeries;
    }
    if(m_ecgChart) {
        delete m_ecgChart;
    }
    if(m_scgSeries) {
        delete m_scgSeries;
    }
    if(m_scgChart) {
        delete m_scgChart;
    }
    if(m_serialPort) {
        delete m_serialPort;
    }
    if(m_udpSocket) {
        delete m_udpSocket;
    }
    delete ui;
}

void MainWindow::refreshSerialDevice() {
    // 清空下拉列表框
    ui->deviceComboBox->clear();

    // 获取当前存在的所有串口
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();

    // 将串口名称添加到下拉列表框中
    // qt宏可能会被废弃，但c++11不会
    //    foreach (const QSerialPortInfo &portInfo, portList)
    //    {
    //        ui->deviceComboBox->addItem(portInfo.portName());
    //    }

    for(const QSerialPortInfo& portInfo : portList) {
        ui->deviceComboBox->addItem(portInfo.portName());
    }

    // 将下拉框中的内容转换为QStringList
    QStringList items;
    for(int i = 0; i < ui->deviceComboBox->count(); ++i) {
        items.append(ui->deviceComboBox->itemText(i));
    }
    // 将逆序排列后的内容重新设置到下拉框中
    ui->deviceComboBox->clear();
    for(int i = items.count() - 1; i >= 0; --i) {
        ui->deviceComboBox->addItem(items.at(i));
    }


}

void MainWindow::openSerialPort() {
    QList<QPushButton*> buttonList = {ui->rebootPushButton, ui->batteryPushButton, ui->sendingPushButton,  ui->sysInfoPushButton, ui->stopPushButton};
    if(isdeviceconnect) {
        disconnect(m_serialPort, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
        delete m_serialPort;
        m_serialPort = nullptr;
        ui->connectPushButton->setText("connect");
        isdeviceconnect = false;
        for(auto button : buttonList) {
            button->setEnabled(false);
        }
        return;
    }

    QString portName = ui->deviceComboBox->currentText();
    if(portName.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please check serial port."));
        return;
    }
    m_serialPort = new QSerialPort(this);
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    if(!m_serialPort->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open serial port."));
        return;
    }
    connect(m_serialPort, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
    ui->textBrowser->clear();
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentDateTimeString = currentDateTime.toString("[yyyy-MM-dd hh:mm:ss]") + " connect successful\n";
    ui->textBrowser->insertPlainText(currentDateTimeString);
    ui->connectPushButton->setText("disconnect");
    isdeviceconnect = true;
    for(auto button : buttonList) {
        button->setEnabled(true);
    }
}

void MainWindow::readSerialData() {
    QByteArray data = m_serialPort->readAll();
    ui->textBrowser->insertPlainText(data);
    QScrollBar *sb = ui->textBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::sendSerialData(char a) {
    m_serialPort->write(&a);
}

void MainWindow::openUdpPort() {
    if(iscollectingsignal) {
        disconnect(m_udpSocket, &QUdpSocket::readyRead, this, &MainWindow::processPendingDatagrams);
        delete m_udpSocket;
        m_udpSocket = nullptr;
        ui->collectPushButton->setText("collect");
        iscollectingsignal = false;
        return;
    }
    // 创建UDP套接字
    m_udpSocket = new QUdpSocket(this);
    // 绑定端口，监听广播消息
    m_udpSocket->bind(QHostAddress::AnyIPv4, 3333, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &MainWindow::processPendingDatagrams);
    iscollectingsignal = true;
    ui->collectPushButton->setText("end collect");
}

void MainWindow::processPendingDatagrams() {

    while(m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size());

        QString dataString(datagram);
        QStringList dataList = dataString.split(",");

        // 将数据添加到波形图中
        if(dataList.size() == 3) {
            int seconds = (dataList[0].toDouble() / 1000000.); // 获取秒数部分
            int hours = seconds / 3600; // 获取小时数
            int minutes = (seconds / 60) % 60; // 获取分钟数
            seconds = seconds % 60; // 获取剩余的秒数
            QTime time(hours, minutes, seconds); // 创建QTime对象
            QString timeStr = time.toString("hh:mm:ss"); // 将QTime对象转换为字符串，格式为hh:mm:ss
            qreal sensor1Value = dataList[1].toDouble();
            qreal sensor2Value = dataList[2].toDouble();

            qDebug() << hours << ":" << minutes << ":" << seconds << timeStr;
            qDebug() << dataList[0].toDouble() / 1000;
            qDebug() << sensor1Value;
            qDebug() << sensor2Value;

            //将数据加入绘图区域

        }
    }
}
