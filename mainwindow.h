#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextCodec>
#include <QQuickView>
#include <QtQuick>
#include <QMessageBox>
#include <QTimer>
#include <QProgressDialog>
#include <QProcess>
#include <QButtonGroup>
#include <QtCharts>
#include "QtConcurrent/qtconcurrentrun.h"

#include "common.h"
#include "eventlist.h"
#include "filtersta.h"
#include "newregend.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString configFile = nullptr, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

protected:

private:
    Ui::MainWindow *ui;

    QTextCodec *codec;

    QTimer *playTimer;

    QButtonGroup *conBG;
    int isFilter;

    bool isValid;

    _CONFIGURE cfg;
    void readCFG();
    void decorationGUI();

    //EventList *eventlist;

    QVector<_STATION> staListVT;
    QVector<QString> SSSSSVT;
    QVector<QString> SSSSSDVT;
    QMultiMap<int, _QSCD_FOR_MULTIMAP> dataHouse;
    int evid;
    int eventTime;
    int dataStartTime;
    int duration;
    QString eventType;
    int inSeconds;
    int numStations;
    int distance;
    double thresholdG;
    double thresholdM;
    double pga_vel;
    int eew_evid;
    QString channel;
    QVector<QString> eventFirstInfo;
    int eqTime;
    double eqLat, eqLon;
    double eqMag;
    QString eqLoc;
    int eqNsta;
    bool haveEQ;
    QString evDir;

    void loadStationsFromFile(QString, QString);
    void loadDataFromFile(QString);
    void loadDataFromBinFile(QString);
    void loadDataToDataHouse();
    void loadHeaderFromFile(QString);
    void fillEVInfo();
    void findMaxStations();
    void getPGAVel(QString);
    double getPredictedValue(double, double);

    QQuickView *realMapView;
    QQuickView *stackedMapView;
    QObject *realObj;
    QWidget *realMapContainer;
    QObject *stackedObj;
    QWidget *stackedMapContainer;
    QVariant returnedValue;

    void createStaCircleOnMap();
    void resetStaCircleOnMap();
    void changeCircleOnMap(QString, int, int, QColor, int);

    bool maxPBClicked;

    QLabel *eventNetLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventStaLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventComLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventTimeLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventPGALB[MAX_NUM_EVENTINITSTA];

private slots:
    void doEventPlay();
    void eventLoadPBClicked();
    void rvEventName(QString);
    void changeChannel(int);
    void valueChanged(int);

    void drawMaxCircleOnMap();

    void _qmlSignalfromReplayMap(QString);
    void _qmlSignalfromStackedReplayMap(QString);
    void playPBClicked();
    void stopPBClicked();

    void filterChanged(int);
    void viewFilterPBClicked();
};

#endif // MAINWINDOW_H
