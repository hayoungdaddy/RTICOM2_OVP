#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace QtCharts;

MainWindow::MainWindow(QString configFile, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if(configFile.startsWith("--version") || configFile.startsWith("-version"))
    {
        qDebug() << "--- RTICOM2 OVP ---";
        qDebug() << "Version " + QString::number(VERSION, 'f', 1) + " (2019-07-10)";
        qDebug() << "From KIGAM KERC.";
        exit(1);
    }


    qDebug() << exp(50);
    qDebug() << log(50);
    /*
    QString temp = "rm -rf /home/sysop/.cache/QtLocation/5.8/tiles/osm/*";
    system(temp.toLatin1().data());
    temp = "rm -rf /home/sysop/.cache/RTICOM2_OVP/qmlcache/*";
    system(temp.toLatin1().data());
    */

    ui->setupUi(this);

    NewRegend *nr = new NewRegend;
    nr->show();

    codec = QTextCodec::codecForName("utf-8");
    cfg.configFileName = configFile;
    readCFG();
    decorationGUI();

    cfg.monChanID = 3;
    isFilter = 0;
    ui->viewFilterPB->setEnabled(false);
    connect(ui->viewFilterPB, SIGNAL(clicked()), this, SLOT(viewFilterPBClicked()));

    maxPBClicked = true;
    haveEQ = false;
    //eventlist = new EventList(cfg.db_ip, cfg.db_name, cfg.db_user, cfg.db_passwd, this);
    //connect(eventlist, SIGNAL(sendEventNameToMain(QString)), this, SLOT(rvEventName(QString)));

    connect(ui->play1PB, SIGNAL(clicked()), this, SLOT(playPBClicked()));
    connect(ui->play2PB, SIGNAL(clicked()), this, SLOT(playPBClicked()));
    connect(ui->play4PB, SIGNAL(clicked()), this, SLOT(playPBClicked()));
    connect(ui->stopPB, SIGNAL(clicked()), this, SLOT(stopPBClicked()));
    connect(ui->maxPB, SIGNAL(clicked()), this, SLOT(drawMaxCircleOnMap()));
    connect(ui->eventLoadPB, SIGNAL(clicked()), this, SLOT(eventLoadPBClicked()));
    connect(ui->monChanCB, SIGNAL(currentIndexChanged(int)), this, SLOT(changeChannel(int)));
    playTimer = new QTimer;
    playTimer->stop();
    connect(playTimer, SIGNAL(timeout()), this, SLOT(doEventPlay()));
    connect(ui->slider, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));

    QVariantMap params
    {
        /*
        {"osm.mapping.cache.disk.cost_strategy", 0 },
        {"osm.mapping.cache.disk.size", 0 },
        {"osm.mapping.cache.memory.cost_strategy", 0},
        {"osm.mapping.cache.memory.size", 0 },
        */
        {"osm.mapping.highdpi_tiles", true},
        {"osm.mapping.offline.directory", cfg.mapType}
    };

    realMapView = new QQuickView();
    realMapContainer = QWidget::createWindowContainer(realMapView, this);
    realMapView->setResizeMode(QQuickView::SizeRootObjectToView);
    realMapContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    realMapContainer->setFocusPolicy(Qt::TabFocus);
    realMapView->setSource(QUrl(QStringLiteral("qrc:/ViewMap.qml")));
    ui->realMapLO->addWidget(realMapContainer);
    realObj = realMapView->rootObject();

    stackedMapView = new QQuickView();
    stackedMapContainer = QWidget::createWindowContainer(stackedMapView, this);
    stackedMapView->setResizeMode(QQuickView::SizeRootObjectToView);
    stackedMapContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stackedMapContainer->setFocusPolicy(Qt::TabFocus);
    stackedMapView->setSource(QUrl(QStringLiteral("qrc:/ViewMap.qml")));
    ui->stackedMapLO->addWidget(stackedMapContainer);
    stackedObj = stackedMapView->rootObject();

    QMetaObject::invokeMethod(realObj, "initializePlugin", Q_ARG(QVariant, QVariant::fromValue(params)));
    QMetaObject::invokeMethod(stackedObj, "initializePlugin", Q_ARG(QVariant, QVariant::fromValue(params)));

    QObject::connect(this->realObj, SIGNAL(sendStationIndexSignal(QString)), this, SLOT(_qmlSignalfromReplayMap(QString)));
    QObject::connect(this->stackedObj, SIGNAL(sendStationIndexSignal(QString)), this, SLOT(_qmlSignalfromStackedReplayMap(QString)));

    QMetaObject::invokeMethod(this->realObj, "createRealTimeMapObject", Q_RETURN_ARG(QVariant, returnedValue));
    QMetaObject::invokeMethod(this->stackedObj, "createStackedMapObject", Q_RETURN_ARG(QVariant, returnedValue));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if( !QMessageBox::question( this,
                                codec->toUnicode("프로그램 종료"),
                                codec->toUnicode("프로그램을 종료합니다."),
                                codec->toUnicode("종료"),
                                codec->toUnicode("취소"),
                                QString::null, 1, 1) )
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::playPBClicked()
{
    QObject* pObject = sender();
    int speed = pObject->objectName().mid(4, 1).toInt();

    if(speed == 1)
    {
        playTimer->setInterval(1000);
        ui->play1PB->setStyleSheet("background-color: green");
        ui->play2PB->setStyleSheet("background-color: white");
        ui->play4PB->setStyleSheet("background-color: white");
    }
    else if(speed == 2)
    {
        playTimer->setInterval(500);
        ui->play1PB->setStyleSheet("background-color: white");
        ui->play2PB->setStyleSheet("background-color: green");
        ui->play4PB->setStyleSheet("background-color: white");
    }
    else if(speed == 4)
    {
        playTimer->setInterval(250);
        ui->play1PB->setStyleSheet("background-color: white");
        ui->play2PB->setStyleSheet("background-color: white");
        ui->play4PB->setStyleSheet("background-color: green");
    }

    if(!playTimer->isActive())
        playTimer->start();
}

void MainWindow::stopPBClicked()
{
    ui->play1PB->setStyleSheet("background-color: white");
    ui->play2PB->setStyleSheet("background-color: white");
    ui->play4PB->setStyleSheet("background-color: white");
    playTimer->stop();
}

void MainWindow::doEventPlay()
{
    int current = ui->slider->value();
    if(current >= duration-1)
        stopPBClicked();
    else
        ui->slider->setValue(current + 1);
}

