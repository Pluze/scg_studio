#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow (QWidget* parent)
  : QMainWindow (parent)
  , ui (new Ui::MainWindow) {
  m_ecgSeries = nullptr;
  m_ecgChart = nullptr;

  m_scgSeries = nullptr;
  m_scgChart = nullptr;

  m_serialPort = nullptr;
  m_udpSocket = nullptr;

  isdeviceconnect = false;
  iscollectingsignal = false;

  millis = 0;
  sensor1Value = 0;
  sensor2Value = 0;
  m_maxCacheSize = 1000;

  ui->setupUi (this);

  m_ecgSeries = new QLineSeries();
  m_ecgChart = new QChart();
  m_ecgChart->addSeries (m_ecgSeries);
  ui->m_ecgChartView->setChart (m_ecgChart);

  m_scgSeries = new QLineSeries();
  m_scgChart = new QChart();
  m_scgChart->addSeries (m_scgSeries);
  ui->m_scgChartView->setChart (m_scgChart);

  // 定义一个 QTimer 对象
  m_timer = new QTimer (this);

  connect (ui->refreshPushButton, &QPushButton::clicked, this, &MainWindow::refreshSerialDevice);
  connect (ui->connectPushButton, &QPushButton::clicked, this, &MainWindow::openSerialPort);
  ui->refreshPushButton->click();
  connect (ui->collectPushButton, &QPushButton::clicked, this, &MainWindow::openUdpPort);

  connect (ui->rebootPushButton, &QPushButton::clicked, this, [this]() {
    sendSerialData ('r');
  });
  connect (ui->sendingPushButton, &QPushButton::clicked, this, [this]() {
    sendSerialData ('s');
  });
  connect (ui->stopPushButton, &QPushButton::clicked, this, [this]() {
    sendSerialData ('e');
  });
  connect (ui->batteryPushButton, &QPushButton::clicked, this, [this]() {
    sendSerialData ('b');
  });
  connect (ui->sysInfoPushButton, &QPushButton::clicked, this, [this]() {
    sendSerialData ('?');
  });
  connect (ui->clearPushButton, &QPushButton::clicked, this, [this]() {
    ui->textBrowser->clear();
  });
}

MainWindow::~MainWindow() {
  if (!m_messageCache.isEmpty()) {
    writeCacheToFile();
  }
  if (m_ecgSeries) {
    delete m_ecgSeries;
  }
  if (m_ecgChart) {
    delete m_ecgChart;
  }
  if (m_scgSeries) {
    delete m_scgSeries;
  }
  if (m_scgChart) {
    delete m_scgChart;
  }
  if (m_serialPort) {
    delete m_serialPort;
  }
  if (m_udpSocket) {
    delete m_udpSocket;
  }
  if (m_timer) {
    delete m_timer;
  }
  delete ui;
}

void MainWindow::updateData() {
  static int lastmillis = 0;
  if (millis == 0 || lastmillis == millis) {
    return;
  }
  lastmillis = millis;
  // 添加数据到曲线中
  m_ecgSeries->append (millis, sensor1Value);
  // 如果曲线点数超过一定数量，就删除最前面的数据
  if (m_ecgSeries->count() > 1000) {
    m_ecgSeries->remove (0);
  }
  m_ecgChart->removeSeries (m_ecgSeries);
  m_ecgChart->addSeries (m_ecgSeries);
  m_ecgChart->createDefaultAxes();
  m_ecgChart->title();
  ui->m_ecgChartView->update();

  // 添加数据到曲线中
  m_scgSeries->append (millis, sensor2Value);
  // 如果曲线点数超过一定数量，就删除最前面的数据
  if (m_scgSeries->count() > 1000) {
    m_scgSeries->remove (0);
    qDebug() << m_scgSeries->count();
  }
  m_scgChart->removeSeries (m_scgSeries);
  m_scgChart->addSeries (m_scgSeries);
  m_scgChart->createDefaultAxes();
  m_scgChart->title();
  ui->m_scgChartView->update();
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

  for (const QSerialPortInfo& portInfo : portList) {
    ui->deviceComboBox->addItem (portInfo.portName());
  }

  // 将下拉框中的内容转换为QStringList
  QStringList items;
  for (int i = 0; i < ui->deviceComboBox->count(); ++i) {
    items.append (ui->deviceComboBox->itemText (i));
  }
  // 将逆序排列后的内容重新设置到下拉框中
  ui->deviceComboBox->clear();
  for (int i = items.count() - 1; i >= 0; --i) {
    ui->deviceComboBox->addItem (items.at (i));
  }


}

