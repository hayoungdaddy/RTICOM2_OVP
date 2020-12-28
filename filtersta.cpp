#include "filtersta.h"
#include "ui_filtersta.h"

using namespace QtCharts;

FILTERSTA::FILTERSTA(QVector<_STATION> staList, int evid, int eewEvid, double eewLat, double eewLon, double eewMag, QString eewLoc, int eewTime, QString evtDir, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FILTERSTA)
{
    ui->setupUi(this);

    codec = QTextCodec::codecForName("utf-8");

    staListVT = staList;
    eqTime = eewTime;
    eqDir = evtDir;

    chan = 3;

    ui->eqInfoTB->setRowCount(0);
    ui->eqInfoTB->setSortingEnabled(false);
    ui->eqInfoTB->setRowCount(1);

    ui->eqInfoTB->setItem(0, 0, new QTableWidgetItem(QString::number(evid)));

    QDateTime t, tKST;
    t.setTime_t(eewTime);
    tKST = convertKST(t);
    ui->eqInfoTB->setItem(0, 1, new QTableWidgetItem(tKST.toString("yyyy-MM-dd hh:mm:ss")));

    ui->eqInfoTB->setItem(0, 2, new QTableWidgetItem(QString::number(eewEvid)));
    ui->eqInfoTB->setItem(0, 3, new QTableWidgetItem(QString::number(eewLat, 'f', 4)));
    ui->eqInfoTB->setItem(0, 4, new QTableWidgetItem(QString::number(eewLon, 'f', 4)));
    ui->eqInfoTB->setItem(0, 5, new QTableWidgetItem(QString::number(eewMag, 'f', 1)));
    ui->eqInfoTB->setItem(0, 6, new QTableWidgetItem(eewLoc));

    ui->eqInfoTB->setColumnWidth(1, 200);

    for(int k=0;k<ui->eqInfoTB->columnCount();k++)
    {
        ui->eqInfoTB->item(0, k)->setTextAlignment(Qt::AlignCenter);
    }

    eqChartView1 = new QChartView();
    eqChart1 = new QChart();
    eqChartView2 = new QChartView();
    eqChart2 = new QChart();
    eqChartView3 = new QChartView();
    eqChart3 = new QChart();
    //pgaChartView = new QChartView();
    //pgaChart = new QChart();

    makeEQGraph1();
    makeEQGraph2();
    makeEQGraph3();
    //makePGAGraph();

    ui->pgaGB->hide();    
    ui->noDataPB->hide();
    ui->overMaxMinRangePB->hide();
    ui->overSDPB->hide();
}

FILTERSTA::~FILTERSTA()
{
    delete ui;
}

void FILTERSTA::overSDPBClicked()
{
    if(!overSDStaV.isEmpty())
    {
        QMessageBox msgBox;
        QString temp;
        temp = codec->toUnicode("관측소 리스트 (표준편차 범위 밖)\n------------------------------------------\n");
        for(int i=0;i<overSDStaV.count();i++)
        {
            temp = temp + overSDStaV.at(i).sta + " " +
                    QString::number(overSDStaV.at(i).lat, 'f', 4) + " " +
                    QString::number(overSDStaV.at(i).lon, 'f', 4) + " " +
                    overSDStaV.at(i).comment + "\n";
        }
        msgBox.setText(temp);
        msgBox.exec();
    }
}

void FILTERSTA::overDistPBClicked()
{
    if(!overDistStaV.isEmpty())
    {
        QMessageBox msgBox;
        QString temp;
        temp = codec->toUnicode("관측소 리스트 (진앙과의 거리 400Km 초과)\n------------------------------------------\n");
        for(int i=0;i<overDistStaV.count();i++)
        {
            temp = temp + QString::number(i+1) + ". " + overDistStaV.at(i).sta + " " +
                    QString::number(overDistStaV.at(i).lat, 'f', 4) + " " +
                    QString::number(overDistStaV.at(i).lon, 'f', 4) + " " +
                    overDistStaV.at(i).comment + " " +
                    QString::number(overDistStaV.at(i).distance, 'f', 2) + "Km\n";
        }
        msgBox.setText(temp);
        msgBox.exec();
    }
}