void MainWindow::_qmlSignalfromReplayMap(QString indexS)
{
    QString msg, msg2;
    int index = indexS.toInt();
    msg = staListVT.at(index).sta.left(2) + "/" + staListVT.at(index).sta.mid(2) + "/" + staListVT.at(index).comment;

    if(staListVT.at(index).maxPGATime[cfg.monChanID] != 0)
    {
        QDateTime dt;
        dt.setTime_t(staListVT.at(index).maxPGATime[cfg.monChanID]);
        dt = convertKST(dt);

        msg2 = "     Maximum PGA Time:" + dt.toString("yyyy/MM/dd hh:mm:ss") + "     Maximum PGA:" +
                QString::number(staListVT.at(index).maxPGA[cfg.monChanID], 'f', 6);

        msg = msg + msg2;
    }

    ui->realStatusLB->setText( msg );
}

void MainWindow::_qmlSignalfromStackedReplayMap(QString indexS)
{
    QString msg, msg2;
    int index = indexS.toInt();
    msg = staListVT.at(index).sta.left(2) + "/" + staListVT.at(index).sta.mid(2) + "/" + staListVT.at(index).comment;

    if(staListVT.at(index).maxPGATime[cfg.monChanID] != 0)
    {
        QDateTime dt;
        dt.setTime_t(staListVT.at(index).maxPGATime[cfg.monChanID]);
        dt = convertKST(dt);

        msg2 = "     Maximum PGA Time:" + dt.toString("yyyy/MM/dd hh:mm:ss") + "     Maximum PGA:" +
                QString::number(staListVT.at(index).maxPGA[cfg.monChanID], 'f', 6);

        msg = msg + msg2;
    }

    ui->stackedStatusLB->setText( msg );
}

void MainWindow::eventLoadPBClicked()
{
    EventList *eventlist = new EventList(cfg.db_ip, cfg.db_name, cfg.db_user, cfg.db_passwd, this);
    connect(eventlist, SIGNAL(sendEventNameToMain(QString)), this, SLOT(rvEventName(QString)));
    eventlist->show();
    //if(eventlist->isHidden())
        //eventlist->show();
}

void MainWindow::changeCircleOnMap(QString mapName, int staIndex, int width, QColor colorName, int opacity)
{
    if(mapName.startsWith("realMap"))
    {
        QMetaObject::invokeMethod(this->realObj, "changeSizeAndColorForStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, staIndex), Q_ARG(QVariant, width),
                                  Q_ARG(QVariant, colorName), Q_ARG(QVariant, opacity));

    }
    else if(mapName.startsWith("stackedMap"))
    {
        QMetaObject::invokeMethod(this->stackedObj, "changeSizeAndColorForStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, staIndex), Q_ARG(QVariant, width),
                                  Q_ARG(QVariant, colorName),
                                  Q_ARG(QVariant, opacity));
    }
}

void MainWindow::rvEventName(QString eventName)
{
    QDir evtDir(cfg.eventDir + "/" + eventName);
    if(!evtDir.exists())
    {
        QString temp = codec->toUnicode("데이터를 요청하고 있습니다.");
        QProgressDialog progress(temp, codec->toUnicode("취소"), 0, 2, this);
        progress.setWindowTitle(codec->toUnicode("RTICOM2 OVP"));
        progress.setMinimumWidth(700);
        progress.setWindowModality(Qt::WindowModal);
        progress.setValue(0);

        evtDir.mkpath(".");
        QProcess process;
        QString cmd = "scp kisstool@" + cfg.db_ip + ":/kisstool/data/EVENTS/" + eventName + ".tar.gz " + evtDir.path();
        process.start(cmd);
        process.waitForFinished(-1);
        cmd = "tar --strip-component=6 -xzf " + evtDir.path() + "/" + eventName.section("/", -1) + ".tar.gz -C " + evtDir.path();
        progress.setValue(1);
        process.start(cmd);
        process.waitForFinished(-1);
        progress.setValue(2);
    }
    /*
    else
    {
        QDir evtFileDir(cfg.eventDir + "/" + eventName + "/SRC");
        if(!evtFileDir.exists())
        {
            QProcess process;
            QString cmd = "scp kisstool@" + cfg.db_ip + ":/kisstool/data/EVENTS/" + eventName + ".tar.gz " + evtDir.path();
            process.start(cmd);
            process.waitForFinished(-1);
            cmd = "tar --strip-component=6 -xzf " + evtDir.path() + "/" + eventName.section("/", -1) + ".tar.gz -C " + evtDir.path();
            process.start(cmd);
            process.waitForFinished(-1);
        }
    }
    */

    evDir = evtDir.path();
    QString evDataFileName = evtDir.path() + "/SRC/PGA_G_B.dat";
    QString evDataTxtFileName = evtDir.path() + "/SRC/PGA_G.dat";
    QString evHeaderFileName = evtDir.path() + "/SRC/header.dat";
    QString evStaFileName = evtDir.path() + "/SRC/stalist_G.dat";
    QString evStaSDFileName = evtDir.path() + "/1_DISTANCE-TIME/realDataAfterSD.dat";

    ui->maxPB->setEnabled(true);
    ui->monChanCB->setEnabled(true);
    ui->slider->setEnabled(true);
    ui->play1PB->setEnabled(true);
    ui->play2PB->setEnabled(true);
    ui->play4PB->setEnabled(true);
    ui->stopPB->setEnabled(true);

    isValid = true;

    loadHeaderFromFile(evHeaderFileName);
    if(haveEQ && isValid)
    {
        getPGAVel(evtDir.path() + "/1_DISTANCE-TIME/linregHeaderAfterSD.dat");
    }

    if(isValid)
        loadStationsFromFile(evStaFileName, evStaSDFileName);

    if(isValid)
        createStaCircleOnMap();

    if(isValid)
    {
        loadDataFromBinFile(evDataFileName);

        if(!isValid)
            loadDataFromFile(evDataTxtFileName);
    }

    if(isValid)
        fillEVInfo();
}

void MainWindow::getPGAVel(QString evHeaderAfterSDFileName)
{
    QFile headerSDFile(evHeaderAfterSDFileName);
    if(!headerSDFile.exists())
    {
        qDebug() << "Header After Standard Deviation file doesn't exists.";
        pga_vel = S_VEL;
    }

    if(headerSDFile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&headerSDFile);
        QString line, _line;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();

            if(_line.startsWith("PGA_Velocity"))
            {
                pga_vel = _line.section(":", 1, 1).toDouble();
                break;
            }
            else
                continue;
        }
        headerSDFile.close();
    }
}

