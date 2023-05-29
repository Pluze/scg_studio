#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "3rdparty/AudioFFT/AudioFFT.h"

MainWindow::MainWindow (QWidget* parent)
  : QMainWindow (parent)
  , ui (new Ui::MainWindow) {
  isdeviceconnect = false;
  iscollectingsignal = false;
  m_maxCacheSize = 4096;
  corrhr.reserve (50);
  ui->setupUi (this);
  //ecg plot init
  ecg_mchart = new mPlotChart ("Time(S)", "ECG level(V)");
  ui->m_ecgChartView->setChart (ecg_mchart->m_Chart);
  //scg plot init
  scg_mchart = new mPlotChart ("Time(S)", "Acceleration level(g)");
  ui->m_scgChartView->setChart (scg_mchart->m_Chart);
  //scg fft plot init
  scgFFT_mchart = new mPlotChart ("FFT point", "db");
  ui->m_scgFFTChartView->setChart (scgFFT_mchart->m_Chart);
  //scg corr plot init
  scgCORR_mchart = new mPlotChart ("Time(S)", "corr value");
  ui->m_scgCORRChartView->setChart (scgCORR_mchart->m_Chart);
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
  delete ui;
}

void MainWindow::updateData() {
  if (m_ecgbuffer.isEmpty() || m_scgbuffer.isEmpty() || m_scgbuffer[1].y() == 0) {
    return;
  }

  //refresh scg plot
  scg_mchart->m_Series->replace (m_scgbuffer);
  scg_mchart->autoYaxis();
  scg_mchart->autoXaxis();

  //refresh ecg plot
  ecg_mchart->m_Series->replace (m_ecgbuffer);
  ecg_mchart->autoYaxis (0, 1);
  ecg_mchart->autoXaxis();

  //refresh scg fft plot
  const size_t fftSize = 2048; // Needs to be power of 2!
  qreal fs = m_scgbuffer.length() / (m_scgbuffer.constLast().x() - m_scgbuffer.constFirst().x());
  std::vector<float> input (fftSize, 0.0f);
  std::vector<float> re (audiofft::AudioFFT::ComplexSize (fftSize));
  std::vector<float> im (audiofft::AudioFFT::ComplexSize (fftSize));
  std::vector<float> output (fftSize);
  for (int i = 0; i < fftSize; i++) {
    input[i] = m_scgbuffer[i].y();
  }
  audiofft::AudioFFT m_fft;
  m_fft.init (fftSize);
  m_fft.fft (input.data(), re.data(), im.data());
  m_fft.ifft (output.data(), re.data(), im.data());
  for (int i = 0; i < fftSize; i++) {
    m_scgFFTbuffer[i].setX (fs / fftSize * i);
    m_scgFFTbuffer[i].setY (log (sqrt (re[i]*re[i] + im[i]*im[i])));
    if (std::isinf (m_scgFFTbuffer[i].y())) {
      m_scgFFTbuffer[i].setY (0);
    }
  }
  scgFFT_mchart->m_Series->replace (m_scgFFTbuffer);
  scgFFT_mchart->m_axisY->setRange (-5, 2.6);
  scgFFT_mchart->m_axisX->setRange (m_scgFFTbuffer[1].x(), m_scgFFTbuffer[250].x());

  //refresh scg corr plot
  qreal mean = 0;
  for (int i = 0; i < m_scgbuffer.size(); i++) {
    mean += m_scgbuffer[i].y();
  }
  mean /= m_scgbuffer.size();

  qreal std = 0;
  for (int i = 0; i < m_scgbuffer.size(); i++) {
    std += (m_scgbuffer[i].y() - mean) * (m_scgbuffer[i].y() - mean);
  }
  std /= m_scgbuffer.size();
  std = sqrt (std);
  qreal corr;
  int m = 0;    // 周期
  qreal max_corr = 0;
  for (int i = 0; i < m_scgbuffer.size(); i++) {
    corr = 0;
    for (int j = 0; j < m_scgbuffer.size() - i; j++) {
      corr += (m_scgbuffer[j].y() - mean) * (m_scgbuffer[j + i].y() - mean);
    }
    corr /= (m_scgbuffer.size() - i) * std * std;
    m_scgCORRbuffer[i].setY (corr);
    m_scgCORRbuffer[i].setX ((m_scgbuffer[i].x() - m_scgbuffer[0].x()));
    if (corr > max_corr && i > 100 && i < 1024) { // 找到第一个最大相关系数
      max_corr = corr;
      m = i;
    }
  }
  qreal tmp = 60 /  m_scgCORRbuffer[m].x();
  if (tmp > 20 && tmp < 200) {
    qDebug() << m_scgCORRbuffer[m].x();
    if (corrhr.length() == 50) {
      corrhr.removeFirst();
    }
    corrhr.append (tmp);
    QList<qreal> sortedhr = corrhr;
    std::sort (sortedhr.begin(), sortedhr.end());
    int ind = sortedhr.length() / 2;
    ui->lcdNumber->display (sortedhr[ind]);
  }
  scgCORR_mchart->m_Series->replace (m_scgCORRbuffer);
  scgCORR_mchart->autoYaxis (0, -0.2);
  scgCORR_mchart->m_axisX->setRange (m_scgCORRbuffer[100].x(), m_scgCORRbuffer[2048].x());
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
    ui->connectPushButton->setText (tr ("connect"));
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
  ui->connectPushButton->setText (tr ("disconnect"));
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
    ui->collectPushButton->setText (tr ("collect"));
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
  ui->collectPushButton->setText (tr ("end collect"));
  QString defaultFileName = QDateTime::currentDateTime().toString ("yyyy-MM-dd_hh-mm-ss") + ".txt";
  QString dir = QFileDialog::getExistingDirectory (this, tr ("Select Directory"));
  if (dir.isEmpty()) {
    qWarning() << "dir.isEmpty";
    return;
  }
  fileName = dir + "/" + defaultFileName;
  qDebug() << fileName;
  // 连接定时器的timeout信号和槽函数
  connect (m_timer, &QTimer::timeout, this, &MainWindow::updateData);
  // 启动定时器
  m_timer->start (1000 / 50);
}