void FILTERSTA::overMaxMinRangePBClicked()
{
    if(!outOfRangeForMAXMINVelStaV.isEmpty())
    {
        QMessageBox msgBox;
        QString temp;
        temp = codec->toUnicode("관측소 리스트 (유효 속도범위 밖)\n------------------------------------------\n");
        for(int i=0;i<outOfRangeForMAXMINVelStaV.count();i++)
        {
            temp = temp + QString::number(i+1) + ". " + outOfRangeForMAXMINVelStaV.at(i).sta + " " +
                    QString::number(outOfRangeForMAXMINVelStaV.at(i).lat, 'f', 4) + " " +
                    QString::number(outOfRangeForMAXMINVelStaV.at(i).lon, 'f', 4) + " " +
                    outOfRangeForMAXMINVelStaV.at(i).comment + "\n";
        }
        msgBox.setText(temp);
        msgBox.exec();
    }
}

void FILTERSTA::notUsedPBClicked()
{
    if(!notUsedStaV.isEmpty())
    {
        QMessageBox msgBox;
        QString temp;
        temp = codec->toUnicode("관측소 리스트 (사용 안함)\n------------------------------------------\n");
        for(int i=0;i<notUsedStaV.count();i++)
        {
            temp = temp + QString::number(i+1) + ". " + notUsedStaV.at(i).sta + " " +
                    QString::number(notUsedStaV.at(i).lat, 'f', 4) + " " +
                    QString::number(notUsedStaV.at(i).lon, 'f', 4) + " " +
                    notUsedStaV.at(i).comment + "\n";
        }
        msgBox.setText(temp);
        msgBox.exec();
    }
}

void FILTERSTA::noDataPBClicked()
{
    if(!noDataStaV.isEmpty())
    {
        QMessageBox msgBox;
        QString temp;
        temp = codec->toUnicode("관측소 리스트 (데이터 없음)\n------------------------------------------\n");
        for(int i=0;i<noDataStaV.count();i++)
        {
            temp = temp + QString::number(i+1) + ". " + noDataStaV.at(i).sta + " " +
                    QString::number(noDataStaV.at(i).lat, 'f', 4) + " " +
                    QString::number(noDataStaV.at(i).lon, 'f', 4) + " " +
                    noDataStaV.at(i).comment + "\n";
        }
        msgBox.setText(temp);
        msgBox.exec();
    }
}

void FILTERSTA::nErrorPBClicked()
{
    if(!nErrorStaV.isEmpty())
    {
        QMessageBox msgBox;
        QString temp;
        temp = codec->toUnicode("관측소 리스트 (정보 오류)\n------------------------------------------\n");
        for(int i=0;i<nErrorStaV.count();i++)
        {
            temp = temp + QString::number(i+1) + ". " + nErrorStaV.at(i).sta + " " +
                    QString::number(nErrorStaV.at(i).lat, 'f', 4) + " " +
                    QString::number(nErrorStaV.at(i).lon, 'f', 4) + " " +
                    nErrorStaV.at(i).comment + "\n";
        }
        msgBox.setText(temp);
        msgBox.exec();
    }
}