double MainWindow::getPredictedValue(double dist, double mag)
{
    double y = (dist * dist) + (EQ_DEPTH * EQ_DEPTH);
    double R = sqrt(y);

    qDebug() << dist << R;

    /* 2001  */
    double ksai0[4] = {0.1250737E+02, 0.4874629E+00, -0.2940726E-01, 0.1737204E-01};
    double ksai1[4] = {-0.1928185E-02, 0.2251016E-03, -0.6378615E-04, 0.6967121E-04};
    double ksai2[4] = {-0.5795112E+00, 0.1138817E+00, -0.1162326E-01, -0.3646674E-02};



    /* 2003
    double ksai0[4] = {0.1073829E+02 , 0.5909022E+00, -0.5622945E-01, 0.2135007E-01};
    double ksai1[4] = {-0.2379955E-02, 0.2081359E-03, -0.2046806E-04, 0.4192630E-04};
    double ksai2[4] = {-0.2437218E+00, 0.9498274E-01, -0.8804236E-02, -0.3302350E-02};
     */

    double c0, c1, c2;

    /*
    c0 = ksai0[0] + (ksai0[1]*(mag-6)) + pow((ksai0[2]*(mag-6)), 2) + pow((ksai0[3]*(mag-6)), 3);
    c1 = ksai1[0] + (ksai1[1]*(mag-6)) + pow((ksai1[2]*(mag-6)), 2) + pow((ksai1[3]*(mag-6)), 3);
    c2 = ksai2[0] + (ksai2[1]*(mag-6)) + pow((ksai2[2]*(mag-6)), 2) + pow((ksai2[3]*(mag-6)), 3);

    double lnSA;
    if(dist < 100)
        lnSA = c0 + (c1*dist) + (c2*log(dist)) - log(dist) - (log(100)*0.5);
    else if(dist > 100)
        lnSA = c0 + (c1*dist) + (c2*log(dist)) - log(100) - (log(dist)*0.5);
        */

    c0 = ksai0[0] + ksai0[1]*(mag-6) + ksai0[2]*pow((mag-6), 2) + ksai0[3]*pow((mag-6), 3);
    c1 = ksai1[0] + ksai1[1]*(mag-6) + ksai1[2]*pow((mag-6), 2) + ksai1[3]*pow((mag-6), 3);
    c2 = ksai2[0] + ksai2[1]*(mag-6) + ksai2[2]*pow((mag-6), 2) + ksai2[3]*pow((mag-6), 3);


    double lnSA;
    if(R < 100)
        lnSA = c0 + c1*R + c2*log(R) - log(R) - 0.5*log(100);
    else if(R > 100)
        lnSA = c0 + c1*R + c2*log(R) - log(100) - 0.5*log(R);
    else if(R == 100)
        lnSA = c0 + c1*R + c2*log(R) - log(100) - 0.5*log(100);

   return exp(lnSA);
}

void MainWindow::viewFilterPBClicked()
{
    FILTERSTA *filtersta = new FILTERSTA(staListVT, evid, eew_evid, eqLat, eqLon, eqMag, eqLoc, eventTime, evDir, this);
    filtersta->show();
}

void MainWindow::readCFG()
{
    QFile file(cfg.configFileName);
    if(!file.exists())
    {
        qDebug() << "Failed configuration. Parameter file doesn't exists.";

        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("파라미터 파일이 존재하지 않습니다. (") + cfg.configFileName + ")\n" + codec->toUnicode("프로그램을 종료합니다."));
        msgBox.exec();
        exit(1);
    }
    if(file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);
        QString line, _line;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();

            if(_line.startsWith(" ") || _line.startsWith("#"))
                continue;
            else if(_line.startsWith("HOME_DIR"))
            {
                cfg.homeDir = _line.section("=", 1, 1);
                cfg.logDir = cfg.homeDir + "/logs";
                cfg.eventDir = cfg.homeDir + "/events";
            }
            else if(_line.startsWith("DB_IP"))
                cfg.db_ip = _line.section("=", 1, 1);
            else if(_line.startsWith("DB_NAME"))
                cfg.db_name = _line.section("=", 1, 1);
            else if(_line.startsWith("DB_USERNAME"))
                cfg.db_user = _line.section("=", 1, 1);
            else if(_line.startsWith("DB_PASSWD"))
                cfg.db_passwd = _line.section("=", 1, 1);

            else if(_line.startsWith("MAP_TYPE"))
            {
                if(_line.section("=", 1, 1).startsWith("satellite")) cfg.mapType = "/.RTICOM2/map_data/mapbox-satellite";
                else if(_line.section("=", 1, 1).startsWith("outdoors")) cfg.mapType = "/.RTICOM2/map_data/mapbox-outdoors";
                else if(_line.section("=", 1, 1).startsWith("light")) cfg.mapType = "/.RTICOM2/map_data/mapbox-light";
                else if(_line.section("=", 1, 1).startsWith("street")) cfg.mapType = "/.RTICOM2/map_data/osm_street";
            }
        }
        file.close();
    }
}

