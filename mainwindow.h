
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
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QTimer>
#include <QFile>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow

{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private slots:
    void refreshSerialDevice();
    void openSerialPort();
    void openUdpPort();
    void readSerialData();
    void sendSerialData(char command);
    void processUDPdata();
    void updateData();
    void writeCacheToFile();

  private:
    Ui::MainWindow *ui;

    QLineSeries *m_ecgSeries;
    QChart* m_ecgChart;

    QLineSeries* m_scgSeries;
    QChart* m_scgChart;

    QSerialPort* m_serialPort;
    QUdpSocket* m_udpSocket;

    QTimer* m_timer;

    bool isdeviceconnect;
    bool iscollectingsignal;

    int millis;
    qreal sensor1Value;
    qreal sensor2Value;
    QStringList m_messageCache;
    int m_maxCacheSize;
};

#endif // MAINWINDOW_H
