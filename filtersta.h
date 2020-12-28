#ifndef FILTERSTA_H
#define FILTERSTA_H

#include <QDialog>
#include <QtCharts>
#include <QDebug>
#include <QMessageBox>
#include <QTextCodec>

#include "common.h"

#define REGRESSION_RANGE 3
#define MAX_VELOCITY 7.0
#define MIN_VELOCITY 2.0
#define THRESHOLD_FOR_DIST 400

namespace Ui {
class FILTERSTA;
}

class FILTERSTA : public QDialog
{
    Q_OBJECT

public:
    explicit FILTERSTA(QVector<_STATION> staList, int evid=0, int eewEvid=0, double eewLat=0, double eewLon=0, double eewMag=0, QString eewLoc=nullptr, int eewTime=0,
                       QString evtDir=nullptr, QWidget *parent=nullptr);
    ~FILTERSTA();

private:
    Ui::FILTERSTA *ui;

    QTextCodec *codec;

    QVector<_STATION> staListVT;
    QVector<_STATION> staListVTforGraph2;
    QVector<_STATION> staListVTforGraph3;
    QVector<_STATION> staListVTforGraph4;

    int eqTime;
    QString eqDir;

    void makeEQGraph1();
    QChartView *eqChartView1;
    QChart *eqChart1;
    QVector<_STATION> notUsedStaV;
    QVector<_STATION> noDataStaV;
    QVector<_STATION> nErrorStaV;

    void makeEQGraph2();
    QChartView *eqChartView2;
    QChart *eqChart2;
    QVector<_STATION> outOfRangeForMAXMINVelStaV;
    QVector<_STATION> overDistStaV;

    void makeEQGraph3();
    QChartView *eqChartView3;
    QChart *eqChart3;
    QVector<_STATION> overSDStaV;

    void makePGAGraph();
    QChartView *pgaChartView;
    QChart *pgaChart;

    int chan;

private slots:
    void notUsedPBClicked();
    void noDataPBClicked();
    void nErrorPBClicked();
    void overDistPBClicked();
    void overMaxMinRangePBClicked();
    void overSDPBClicked();
};

#endif // FILTERSTA_H