void MainWindow::loadStationsFromFile(QString staFileName, QString evStaSDFileName)
{
    staListVT.clear();
    SSSSSVT.clear();
    SSSSSDVT.clear();

    QFile evStaSDFile(evStaSDFileName);
    if(!evStaSDFile.exists())
    {
        ui->aFilterCB->setEnabled(false);
        ui->viewFilterPB->setEnabled(false);
    }
    else
    {
        ui->aFilterCB->setEnabled(true);
        ui->viewFilterPB->setEnabled(true);

        if(evStaSDFile.open(QIODevice::ReadOnly))
        {
            QTextStream stream(&evStaSDFile);
            QString line, _line;

            while(!stream.atEnd())
            {
                line = stream.readLine();
                _line = line.simplified();

                SSSSSDVT.push_back(_line.section(" ", 2, 2));
            }
            evStaSDFile.close();
        }
    }

    QFile evStaFile(staFileName);
    if(!evStaFile.exists())
    {
        qDebug() << "Stations file doesn't exists.";
        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("관측소 파일이 존재하지 않습니다. (") + staFileName + ")" );
        msgBox.exec();
        isValid = false;
        return;
    }

    if(evStaFile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&evStaFile);
        QString line, _line;
        int staIndex = 0;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();
            _STATION sta;
            sta.index = staIndex;
            staIndex++;
            sta.sta = _line.section("=", 0, 0);
            SSSSSVT.push_back(_line.section("=", 0, 0));
            sta.lat = _line.section("=", 1, 1).toDouble();
            sta.lon = _line.section("=", 2, 2).toDouble();
            sta.elev = _line.section("=", 3, 3).toDouble();
            sta.comment = _line.section("=", 4, 4);
            sta.lastPGA = 0;
            sta.lastPGATime = 0;
            for(int i=0;i<5;i++)
            {
                sta.maxPGA[i] = 0;
                sta.maxPGATime[i] = 0;
            }
            for(int i=0;i<MAX_EVENT_DURATION;i++)
            {
                for(int j=0;j<5;j++)
                {
                    sta.stackedPGA[i][j] = 0;
                }
            }
            sta.inUse =_line.section("=", 5, 5).toInt();
            sta.lastAlive = 0;

            if(haveEQ)
            {
                sta.distance = getDistance(sta.lat, sta.lon, eqLat, eqLon);
                sta.predictedPGA = getPredictedValue(sta.distance, eqMag);
            }
            else
                sta.distance = 0;
            staListVT.push_back(sta);
        }
        evStaFile.close();
    }

    for(int i=0;i<staListVT.count();i++)
    {
        int staIndex;
        staIndex = SSSSSDVT.indexOf(staListVT.at(i).sta);
        if(staIndex != -1)
        {
            _STATION sta = staListVT.at(i);
            sta.lastAlive = 1;
            staListVT.replace(i, sta);
        }
    }
}

void MainWindow::loadHeaderFromFile(QString headerFileName)
{
    /*
    EVID=1523
    EVENT_TIME=1558902012
    DATA_START_TIME=1558902007
    DURATION=120
    EVENT_TYPE=E
    MAG_THRESHOLD=1.00
    EEW_INFO=19428:1558902012:36.1210:129.3485:1.56:3

EVENT_CONDITION=3:4:3.00:50
FIRST_EVENT_INFO=1558531133:KRSLG:역)서울역사:3.796869
FIRST_EVENT_INFO=1558531133:ACJJG:제주대학교아라):3.132810
FIRST_EVENT_INFO=1558531134:ARGOG:농)광주저수지:3.957200
FIRST_EVENT_INFO=1558531135:GKGBG:경기광백댐:3.223130
    */
    evid = 0;
    eventTime = 0;
    dataStartTime = 0;
    duration = 0;
    inSeconds = 0;
    numStations = 0;
    thresholdG = 0;
    distance = 0;
    channel = "Horizontal PGA";
    eventFirstInfo.clear();
    eew_evid = 0;
    eqTime = 0;
    eqLat = 0;
    eqLon = 0;
    eqMag = 0;
    eqNsta = 0;

    QFile headerFile(headerFileName);
    if(!headerFile.exists())
    {
        qDebug() << "Header file doesn't exists.";
        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("이벤트 헤더 파일이 존재하지 않습니다. (") + headerFileName + ")" );
        msgBox.exec();
        isValid = false;
        return;
    }

    if(headerFile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&headerFile);
        QString line, _line;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();

            if(_line.startsWith("EVID"))
                evid = _line.section("=", 1, 1).toInt();
            else if(_line.startsWith("EVENT_TIME"))
                eventTime = _line.section("=", 1, 1).toInt();
            else if(_line.startsWith("DATA_START_TIME"))
                dataStartTime = _line.section("=", 1, 1).toInt();
            else if(_line.startsWith("DURATION"))
                duration = _line.section("=", 1, 1).toInt();
            else if(_line.startsWith("EVENT_TYPE"))
                eventType = _line.section("=", 1, 1);
            else if(_line.startsWith("MAG_THRESHOLD"))
                thresholdM = _line.section("=", 1, 1).toDouble();
            else if(_line.startsWith("EVENT_CONDITION"))
            {
                inSeconds = _line.section("=", 1, 1).section(":", 0, 0).toInt();
                numStations = _line.section("=", 1, 1).section(":", 1, 1).toInt();
                thresholdG = _line.section("=", 1, 1).section(":", 2, 2).toDouble();
                distance = _line.section("=", 1, 1).section(":", 3, 3).toInt();
            }
            else if(_line.startsWith("EEW_INFO"))  // EEW_INFO=19428:1558902012:36.1210:129.3485:1.56:3
            {
                eew_evid = _line.section("=", 1, 1).section(":", 0, 0).toInt();
                eqTime = _line.section("=", 1, 1).section(":", 1, 1).toInt();
                eqLat = _line.section("=", 1, 1).section(":", 2, 2).toDouble();
                eqLon = _line.section("=", 1, 1).section(":", 3, 3).toDouble();
                eqMag = _line.section("=", 1, 1).section(":", 4, 4).toDouble();
                //eqMag = 3.9;
                eqNsta = _line.section("=", 1, 1).section(":", 5, 5).toInt();
            }
            else if(_line.startsWith("FIRST_EVENT_INFO"))
                eventFirstInfo.push_back(_line.section("=", 1, 1));
        }
        headerFile.close();
    }

    if(eventType.startsWith("E"))
    {
        haveEQ = true;
        ui->bFilterCB->setEnabled(true);
        ui->aFilterCB->setEnabled(true);
        ui->viewFilterPB->setEnabled(true);
    }
    else
    {
        haveEQ = false;
        ui->bFilterCB->setEnabled(false);
        ui->aFilterCB->setEnabled(false);
        ui->viewFilterPB->setEnabled(false);
    }
}

void MainWindow::loadDataFromBinFile(QString dataFileName)
{
    dataHouse.clear();

    // insert to multimap
    QFile dataFile(dataFileName);
    if(!dataFile.exists())
    {
        /*
        qDebug() << "Data file doesn't exists.";
        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("데이터 파일이 존재하지 않습니다. (") + dataFileName + ")" );
        msgBox.exec();
        */
        isValid = false;
        return;
    }

    if(dataFile.open(QIODevice::ReadOnly))
    {
        QDataStream stream(&dataFile);
        _QSCD_FOR_BIN qfb;

        while(!stream.atEnd())
        {
            int result = stream.readRawData((char *)&qfb, sizeof(_QSCD_FOR_BIN));

            _QSCD_FOR_MULTIMAP qfmm;
            qfmm.sta = QString(qfb.sta);
            qfmm.pga[0] = qfb.pga[0];
            qfmm.pga[1] = qfb.pga[1];
            qfmm.pga[2] = qfb.pga[2];
            qfmm.pga[3] = qfb.pga[3];
            qfmm.pga[4] =qfb.pga[4];

            int index = SSSSSVT.indexOf(qfmm.sta);

            if(index != -1)
            {
                _STATION sta;
                sta = staListVT.at(index);
                int needReplace = 0;

                for(int i=0;i<5;i++)
                {
                    if(sta.maxPGA[i] < qfmm.pga[i])
                    {
                        sta.maxPGATime[i] = qfb.time;
                        sta.maxPGA[i] = qfmm.pga[i];
                        needReplace = 1;
                    }
                }

                if(needReplace == 1)
                    staListVT.replace(index, sta);

                dataHouse.insert(qfb.time, qfmm);
            }
        }
        dataFile.close();
    }

    loadDataToDataHouse();
}

