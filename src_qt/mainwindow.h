#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QUdpSocket>
#include <QtCharts/QChart>
#include <QtCharts/QXYSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QtCharts/QValueAxis>
#include "mplotchart.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow

{
  Q_OBJECT

 public:
  MainWindow (QWidget* parent = nullptr);
  ~MainWindow();
  static const int sampleCount = 4096;

 private slots:
  void refreshSerialDevice();
  void openSerialPort();
  void openUdpPort();
  void readSerialData();
  void sendSerialData (char command);
  void processUDPdata();
  void updateData();
  void writeCacheToFile();

 private:
  Ui::MainWindow* ui;

  QList<QPointF> m_scgbuffer;
  QList<QPointF> m_ecgbuffer;
  QList<QPointF> m_scgFFTbuffer;
  QList<QPointF> m_scgCORRbuffer;
  QList<qreal> corrhr;

  mPlotChart* ecg_mchart;
  mPlotChart* scg_mchart;
  mPlotChart* scgFFT_mchart;
  mPlotChart* scgCORR_mchart;

  QSerialPort* m_serialPort;
  QUdpSocket* m_udpSocket;

  QTimer* m_timer;

  bool isdeviceconnect;
  bool iscollectingsignal;

  QStringList m_messageCache;
  QString fileName;
  QFile* file;
  int m_maxCacheSize;
};

#endif // MAINWINDOW_H