void FILTERSTA::makeEQGraph1()
{
    int minX = 999, maxX = 0;
    int minY = 999, maxY = 0;

    int nTotal = 0;
    notUsedStaV.clear();
    noDataStaV.clear();
    int nUsed = 0;
    nErrorStaV.clear();
    int nNotUsed = 0;
    int nNoData = 0;
    int nError = 0;
    staListVTforGraph2.clear();

    QScatterSeries *realDataSeries = new QScatterSeries();

    realDataSeries->setName("Each Station");
    realDataSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    realDataSeries->setMarkerSize(10.0);

    for(int i=0;i<staListVT.count();i++)
    {
        nTotal++;
        if(staListVT.at(i).inUse == 0)
            notUsedStaV.push_back(staListVT.at(i));
        else if(staListVT.at(i).maxPGA[chan] == 0)
            noDataStaV.push_back(staListVT.at(i));
        else if(staListVT.at(i).distance >= 1000)
            nErrorStaV.push_back(staListVT.at(i));
        else
        {
            if(staListVT.at(i).distance < minX) minX = staListVT.at(i).distance;
            if(staListVT.at(i).distance > maxX) maxX = staListVT.at(i).distance;

            int difftime = staListVT.at(i).maxPGATime[chan] - eqTime;
            if(difftime < minY) minY = difftime;
            if(difftime > maxY) maxY = difftime;

            nUsed++;
            staListVTforGraph2.push_back(staListVT.at(i));
            realDataSeries->append(staListVT.at(i).distance, difftime);
        }
    }

    if(nErrorStaV.isEmpty()) { nError = 0; ui->nErrorPB->hide(); } else { nError=nErrorStaV.count(); ui->nErrorPB->show(); }
    if(notUsedStaV.isEmpty()) { nNotUsed = 0; ui->notUsedPB->hide(); } else { nNotUsed=notUsedStaV.count(); ui->notUsedPB->show(); }
    if(noDataStaV.isEmpty()) nNoData = 0; else nNoData=noDataStaV.count();

    ui->total1LB->setText(QString::number(nTotal));
    ui->notUsedLB->setText(QString::number(nNotUsed));
    ui->noDataLB->setText(QString::number(nNoData));
    ui->nErrorLB->setText(QString::number(nError));
    ui->used1LB->setText(QString::number(nUsed));

    connect(ui->notUsedPB, SIGNAL(clicked()), this, SLOT(notUsedPBClicked()));
    connect(ui->noDataPB, SIGNAL(clicked()), this, SLOT(noDataPBClicked()));
    connect(ui->nErrorPB, SIGNAL(clicked()), this, SLOT(nErrorPBClicked()));

    eqChart1->addSeries(realDataSeries);
    eqChart1->createDefaultAxes();

    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();
    axisX->setTitleText("DISTANCE (Km)");
    axisY->setTitleText("TIME (s)");
    axisX->setLabelFormat("%i");
    axisY->setLabelFormat("%i");

    int x2 = maxX % 50;
    int x3 = maxX / 50;
    int x4 = 50 - x2;

    int y2 = maxY % 20;
    int y3 = maxY / 20;
    int y4 = 20 - y2;

    int yy2 = minY % 20;
    int yy3 = minY / 20;
    int yy4 = -(20 + yy2);

    axisX->setRange(0, (x3*50) + x2 + x4);
    axisY->setRange((yy3*20) + yy2 + yy4, (y3*20) + y2 + y4);
    axisX->setTickCount(((x3*50) + x2 + x4)/50 + 1);
    axisY->setTickCount(((y3*20) + y2 + y4 + abs((yy3*20) + yy2 + yy4)) / 20 + 1);

    eqChart1->setAxisX(axisX, realDataSeries);
    eqChart1->setAxisY(axisY, realDataSeries);

    eqChartView1->setRenderHint(QPainter::Antialiasing);
    eqChartView1->setChart(eqChart1);
    ui->eqGraph1LO->addWidget(eqChartView1);
}