void MainWindow::loadDataFromFile(QString dataFileName)
{
    dataHouse.clear();

    // insert to multimap
    QFile dataFile(dataFileName);
    if(!dataFile.exists())
    {
        qDebug() << "Data file doesn't exists.";
        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("데이터 파일이 존재하지 않습니다. (") + dataFileName + ")" );
        msgBox.exec();
        isValid = false;
        return;
    }

    if(dataFile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&dataFile);
        QString line, _line;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();

            int dataTime = _line.section(" ", 0, 0).toInt();

            _QSCD_FOR_MULTIMAP qfmm;
            qfmm.sta = _line.section("=", 0, 0).section(" ", 1, 1);
            qfmm.pga[0] = _line.section("=", 1, 1).section(":", 0, 0).toDouble();
            qfmm.pga[1] = _line.section("=", 1, 1).section(":", 1, 1).toDouble();
            qfmm.pga[2] = _line.section("=", 1, 1).section(":", 2, 2).toDouble();
            qfmm.pga[3] = _line.section("=", 1, 1).section(":", 3, 3).toDouble();
            qfmm.pga[4] = _line.section("=", 1, 1).section(":", 4, 4).toDouble();

            int index = SSSSSVT.indexOf(qfmm.sta);

            if(index != -1)
            {
                _STATION sta;
                sta = staListVT.at(index);
                int needReplace = 0;

                for(int i=0;i<5;i++)
                {
                    if(sta.maxPGA[i] < qfmm.pga[i])
                    {
                        sta.maxPGATime[i] = dataTime;
                        sta.maxPGA[i] = qfmm.pga[i];
                        needReplace = 1;
                    }
                }

                if(needReplace == 1)
                    staListVT.replace(index, sta);

                dataHouse.insert(dataTime, qfmm);
            }
        }
        dataFile.close();
    }

    isValid = true;

    loadDataToDataHouse();
}

void MainWindow::loadDataToDataHouse()
{
    duration = dataHouse.lastKey() - dataHouse.firstKey();

    if(duration >= MAX_EVENT_DURATION)
        duration = MAX_EVENT_DURATION;

    ui->slider->setRange(0, duration-1);

    QString temp = codec->toUnicode("데이터를 담고 있습니다.");
    QProgressDialog progress(temp, codec->toUnicode("취소"), 0, duration-1, this);
    progress.setWindowTitle(codec->toUnicode("RTICOM2 OVP"));
    progress.setMinimumWidth(700);
    progress.setWindowModality(Qt::WindowModal);

    for(int i=0;i<duration;i++)
    {
        QList<_QSCD_FOR_MULTIMAP> pgaList = dataHouse.values(dataHouse.firstKey() + i);

        if(pgaList.count() != 0)
        {
            for(int j=0;j<staListVT.count();j++)
            {
                _STATION sta = staListVT.at(j);

                bool succeed = false;
                for(int k=0;k<pgaList.count();k++)
                {
                    if(sta.sta.startsWith(pgaList.at(k).sta))
                    {
                        if(i==0)
                        {
                            for(int l=0;l<5;l++)
                                sta.stackedPGA[i][l] = pgaList.at(k).pga[l];
                        }
                        else
                        {
                            for(int l=0;l<5;l++)
                            {
                                if(sta.stackedPGA[i-1][l] >= pgaList.at(k).pga[l])
                                    sta.stackedPGA[i][l] = sta.stackedPGA[i-1][l];
                                else
                                    sta.stackedPGA[i][l] = pgaList.at(k).pga[l];
                            }
                        }
                        staListVT.replace(j, sta);
                        succeed = true;
                        break;
                    }
                }
                if(!succeed)
                {
                    if(i==0)
                    {
                        for(int l=0;l<5;l++)
                            sta.stackedPGA[i][l] = 0;
                    }
                    else
                    {
                        for(int l=0;l<5;l++)
                        {
                            if(sta.stackedPGA[i-1][l] > 0)
                                sta.stackedPGA[i][l] = sta.stackedPGA[i-1][l];
                            else
                                sta.stackedPGA[i][l] = 0;
                        }
                    }
                    staListVT.replace(j, sta);
                }
            }
        }
        else if(pgaList.count() == 0)
        {
            for(int j=0;j<staListVT.count();j++)
            {
                _STATION sta = staListVT.at(j);

                if(i==0)
                {
                    for(int l=0;l<5;l++)
                        sta.stackedPGA[i][l] = 0;
                }
                else
                {
                    for(int l=0;l<5;l++)
                    {
                        if(sta.stackedPGA[i-1][l] > 0)
                            sta.stackedPGA[i][l] = sta.stackedPGA[i-1][l];
                        else
                            sta.stackedPGA[i][l] = 0;
                    }
                }
                staListVT.replace(j, sta);
            }
        }
        /*
        if(pgaList.count() == 0)
        {
            for(int j=0;j<staListVT.count();j++)
            {
                _STATION sta = staListVT.at(j);

                if(i==0)
                {
                    for(int l=0;l<5;l++)
                        sta.stackedPGA[i][l] = 0;
                }
                else
                {
                    for(int l=0;l<5;l++)
                    {
                        if(sta.stackedPGA[i-1][l] > 0)
                            sta.stackedPGA[i][l] = sta.stackedPGA[i-1][l];
                        else
                            sta.stackedPGA[i][l] = 0;
                    }
                }
                staListVT.replace(j, sta);
            }
        }
        else
        {
            for(int j=0;j<staListVT.count();j++)
            {
                _STATION sta = staListVT.at(j);

                for(int k=0;k<pgaList.count();k++)
                {
                    if(sta.sta.startsWith(pgaList.at(k).sta))
                    {
                        if(i==0)
                        {
                            for(int l=0;l<5;l++)
                                sta.stackedPGA[i][l] = pgaList.at(k).pga[l];
                        }
                        else
                        {
                            for(int l=0;l<5;l++)
                            {
                                if(sta.stackedPGA[i-1][l] >= pgaList.at(k).pga[l])
                                    sta.stackedPGA[i][l] = sta.stackedPGA[i-1][l];
                                else
                                    sta.stackedPGA[i][l] = pgaList.at(k).pga[l];
                            }
                        }
                        staListVT.replace(j, sta);
                        break;
                    }
                    else
                    {
                        if(i==0)
                        {
                            for(int l=0;l<5;l++)
                                sta.stackedPGA[i][l] = 0;
                        }
                        else
                        {
                            for(int l=0;l<5;l++)
                            {
                                if(sta.stackedPGA[i-1][l] > 0)
                                    sta.stackedPGA[i][l] = sta.stackedPGA[i-1][l];
                                else
                                    sta.stackedPGA[i][l] = 0;
                            }
                        }
                        staListVT.replace(j, sta);
                    }
                }
            }
        }
        */
        progress.setValue(i);
    }
    progress.setValue(duration-1);

    ui->slider->setValue(0);
    drawMaxCircleOnMap();
}

