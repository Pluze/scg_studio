
#ifndef MPLOTCHART_H
#define MPLOTCHART_H

#include <QtCharts/QChart>
#include <QtCharts/QXYSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

class mPlotChart : public QObject

{
 public:
  mPlotChart (QString xlabel, QString ylabel);
  ~mPlotChart();
  QValueAxis* m_axisX;
  QValueAxis* m_axisY;
  QXYSeries* m_Series;
  QChart* m_Chart;
};

#endif // MPLOTCHART_H