void FILTERSTA::makeEQGraph2()
{
    int minX = 999, maxX = 0;
    int maxY = 0;

    int nTotal = 0;
    outOfRangeForMAXMINVelStaV.clear();
    int nOutOfRange = 0;
    overDistStaV.clear();
    int nOverDist = 0;
    int nUsed = 0;

    staListVTforGraph3.clear();
    staListVTforGraph4.clear();

    QScatterSeries *realDataSeries = new QScatterSeries();
    QLineSeries *linearRegressionSeries = new QLineSeries();
    QLineSeries *maxWaveSeries = new QLineSeries();
    QLineSeries *minWaveSeries = new QLineSeries();

    realDataSeries->setName("Each Station");
    realDataSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    realDataSeries->setMarkerSize(10.0);

    maxWaveSeries->setName("MAX Velocity (" + QString::number(MAX_VELOCITY, 'f', 1) + "Km/s)");
    minWaveSeries->setName("MIN Velocity (" + QString::number(MIN_VELOCITY, 'f', 1) + "Km/s)");

    for(int i=0;i<staListVTforGraph2.count();i++)
    {
        nTotal++;
        if(staListVTforGraph2.at(i).distance > THRESHOLD_FOR_DIST)
        {
            overDistStaV.push_back(staListVTforGraph2.at(i));
            continue;
        }
        else
        {
            int difftime = staListVTforGraph2.at(i).maxPGATime[chan] - eqTime;

            double maxSec = staListVTforGraph2.at(i).distance / MAX_VELOCITY;
            double minSec = staListVTforGraph2.at(i).distance / MIN_VELOCITY;

            if(difftime >= maxSec && difftime <= minSec)
            {
                if(minSec > maxY) maxY = minSec;
                if(staListVTforGraph2.at(i).distance < minX) minX = staListVTforGraph2.at(i).distance;
                if(staListVTforGraph2.at(i).distance > maxX) maxX = staListVTforGraph2.at(i).distance;

                maxWaveSeries->append(staListVTforGraph2.at(i).distance, maxSec);
                minWaveSeries->append(staListVTforGraph2.at(i).distance, minSec);
                realDataSeries->append(staListVTforGraph2.at(i).distance, difftime);
                nUsed++;
                staListVTforGraph3.push_back(staListVTforGraph2.at(i));
            }
            else
                outOfRangeForMAXMINVelStaV.push_back(staListVTforGraph2.at(i));
        }
    }

    if(overDistStaV.isEmpty()) { nOverDist = 0; ui->overDistPB->hide(); } else { nOverDist=overDistStaV.count(); ui->overDistPB->show(); }
    if(outOfRangeForMAXMINVelStaV.isEmpty()) { nOutOfRange = 0; ui->overMaxMinRangePB->hide(); } else { nOutOfRange=outOfRangeForMAXMINVelStaV.count(); ui->overMaxMinRangePB->show(); }

    ui->total2LB->setText(QString::number(nTotal));
    ui->overDistLB->setText(QString::number(nOverDist));
    ui->overMaxMinRangeLB->setText(QString::number(nOutOfRange));
    ui->used2LB->setText(QString::number(nUsed));

    connect(ui->overDistPB, SIGNAL(clicked()), this, SLOT(overDistPBClicked()));
    connect(ui->overMaxMinRangePB, SIGNAL(clicked()), this, SLOT(overMaxMinRangePBClicked()));

    QVector<double> x;
    QVector<double> y;

    for(int i=0;i<realDataSeries->count();i++)
    {
        x.push_back(realDataSeries->at(i).x());
        y.push_back(realDataSeries->at(i).y());
    }

    double m, b, r;
    linregVector(realDataSeries->count(), x, y, &m, &b, &r);

    ui->slope1LB->setText(QString::number(m, 'f', 5));
    ui->intercept1LB->setText(QString::number(b, 'f', 5));
    ui->coeff1LB->setText(QString::number(r, 'f', 5));

    double pgaVelocity = 0;
    int count = 0;
    for(int x=minX;x<maxX;x++)
    {
        linearRegressionSeries->append(x, (m*x)+b);

        if(count != 0)
        {
            double ddd = x-(x-1);
            double ttt = ((m*x)+b) - ((m*(x-1))+b);
            double vel = ddd / ttt;
            pgaVelocity = vel;
        }

        count++;
    }

    ui->pgaVel1LB->setText(QString::number(pgaVelocity, 'f', 2) + "Km/s");
    linearRegressionSeries->setName("Linear Regression (" + QString::number(pgaVelocity, 'f', 2) + "Km/s)");

    overSDStaV.clear();
    for(int i=0;i<staListVTforGraph3.count();i++)
    {
        int difftime = staListVTforGraph3.at(i).maxPGATime[chan] - eqTime;
        double minValue, maxValue;
        minValue = (staListVTforGraph3.at(i).distance*m)+b-REGRESSION_RANGE;
        maxValue = (staListVTforGraph3.at(i).distance*m)+b+REGRESSION_RANGE;

        if(difftime >= minValue && difftime <= maxValue)
        {
            staListVTforGraph4.push_back(staListVTforGraph3.at(i));
        }
        else
        {
            overSDStaV.push_back(staListVTforGraph3.at(i));
        }
    }

    eqChart2->addSeries(realDataSeries);
    eqChart2->addSeries(maxWaveSeries);
    eqChart2->addSeries(minWaveSeries);
    eqChart2->addSeries(linearRegressionSeries);

    eqChart2->createDefaultAxes();

    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();
    axisX->setTitleText("DISTANCE (Km)");
    axisY->setTitleText("TIME (s)");
    axisX->setLabelFormat("%i");
    axisY->setLabelFormat("%i");

    int x2 = maxX % 50;
    int x3 = maxX / 50;
    int x4 = 50 - x2;

    int y2 = maxY % 20;
    int y3 = maxY / 20;
    int y4 = 20 - y2;

    axisX->setRange(0, (x3*50) + x2 + x4);
    axisY->setRange(0, (y3*20) + y2 + y4);
    axisX->setTickCount(((x3*50) + x2 + x4)/50 + 1);
    axisY->setTickCount(((y3*20) + y2 + y4)/20 + 1);

    eqChart2->setAxisX(axisX, realDataSeries);
    eqChart2->setAxisY(axisY, realDataSeries);

    eqChartView2->setRenderHint(QPainter::Antialiasing);
    eqChartView2->setChart(eqChart2);
    ui->eqGraph2LO->addWidget(eqChartView2);
}