void MainWindow::fillEVInfo()
{
    if(haveEQ)
    {
        ui->eventCondition1LB->hide();
        ui->eventCondition2LB->hide();
        ui->eventCondition3LB->hide();
        ui->eventCondition4LB->hide();
        ui->con2LB->hide();
        ui->con3LB->hide();
        ui->con4LB->hide();
        ui->eewFR->show();
        ui->galFR->hide();

        ui->con1LB->setText(codec->toUnicode("규모 ") + QString::number(thresholdM, 'f', 1) + codec->toUnicode(" 이상 조기경보 정보 수신 시"));
    }
    else
    {
        ui->eventCondition1LB->show();
        ui->eventCondition2LB->show();
        ui->eventCondition3LB->show();
        ui->eventCondition4LB->show();
        ui->con2LB->show();
        ui->con3LB->show();
        ui->con4LB->show();
        ui->eewFR->hide();
        ui->galFR->show();

        ui->con1LB->setText(codec->toUnicode("초 안에, "));
    }

    QDateTime t, tKST;
    t.setTime_t(eventTime);
    tKST = convertKST(t);
    ui->eventTimeLB->setText(tKST.toString("yyyy-MM-dd hh:mm:ss"));

    ui->eventCondition1LB->setText(QString::number(inSeconds));
    ui->eventCondition2LB->setText(QString::number(numStations));
    ui->eventCondition3LB->setText(QString::number(thresholdG, 'f', 1));
    ui->eventCondition4LB->setText(QString::number(distance));

    for(int j=0;j<MAX_NUM_EVENTINITSTA;j++)
    {
        eventNetLB[j]->clear();
        eventStaLB[j]->clear();
        eventComLB[j]->clear();
        eventTimeLB[j]->clear();
        eventPGALB[j]->clear();
    }

    //FIRST_EVENT_INFO=1558531133:KRSLG:역)서울역사:3.796869
    for(int j=0;j<eventFirstInfo.count();j++) // FIRST_EVENT_INFO=1551749617:GS:HJG:(가)합정관리소:48.789825
    {     
        if(eventFirstInfo.at(j).section(":", 1, 1) != "")
        {
            eventNetLB[j]->setText(eventFirstInfo.at(j).section(":", 1, 1).left(2));
            eventStaLB[j]->setText(eventFirstInfo.at(j).section(":", 1, 1));
            eventComLB[j]->setText(eventFirstInfo.at(j).section(":", 2, 2));
            t.setTime_t(eventFirstInfo.at(j).section(":", 0, 0).toInt());
            tKST = convertKST(t);
            eventTimeLB[j]->setText(tKST.toString("hh:mm:ss"));
            eventPGALB[j]->setText(eventFirstInfo.at(j).section(":", 3, 3));
        }
    }
    if(channel.left(1).startsWith("Z"))
        cfg.monChanID = 0;
    else if(channel.left(1).startsWith("N"))
        cfg.monChanID = 1;
    else if(channel.left(1).startsWith("E"))
        cfg.monChanID = 2;
    else if(channel.left(1).startsWith("H"))
        cfg.monChanID = 3;
    else if(channel.left(4).startsWith("T"))
        cfg.monChanID = 4;

    ui->monChanCB->setCurrentIndex(cfg.monChanID);
    changeChannel(cfg.monChanID);

    ui->eqTimeLB->clear();
    ui->eqLocLB->clear();
    ui->eqLoc2LB->clear();
    ui->eqMagLB->clear();

    if(haveEQ)
    {
        QDateTime eqTimeD, eqTimeDKST;
        eqTimeD.setTime_t(eqTime);
        eqTimeDKST = convertKST(eqTimeD);
        ui->eqTimeLB->setText(eqTimeDKST.toString("yyyy/MM/dd hh:mm:ss"));

        QProcess process;
        QString cmd = cfg.homeDir + "/bin/" + find_loc_program + " " + QString::number(eqLat, 'f', 4) + " " + QString::number(eqLon, 'f', 4);
        process.start(cmd);
        process.waitForFinished(-1); // will wait forever until finished

        QString stdout = process.readAllStandardOutput();
        int leng = stdout.length();
        eqLoc = stdout.left(leng-1);

        ui->eqLocLB->setText(eqLoc);
        ui->eqLoc2LB->setText("LAT:"+QString::number(eqLat, 'f', 4) + "    LON:"+QString::number(eqLon, 'f', 4));
        ui->eqMagLB->setText(QString::number(eqMag, 'f', 1));

        QString tempMag = QString::number(eqMag, 'f', 1);
        QMetaObject::invokeMethod(this->realObj, "showEEWStarMarker",
                                  Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, eqLat), Q_ARG(QVariant, eqLon),
                                  Q_ARG(QVariant, tempMag));
        QMetaObject::invokeMethod(this->stackedObj, "showEEWStarMarker",
                                  Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, eqLat), Q_ARG(QVariant, eqLon),
                                  Q_ARG(QVariant, tempMag));
    }
    else
    {
        haveEQ = false;
    }
}

void MainWindow::changeChannel(int monChan)
{
    cfg.monChanID = monChan;

    findMaxStations();
    if(maxPBClicked)
        drawMaxCircleOnMap();
    else
        valueChanged(ui->slider->value());
}