void MainWindow::openSerialPort() {
  QList<QPushButton*> buttonList = {ui->rebootPushButton, ui->batteryPushButton, ui->sendingPushButton,  ui->sysInfoPushButton, ui->stopPushButton};
  if (isdeviceconnect) {
    disconnect (m_serialPort, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
    delete m_serialPort;
    m_serialPort = nullptr;
    ui->connectPushButton->setText ("connect");
    isdeviceconnect = false;
    for (auto button : buttonList) {
      button->setEnabled (false);
    }
    return;
  }

  QString portName = ui->deviceComboBox->currentText();
  if (portName.isEmpty()) {
    QMessageBox::warning (this, tr ("Error"), tr ("Please check serial port."));
    return;
  }
  m_serialPort = new QSerialPort (this);
  m_serialPort->setPortName (portName);
  m_serialPort->setBaudRate (QSerialPort::Baud115200);
  m_serialPort->setDataBits (QSerialPort::Data8);
  m_serialPort->setParity (QSerialPort::NoParity);
  m_serialPort->setStopBits (QSerialPort::OneStop);
  if (!m_serialPort->open (QIODevice::ReadWrite)) {
    QMessageBox::critical (this, tr ("Error"), tr ("Failed to open serial port."));
    return;
  }
  connect (m_serialPort, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
  ui->textBrowser->clear();
  QDateTime currentDateTime = QDateTime::currentDateTime();
  QString currentDateTimeString = currentDateTime.toString ("[yyyy-MM-dd hh:mm:ss]") + " connect successful\n";
  ui->textBrowser->insertPlainText (currentDateTimeString);
  ui->connectPushButton->setText ("disconnect");
  isdeviceconnect = true;
  for (auto button : buttonList) {
    button->setEnabled (true);
  }
}

void MainWindow::readSerialData() {
  QByteArray data = m_serialPort->readAll();
  ui->textBrowser->insertPlainText (data);
  QScrollBar* sb = ui->textBrowser->verticalScrollBar();
  sb->setValue (sb->maximum());
}

void MainWindow::sendSerialData (char a) {
  m_serialPort->write (&a);
}

void MainWindow::openUdpPort() {
  if (iscollectingsignal) {
    disconnect (m_udpSocket, &QUdpSocket::readyRead, this, &MainWindow::processUDPdata);
    delete m_udpSocket;
    m_udpSocket = nullptr;
    ui->collectPushButton->setText ("collect");
    iscollectingsignal = false;
    m_timer->stop();
    disconnect (m_timer, &QTimer::timeout, this, &MainWindow::updateData);
    return;
  }
  // 创建UDP套接字
  m_udpSocket = new QUdpSocket (this);
  // 绑定端口，监听广播消息
  m_udpSocket->bind (QHostAddress::AnyIPv4, 3333, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
  connect (m_udpSocket, &QUdpSocket::readyRead, this, &MainWindow::processUDPdata);
  iscollectingsignal = true;
  ui->collectPushButton->setText ("end collect");
  // 连接定时器的timeout信号和槽函数
  connect (m_timer, &QTimer::timeout, this, &MainWindow::updateData);
  // 启动定时器
  m_timer->start (1000 / 144);
}

void MainWindow::processUDPdata() {

  while (m_udpSocket->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize (m_udpSocket->pendingDatagramSize());
    m_udpSocket->readDatagram (datagram.data(), datagram.size());

    QString dataString (datagram);
    QStringList dataList = dataString.split (",");

    if (dataList.size() == 3) {
      m_messageCache.append (dataString);
      millis = (dataList[0].toDouble() / 1000.);
      sensor1Value = dataList[1].toDouble();
      sensor2Value = dataList[2].toDouble();
//            int seconds = millis / 1000; // 获取秒数部分
//            int hours = seconds / 3600; // 获取小时数
//            int minutes = (seconds / 60) % 60; // 获取分钟数
//            seconds = seconds % 60; // 获取剩余的秒数
//            QTime time(hours, minutes, seconds); // 创建QTime对象
//            QString timeStr = time.toString("hh:mm:ss"); // 将QTime对象转换为字符串，格式为hh:mm:ss

//            qDebug() << hours << ":" << minutes << ":" << seconds << timeStr;
//            qDebug() << dataList[0].toDouble() / 1000;
//            qDebug() << sensor1Value;
//            qDebug() << sensor2Value;
      if (m_messageCache.size() >= m_maxCacheSize) {
        writeCacheToFile();
      }

    }
  }
}

void MainWindow::writeCacheToFile() {
  // 打开文件，如果文件不存在则创建
  QFile file ("D:/message.txt");
  if (!file.open (QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    qWarning() << "Failed to open file";
    return;
  }

  // 将内存缓存中的字符串信息写入文件
  QTextStream out (&file);
  for (const QString& message : m_messageCache) {
    out << message << "\n";
  }

  // 清空内存缓存
  m_messageCache.clear();
}