void FILTERSTA::makeEQGraph3()
{
    int minX = 999, maxX = 0;
    int nOutofSD = 0;
    int nUsed = 0;

    QScatterSeries *realDataSeries = new QScatterSeries();
    QLineSeries *linearRegressionSeries = new QLineSeries();
    QLineSeries *linearRegressionSeriesUp = new QLineSeries();
    QLineSeries *linearRegressionSeriesDown = new QLineSeries();

    realDataSeries->setName("Each Station");
    realDataSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    realDataSeries->setMarkerSize(10.0);

    linearRegressionSeriesUp->setName("Linear Regression +" + QString::number(REGRESSION_RANGE));
    linearRegressionSeriesDown->setName("Linear Regression -" + QString::number(REGRESSION_RANGE));

    for(int i=0;i<staListVTforGraph4.count();i++)
    {
        nUsed++;
        int difftime = staListVTforGraph4.at(i).maxPGATime[chan] - eqTime;

        if(staListVTforGraph4.at(i).distance < minX) minX = staListVTforGraph4.at(i).distance;
        if(staListVTforGraph4.at(i).distance > maxX) maxX = staListVTforGraph4.at(i).distance;

        realDataSeries->append(staListVTforGraph4.at(i).distance, difftime);
    }

    if(overSDStaV.isEmpty()) { nOutofSD = 0; ui->overSDPB->hide(); } else { nOutofSD=overSDStaV.count(); ui->overSDPB->show(); }

    ui->total3LB->setText(ui->used2LB->text());
    ui->overSDLB->setText(QString::number(nOutofSD));
    ui->used3LB->setText(QString::number(nUsed));

    connect(ui->overSDPB, SIGNAL(clicked()), this, SLOT(overSDPBClicked()));

    QVector<double> x;
    QVector<double> y;

    for(int i=0;i<realDataSeries->count();i++)
    {
        x.push_back(realDataSeries->at(i).x());
        y.push_back(realDataSeries->at(i).y());
    }

    double m, b, r;
    linregVector(realDataSeries->count(), x, y, &m, &b, &r);

    ui->slope2LB->setText(QString::number(m, 'f', 5));
    ui->intercept2LB->setText(QString::number(b, 'f', 5));
    ui->coeff2LB->setText(QString::number(r, 'f', 5));

    double pgaVelocity = 0;
    int count = 0;
    for(int x=minX;x<maxX;x++)
    {
        linearRegressionSeries->append(x, (m*x)+b);
        linearRegressionSeriesUp->append(x, (m*x)+b+REGRESSION_RANGE);
        linearRegressionSeriesDown->append(x, (m*x)+b-REGRESSION_RANGE);

        if(count != 0)
        {
            double ddd = x-(x-1);
            double ttt = ((m*x)+b) - ((m*(x-1))+b);
            double vel = ddd / ttt;
            pgaVelocity = vel;
        }
        count++;
    }

    ui->pgaVel2LB->setText(QString::number(pgaVelocity, 'f', 2) + "Km/s");
    linearRegressionSeries->setName("Linear Regression (" + QString::number(pgaVelocity, 'f', 2) + "Km/s)");

    eqChart3->addSeries(realDataSeries);  
    eqChart3->addSeries(linearRegressionSeriesUp);
    eqChart3->addSeries(linearRegressionSeriesDown);
    eqChart3->addSeries(linearRegressionSeries);

    eqChart3->createDefaultAxes();

    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();
    axisX->setTitleText("Distance (Km)");
    axisY->setTitleText("TIME (s)");
    axisX->setLabelFormat("%i");
    axisY->setLabelFormat("%i");

    //axisX->setRange(0, 400);
    //axisY->setRange(0, 130);
    //axisX->setTickCount(9);
    //axisY->setTickCount(14);

    eqChart3->setAxisX(axisX, realDataSeries);
    eqChart3->setAxisY(axisY, realDataSeries);

    eqChartView3->setRenderHint(QPainter::Antialiasing);
    eqChartView3->setChart(eqChart3);
    ui->eqGraph3LO->addWidget(eqChartView3);
}