void MainWindow::processUDPdata() {

  while (m_udpSocket->hasPendingDatagrams()) {
//    static int bufferindex = 0;
    QByteArray datagram;
    datagram.resize (m_udpSocket->pendingDatagramSize());
    m_udpSocket->readDatagram (datagram.data(), datagram.size());

    QString dataString (datagram);
    QStringList dataList = dataString.split (",");

    if (m_ecgbuffer.isEmpty()) {
      m_ecgbuffer.reserve (sampleCount);
      for (int i = 0; i < sampleCount - 1; ++i) {
        m_ecgbuffer.append (QPointF (i, 0));
      }
    }
    if (m_scgbuffer.isEmpty()) {
      m_scgbuffer.reserve (sampleCount);
      for (int i = 0; i < sampleCount - 1; ++i) {
        m_scgbuffer.append (QPointF (i, 0));
      }
    }
    if (m_scgFFTbuffer.isEmpty()) {
      m_scgFFTbuffer.reserve (sampleCount);
      for (int i = 0; i < sampleCount - 1; ++i) {
        m_scgFFTbuffer.append (QPointF (i, 0));
      }
    }
    if (m_scgCORRbuffer.isEmpty()) {
      m_scgCORRbuffer.reserve (sampleCount);
      for (int i = 0; i < sampleCount - 1; ++i) {
        m_scgCORRbuffer.append (QPointF (i, 0));
      }
    }
    if (dataList.size() == 3) {
      m_messageCache.append (dataString);
      qreal millis = (dataList[0].toDouble() / 1000.);
      qreal ecgpoint = dataList[1].toDouble();
      qreal scgpoint = dataList[2].toDouble();
      if (scgpoint > 32768) {
        scgpoint = scgpoint - 65536;
      }
      scgpoint = scgpoint / 16393;
      m_ecgbuffer.removeFirst();
      m_scgbuffer.removeFirst();
      m_ecgbuffer.append (QPointF (millis / 1000, ecgpoint));
      m_scgbuffer.append (QPointF (millis / 1000, scgpoint));
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
  if (fileName.isEmpty()) {
    qWarning() << "fileName.isEmpty";
    return;
  }
  // 打开文件，如果文件不存在则创建
  file = new QFile (fileName);
  if (!file->open (QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    qWarning() << "Failed to open file";
    return;
  }

  // 将内存缓存中的字符串信息写入文件
  QTextStream out (file);
  for (const QString& message : m_messageCache) {
//    out << message << "\n";
    out << message;
  }

  // 清空内存缓存
  m_messageCache.clear();
}