void MainWindow::valueChanged(int value)
{
    maxPBClicked = false;
    int dataTime = dataHouse.firstKey() + value;

    ui->maxPB->setStyleSheet("background-color: white");
    QDateTime time, timeKST;
    time.setTime_t(dataTime);
    timeKST = convertKST(time);

    ui->dataTimeLCD->display(timeKST.toString("hh:mm:ss"));

    if(haveEQ)
    {
        int diffTime = time.toTime_t() - eqTime;
        if(diffTime > 0)
        {
            double radiusP = diffTime * P_VEL * 1000;
            double radiusS = diffTime * pga_vel * 1000;

            QMetaObject::invokeMethod(this->realObj, "showCircleForAnimation",
                                      Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, eqLat), Q_ARG(QVariant, eqLon),
                                      Q_ARG(QVariant, radiusP), Q_ARG(QVariant, radiusS), Q_ARG(QVariant, 1));
            QMetaObject::invokeMethod(this->stackedObj, "showCircleForAnimation",
                                      Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, eqLat), Q_ARG(QVariant, eqLon),
                                      Q_ARG(QVariant, radiusP), Q_ARG(QVariant, radiusS), Q_ARG(QVariant, 1));
        }
        else
        {
            double radiusP = P_VEL * 1000;
            double radiusS = pga_vel * 1000;

            QMetaObject::invokeMethod(this->realObj, "showCircleForAnimation",
                                      Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, eqLat), Q_ARG(QVariant, eqLon),
                                      Q_ARG(QVariant, radiusP), Q_ARG(QVariant, radiusS), Q_ARG(QVariant, 0));
            QMetaObject::invokeMethod(this->stackedObj, "showCircleForAnimation",
                                      Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, eqLat), Q_ARG(QVariant, eqLon),
                                      Q_ARG(QVariant, radiusP), Q_ARG(QVariant, radiusS), Q_ARG(QVariant, 0));
        }
    }

    QList<_QSCD_FOR_MULTIMAP> pgaList = dataHouse.values(dataTime);

    resetStaCircleOnMap();

    int realstanum = 0;
    int staIndex, regendIndex, repeatIndex;
    QVector<int> indexVector;

    for(int i=0;i<staListVT.count();i++)
    {
        if(isFilter == 1)
        {
            if(staListVT.at(i).lastAlive != 1)
            {
                changeCircleOnMap("stackedMap", staListVT.at(i).index, 10, "white", 0);
                continue;
            }
        }

        if(staListVT.at(i).stackedPGA[value][cfg.monChanID] != 0)
        {
            QFuture<int> future = QtConcurrent::run(getRegendIndex, staListVT.at(i).stackedPGA[value][cfg.monChanID]);
            future.waitForFinished();
            regendIndex = future.result();

            changeCircleOnMap("stackedMap", i, pgaWidth[regendIndex], pgaColor[regendIndex], 1);
        }
        else
        {
            changeCircleOnMap("stackedMap", i, 10, "white", 0);
        }
    }

    for(int i=0;i<pgaList.count();i++)
    {
        staIndex = SSSSSVT.indexOf(pgaList.at(i).sta);
        repeatIndex = indexVector.indexOf(staIndex);

        if(staIndex != -1 && staListVT.at(staIndex).inUse == 1 && repeatIndex == -1)
        {         
            indexVector.push_back(staIndex);

            _STATION _sta;
            _sta = staListVT.at(staIndex);
            _sta.lastPGATime = time.toTime_t();
            _sta.lastPGA = pgaList.at(i).pga[cfg.monChanID];
            staListVT.replace(staIndex, _sta);

            if(isFilter == 1)
            {
                if(_sta.lastAlive != 1)
                {
                    changeCircleOnMap("realMap", staIndex, 10, "white", 0);
                    continue;
                }
            }

            realstanum++;

            QFuture<int> future = QtConcurrent::run(getRegendIndex, pgaList.at(i).pga[cfg.monChanID]);
            future.waitForFinished();
            regendIndex = future.result();

            changeCircleOnMap("realMap", staIndex, pgaWidth[regendIndex], pgaColor[regendIndex], 1);
        }
    }

    ui->staNumLB->setText(QString::number(realstanum));
}

void MainWindow::findMaxStations()
{
    ui->eventMaxPGATW->setRowCount(0);
    ui->eventMaxPGATW->setSortingEnabled(false);

    for(int i=0;i<staListVT.count();i++)
    {
        if(isFilter == 1)
        {
            if(staListVT.at(i).lastAlive != 1)
                continue;
        }

        if(staListVT.at(i).maxPGA[cfg.monChanID] == 0)
        {
                continue;
        }

        QDateTime t;
        t.setTime_t(staListVT.at(i).maxPGATime[cfg.monChanID]);
        t = convertKST(t);

        ui->eventMaxPGATW->setRowCount(ui->eventMaxPGATW->rowCount()+1);
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 0, new QTableWidgetItem(staListVT.at(i).sta));

        if(haveEQ)
        {
            QTableWidgetItem *distItem = new QTableWidgetItem;
            distItem->setData(Qt::EditRole, staListVT.at(i).distance);
            ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 1, distItem);
        }
        else
            ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 1, new QTableWidgetItem("nan"));

        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 2, new QTableWidgetItem(staListVT.at(i).comment));
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 3, new QTableWidgetItem(t.toString("hh:mm:ss")));

        QTableWidgetItem *pgaItem2 = new QTableWidgetItem;
        pgaItem2->setData(Qt::EditRole, myRound(staListVT.at(i).predictedPGA, 6));
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 4, pgaItem2);

        QTableWidgetItem *pgaItem = new QTableWidgetItem;
        pgaItem->setData(Qt::EditRole, myRound(staListVT.at(i).maxPGA[cfg.monChanID], 6));
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 5, pgaItem);
    }

    for(int j=0;j<ui->eventMaxPGATW->rowCount();j++)
    {
        for(int k=0;k<ui->eventMaxPGATW->columnCount();k++)
        {
            ui->eventMaxPGATW->item(j, k)->setTextAlignment(Qt::AlignCenter);
        }
    }

    ui->eventMaxPGATW->setSortingEnabled(true);

    if(haveEQ)
        ui->eventMaxPGATW->sortByColumn(1, Qt::AscendingOrder);
    else
        ui->eventMaxPGATW->sortByColumn(5, Qt::DescendingOrder);
}

