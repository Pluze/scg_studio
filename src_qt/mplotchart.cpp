
#include "mplotchart.h"

mPlotChart::mPlotChart (QString xlabel, QString ylabel) {
  m_axisX = new QValueAxis;
  m_axisX->setRange (0, 1);
  m_axisX->setLabelFormat ("%g");
  m_axisX->setTitleText (xlabel);
  m_axisY = new QValueAxis;
  m_axisY->setRange (0, 1);
  m_axisY->setTitleText (ylabel);
  m_Series = new QLineSeries;
  m_Chart = new QChart;
  m_Chart->addSeries (m_Series);
  m_Chart->addAxis (m_axisX, Qt::AlignBottom);
  m_Series->attachAxis (m_axisX);
  m_Chart->addAxis (m_axisY, Qt::AlignLeft);
  m_Series->attachAxis (m_axisY);
  m_Chart->legend()->hide();
}

mPlotChart::~mPlotChart() {
  delete m_axisX;
  delete m_axisY;
  delete m_Series;
  delete m_Chart;
}

void mPlotChart::autoXaxis() {
  m_axisX->setRange (m_Series->points().constFirst().x(), m_Series->points().constLast().x());
}

void mPlotChart::autoYaxis (qreal baisMin, qreal baisMax) {
  qreal maxY = std::max_element (m_Series->points().constBegin(), m_Series->points().constEnd(),
  [] (const QPointF & p1, const QPointF & p2) {
    return p1.y() < p2.y();
  })->y();
  qreal minY = std::min_element (m_Series->points().constBegin(), m_Series->points().constEnd(),
  [] (const QPointF & p1, const QPointF & p2) {
    return p1.y() < p2.y();
  })->y();
  m_axisY->setRange (minY + baisMin, maxY + baisMax);
}

void mPlotChart::autoXYaxis() {
  autoXaxis();
  autoYaxis();
}