void FILTERSTA::makePGAGraph()
{
    QVector<_STATION> sortedStaVT;
    sortedStaVT.push_back(staListVTforGraph4.at(0));
    for(int i=1;i<staListVTforGraph4.count();i++)
    {
        for(int j=0;j<sortedStaVT.count();j++)
        {
            if(staListVTforGraph4.at(i).distance <= sortedStaVT.at(j).distance)
            {
                sortedStaVT.insert(j, staListVTforGraph4.at(i));
                break;
            }
            sortedStaVT.push_back(staListVTforGraph4.at(i));
        }
    }

    //QLineSeries *pgaSeries[sortedStaVT.count()];
    int startTime = eqTime + EVENT_SECONDS_FOR_START;

    for(int i=0;i<staListVTforGraph4.count();i++)
    {
        QFile pgaFile(eqDir + "/3_PGA/" + staListVTforGraph4.at(i).sta.left(2) + "/" + staListVTforGraph4.at(i).sta + ".dat");
        qDebug() << pgaFile.fileName();
        if(!pgaFile.exists())
        {
            continue;
        }

        if(pgaFile.open(QIODevice::ReadOnly))
        {
            QTextStream stream(&pgaFile);
            QString line, _line;

            while(!stream.atEnd())
            {
                line = stream.readLine();
                _line = line.simplified();

                QLineSeries *pgaSeries = new QLineSeries();
                pgaSeries->append(_line.section(" ", 0, 0).toInt() - startTime , _line.section(" ", 1, 1).toDouble());

                pgaChart->addSeries(pgaSeries);
            }
            pgaFile.close();
        }
    }

    pgaChart->legend()->setVisible(false);
    pgaChart->createDefaultAxes();
    pgaChartView->setRenderHint(QPainter::Antialiasing);
    pgaChartView->setChart(pgaChart);
    ui->pgaChartLO->addWidget(pgaChartView);
}