void MainWindow::createStaCircleOnMap()
{
    // create staCircle
    QMetaObject::invokeMethod(this->realObj, "clearMap", Q_RETURN_ARG(QVariant, returnedValue));
    QMetaObject::invokeMethod(this->stackedObj, "clearMap", Q_RETURN_ARG(QVariant, returnedValue));

    for(int i=0;i<staListVT.count();i++)
    {
        if(staListVT.at(i).inUse == 1)
        {
            QMetaObject::invokeMethod(this->realObj, "createStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, staListVT.at(i).index), Q_ARG(QVariant, staListVT.at(i).lat),
                                      Q_ARG(QVariant, staListVT.at(i).lon), Q_ARG(QVariant, 10), Q_ARG(QVariant, "white"),
                                      Q_ARG(QVariant, staListVT.at(i).sta.left(2) + "/" + staListVT.at(i).sta.mid(2)),
                                      Q_ARG(QVariant, 0));

            QMetaObject::invokeMethod(this->stackedObj, "createStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, staListVT.at(i).index), Q_ARG(QVariant, staListVT.at(i).lat),
                                      Q_ARG(QVariant, staListVT.at(i).lon), Q_ARG(QVariant, 10), Q_ARG(QVariant, "white"),
                                      Q_ARG(QVariant, staListVT.at(i).sta.left(2) + "/" + staListVT.at(i).sta.mid(2)),
                                      Q_ARG(QVariant, 0));
        }
    }
}

void MainWindow::resetStaCircleOnMap()
{
    for(int i=0;i<staListVT.count();i++)
    {
        if(staListVT.at(i).inUse == 1)
        {
            changeCircleOnMap("realMap", staListVT.at(i).index, 10, "white", 1);
            changeCircleOnMap("stackedMap", staListVT.at(i).index, 10, "white", 1);
        }
    }
}

void MainWindow::drawMaxCircleOnMap()
{
    maxPBClicked = true;
    ui->maxPB->setStyleSheet("background-color: green");
    ui->play1PB->setStyleSheet("background-color: white");
    ui->play2PB->setStyleSheet("background-color: white");
    ui->play4PB->setStyleSheet("background-color: white");
    ui->stopPB->setStyleSheet("background-color: white");

    resetStaCircleOnMap();

    int regendIndex, regendIndex2;
    for(int i=0;i<staListVT.count();i++)
    {
        if(isFilter == 1)
        {
            if(staListVT.at(i).lastAlive != 1)
            {
                changeCircleOnMap("realMap", staListVT.at(i).index, 10, "white", 0);
                changeCircleOnMap("stackedMap", staListVT.at(i).index, 10, "white", 0);

                continue;
            }
        }

        if(staListVT.at(i).inUse == 1 && staListVT.at(i).maxPGA[cfg.monChanID] != 0)
        {
            QFuture<int> future = QtConcurrent::run(getRegendIndex, staListVT.at(i).maxPGA[cfg.monChanID]);
            future.waitForFinished();
            regendIndex = future.result();

            //QFuture<int> future2 = QtConcurrent::run(getRegendIndex, abs(staListVT.at(i).predictedPGA - staListVT.at(i).maxPGA[cfg.monChanID]));
            QFuture<int> future2 = QtConcurrent::run(getRegendIndex, staListVT.at(i).predictedPGA);
            future2.waitForFinished();
            regendIndex2 = future2.result();

            changeCircleOnMap("realMap", staListVT.at(i).index, pgaWidth[regendIndex2], pgaColor[regendIndex2], 1);
            changeCircleOnMap("stackedMap", staListVT.at(i).index, pgaWidth[regendIndex], pgaColor[regendIndex], 1);
        }
        else
        {
            changeCircleOnMap("stackedMap", staListVT.at(i).index, 10, "white", 0);
        }
    }
}

void MainWindow::filterChanged(int state)
{
    isFilter = conBG->checkedId();

    findMaxStations();

    if(maxPBClicked)
        drawMaxCircleOnMap();
    else
        valueChanged(ui->slider->value());
}

void MainWindow::decorationGUI()
{
    conBG = new QButtonGroup(this);
    conBG->addButton(ui->bFilterCB); conBG->setId(ui->bFilterCB, 0);
    conBG->addButton(ui->aFilterCB); conBG->setId(ui->aFilterCB, 1);

    connect(ui->bFilterCB, SIGNAL(stateChanged(int)), this, SLOT(filterChanged(int)));

    ui->bFilterCB->setEnabled(false);
    ui->aFilterCB->setEnabled(false);
    ui->viewFilterPB->setEnabled(false);

    QPixmap titlePX("/.RTICOM2/images/RTICOM2PlayerLogo.png");
    ui->titleLB->setPixmap(titlePX.scaled(ui->titleLB->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    for(int i=0;i<MAX_NUM_EVENTINITSTA;i++)
    {
        eventNetLB[i] = new QLabel;
        eventNetLB[i]->setFixedSize(65, 21);
        eventNetLB[i]->setAlignment(Qt::AlignCenter);
        eventNetLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventNetLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventNetLB[i], i+1, 0);
        eventStaLB[i] = new QLabel;
        eventStaLB[i]->setFixedSize(70, 21);
        eventStaLB[i]->setAlignment(Qt::AlignCenter);
        eventStaLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventStaLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventStaLB[i], i+1, 1);
        eventComLB[i] = new QLabel;
        eventComLB[i]->setFixedSize(163, 21);
        eventComLB[i]->setAlignment(Qt::AlignCenter);
        eventComLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventComLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventComLB[i], i+1, 2);
        eventTimeLB[i] = new QLabel;
        eventTimeLB[i]->setFixedSize(80, 21);
        eventTimeLB[i]->setAlignment(Qt::AlignCenter);
        eventTimeLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventTimeLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventTimeLB[i], i+1, 3);
        eventPGALB[i] = new QLabel;
        eventPGALB[i]->setFixedSize(80, 21);
        eventPGALB[i]->setAlignment(Qt::AlignCenter);
        eventPGALB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventPGALB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventPGALB[i], i+1, 4);
    }

    ui->eventMaxPGATW->setColumnWidth(0, 70);
    ui->eventMaxPGATW->setColumnWidth(1, 90);
    ui->eventMaxPGATW->setColumnWidth(2, 140);
    ui->eventMaxPGATW->setColumnWidth(3, 70);
    ui->eventMaxPGATW->setColumnWidth(4, 70);
}
