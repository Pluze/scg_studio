
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
    void processPendingDatagrams();

  private:
    Ui::MainWindow *ui;

    QLineSeries *m_ecgSeries;
    QChart* m_ecgChart;

    QLineSeries* m_scgSeries;
    QChart* m_scgChart;

    QSerialPort* m_serialPort;
    QUdpSocket* m_udpSocket;

    bool isdeviceconnect;
    bool iscollectingsignal;
};

#endif // MAINWINDOW_H
