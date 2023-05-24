
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

}
