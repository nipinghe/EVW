/* *****************************************************************************
Copyright (c) 2016-2017, The Regents of the University of California (Regents).
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS 
PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, 
UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

*************************************************************************** */

// Written: fmckenna

#include "MainWindow.h"
#include "EarthquakeRecord.h"
#include "sectiontitle.h"

#include <HeaderWidget.h>
#include <FooterWidget.h>


#include <QDebug>
#include <QSlider>
//#include <QtNetwork>
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <qcustomplot.h>
#include <MyGlWidget.h>

#include "WindSim.h"

//#include <../widgets/InputSheetBM/SimpleSpreadsheetWidget.h>
#include <SimpleSpreadsheetWidget.h>
#include <ResponseWidget.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QHostInfo>

// OpenSees include files
#include <Node.h>
#include <ID.h>
#include <SP_Constraint.h>
#include <MP_Constraint.h>
#include <Domain.h>
#include <StandardStream.h>
//#include <LinearCrdTransf3d.h>
#include <Steel01.h>
#include <ZeroLength.h>
#include <LegendreBeamIntegration.h>
#include <ElasticSection3d.h>
#include <LinearSeries.h>
#include <NodalLoad.h>
#include <LoadPattern.h>
#include <SimulationInformation.h>
#include <PathSeries.h>
#include <GroundMotion.h>
#include <UniformExcitation.h>

#include <Newmark.h>
#include <RCM.h>
#include <PlainNumberer.h>
#include <NewtonRaphson.h>
#include <CTestNormDispIncr.h>
#include <PlainHandler.h>
#include <PenaltyConstraintHandler.h>
#include <ProfileSPDLinDirectSolver.h>
#include <ProfileSPDLinSOE.h>
#ifdef _FORTRAN_LIBS
#include <BandGenLinSOE.h>
#include <BandGenLinLapackSolver.h>
#endif
#include <DirectIntegrationAnalysis.h>
#include <AnalysisModel.h>
#include "elCentro.AT2"
#include <Vector.h>
#include <SymBandEigenSOE.h>
#include <SymBandEigenSolver.h>

//style inludes
#include <QGroupBox>
#include <QFrame>

StandardStream sserr;
OPS_Stream *opserrPtr = &sserr;
Domain theDomain;

using namespace Wind;


void
MainWindow::errorMessage(QString mesg) {
QMessageBox::warning(this, tr("Application"), mesg);
}

//
// procedure to create a QLabel QLineInput pair, returns pointer to QLineEdit created
//

QLineEdit *
createTextEntry(QString text,
                QVBoxLayout *theLayout,
                int minL=100,
                int maxL=100,
                QString *unitText =0)
{
    QHBoxLayout *entryLayout = new QHBoxLayout();
    QLabel *entryLabel = new QLabel();
    entryLabel->setText(text);

    QLineEdit *res = new QLineEdit();
    res->setMinimumWidth(minL);
    res->setMaximumWidth(maxL);
    res->setValidator(new QDoubleValidator);

    entryLayout->addWidget(entryLabel);
    entryLayout->addStretch();
    entryLayout->addWidget(res);

    if (unitText != 0) {
        QLabel *unitLabel = new QLabel();
        unitLabel->setText(*unitText);
        unitLabel->setMinimumWidth(40);
        unitLabel->setMaximumWidth(50);
        entryLayout->addWidget(unitLabel);

    }

    entryLayout->setSpacing(10);
    entryLayout->setMargin(0);

    theLayout->addLayout(entryLayout);


    return res;
}


//
// procedure to create a QLabel QLabel pair, returns pointer to QLabel created
//


QLabel *
createLabelEntry(QString text,
                 QVBoxLayout *theLayout,
                 int minL=100,
                 int maxL=100,
                 QString *unitText = 0)
{
    QHBoxLayout *entryLayout = new QHBoxLayout();
    QLabel *entryLabel = new QLabel();
    entryLabel->setText(text);

    QLabel *res = new QLabel();
    res->setMinimumWidth(minL);
    res->setMaximumWidth(maxL);
    res->setAlignment(Qt::AlignRight);

    entryLayout->addWidget(entryLabel);
    entryLayout->addStretch();
    entryLayout->addWidget(res);

    if (unitText != 0) {
        QLabel *unitLabel = new QLabel();
        unitLabel->setText(*unitText);
        unitLabel->setMinimumWidth(40);
        unitLabel->setMaximumWidth(100);
        entryLayout->addWidget(unitLabel);

    }
    entryLayout->setSpacing(10);
    entryLayout->setMargin(0);

    theLayout->addLayout(entryLayout);

    return res;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    numFloors(0), periods(0), buildingW(0), buildingH(1), storyK(0),
    weights(0), k(0), fy(0), b(0), dampRatios(0), floorHeights(0), storyHeights(0),
    dampingRatio(0.02), g(386.4), dt(0), gMotion(0),
    includePDelta(true), needAnalysis(true), analysisFailed(false), motionData(0),
    dispResponsesEarthquake(0), storyForceResponsesEarthquake(0), storyDriftResponsesEarthquake(0), floorForcesEarthquake(0),
    dispResponsesWind(0), storyForceResponsesWind(0), storyDriftResponsesWind(0), floorForcesWind(0),
    maxDisp(1),
    movingSlider(false), fMinSelected(-1),fMaxSelected(-1), sMinSelected(-1),sMaxSelected(-1),
    time(1561),excitationValues(1561), graph(0), groupTracer(0),floorSelected(-1),storySelected(-1)
{

    //
    // user settings
    //

    QSettings settings("SimCenter", "uqFEM");
    QVariant savedValue = settings.value("uuid");
    QUuid uuid;
    if (savedValue.isNull()) {
        uuid = QUuid::createUuid();
        settings.setValue("uuid",uuid);
    } else
        uuid =savedValue.toUuid();

    scaleFactor = 1.0;
    eigValues = new Vector;
    createActions();

    // create a main layout
    mainLayout = new QHBoxLayout();
    largeLayout = new QVBoxLayout();

    // create input and output panel layouts and each to main layout
    createHeaderBox();
    createInputPanel();
    createOutputPanel();
    createFooterBox();

    // create a widget in which to show everything //ALSO SET TO LARGE LAYOUT
    QWidget *widget = new QWidget();
    widget->setLayout(largeLayout);
    this->setCentralWidget(widget);

    QRect rec = QApplication::desktop()->screenGeometry();

    int height = 0.7*rec.height();
    int width = 0.7*rec.width();

    this->resize(width, height);

    //
    // create 2 blank motions & make elCentro current
    //

    QFile file(":/images/ElCentro.json");
    if(file.open(QFile::ReadOnly)) {
       QString jsonText = QLatin1String(file.readAll());
       QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonText.toUtf8());
       QJsonObject jsonObj = jsonDoc.object();
       EarthquakeRecord *elCentro = new EarthquakeRecord();
       elCentro->inputFromJSON(jsonObj);

       QString recordString("ElCentro");
       records.insert(std::make_pair(QString("ElCentro"), elCentro));
       eqMotion->addItem(recordString);
    }

    QFile fileR(":/images/Rinaldi.json");
    if(fileR.open(QFile::ReadOnly)) {
       QString jsonText = QLatin1String(fileR.readAll());
       QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonText.toUtf8());
       QJsonObject jsonObj = jsonDoc.object();
       EarthquakeRecord *rinaldi = new EarthquakeRecord();
       rinaldi->inputFromJSON(jsonObj);

       QString recordString("Northridge-Rinaldi");
       records.insert(std::make_pair(QString("Northridge-Rinaldi"), rinaldi));
       eqMotion->addItem(recordString);
    }

    // create a basic model with defaults
    this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
    this->on_inEarthquakeMotionSelectionChanged("ElCentro");

    // access a web page which will increment the usage count for this tool
    manager = new QNetworkAccessManager(this);

    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    manager->get(QNetworkRequest(QUrl("http://opensees.berkeley.edu/OpenSees/developer/evw/use.php")));

    //
    // google analytics
    // ref: https://developers.google.com/analytics/devguides/collection/protocol/v1/reference
    //


    QNetworkRequest request;
    QUrl host("http://www.google-analytics.com/collect");
    request.setUrl(host);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    // setup parameters of request
    QString requestParams;
    QString hostname = QHostInfo::localHostName() + "." + QHostInfo::localDomainName();
    requestParams += "v=1"; // version of protocol
    requestParams += "&tid=UA-121618417-1"; // Google Analytics account
    requestParams += "&cid=" + uuid.toString(); // unique user identifier
    requestParams += "&t=event";  // hit type = event others pageview, exception
    requestParams += "&an=EVW";   // app name
    requestParams += "&av=1.0.0"; // app version
    requestParams += "&ec=EVW";   // event category
    requestParams += "&ea=start"; // event action

    // send request via post method
    manager->post(request, requestParams.toStdString().c_str());

}

MainWindow::~MainWindow()
{
    delete ui;

    if (weights != 0)
        delete [] weights;
    if (k != 0)
        delete [] k;
    if (fy != 0)
        delete [] fy;
    if (b != 0)
        delete [] b;
    if (gMotion != 0)
        delete [] gMotion;
    if (storyHeights != 0)
        delete [] storyHeights;
    if (floorHeights != 0)
        delete [] floorHeights;
    if (dampRatios != 0)
        delete [] dampRatios;

    delete manager;
}

void MainWindow::draw(MyGlWidget *theGL, int loadingType)
{
    double **theDispResponse = dispResponsesWind;
    if (loadingType == 0)
        theDispResponse = dispResponsesEarthquake;

    if (needAnalysis == true) {
        doAnalysis();
    }
    theGL->reset();

    for (int i=0; i<numFloors; i++) {
        if (i == storySelected)
            theGL->drawLine(i+1+numFloors, theDispResponse[i][currentStep],floorHeights[i],
                            theDispResponse[i+1][currentStep],floorHeights[i+1], 2, 1, 0, 0);
        else if (i >= sMinSelected && i <= sMaxSelected)
            theGL->drawLine(i+1+numFloors, theDispResponse[i][currentStep],floorHeights[i],
                           theDispResponse[i+1][currentStep],floorHeights[i+1], 2, 1, 0, 0);
        else
            theGL->drawLine(i+1+numFloors, theDispResponse[i][currentStep],floorHeights[i],
                            theDispResponse[i+1][currentStep],floorHeights[i+1], 2, 0, 0, 0);
    }
    
    for (int i=0; i<=numFloors; i++) {
        if (i == floorSelected)
            theGL->drawPoint(i,  theDispResponse[i][currentStep],floorHeights[i], 10, 1, 0, 0);
        else if (i >= fMinSelected && i <= fMaxSelected)
            theGL->drawPoint(i, theDispResponse[i][currentStep],floorHeights[i], 10, 1, 0, 0);
        else
            theGL->drawPoint(i, theDispResponse[i][currentStep],floorHeights[i], 10, 0, 0, 1);
    }

    // display range of displacement
    static char maxDispString[50];
    snprintf(maxDispString, 50, "%.3e", maxDisp);
    theGL->drawLine(0, -maxDisp, 0.0, maxDisp, 0.0, 1.0, 0., 0., 0.);

    currentTime->setText(QString().setNum(currentStep*dt,'f',2));
    if (loadingType == 0)
           currentDispEarthquake->setText(QString().setNum(theDispResponse[numFloors][currentStep],'f',2));
    else
        currentDispWind->setText(QString().setNum(theDispResponse[numFloors][currentStep],'f',2));
    theGL->drawBuffers();

}



void MainWindow::updatePeriod()
{
    //periods = 2.0;
}

void MainWindow::setBasicModel(int numF, double W, double H, double K, double zeta, double grav, double width, double depth)
{
    if (numFloors != numF) {

        if (dispResponsesEarthquake != 0) {
            for (int j=0; j<numFloors+1; j++)
                delete [] dispResponsesEarthquake[j];
            delete [] dispResponsesEarthquake;
        }
        if (storyForceResponsesEarthquake != 0) {
            for (int j=0; j<numFloors; j++)
                delete [] storyForceResponsesEarthquake[j];
            delete [] storyForceResponsesEarthquake;
        }
        if (storyDriftResponsesEarthquake != 0) {
            for (int j=0; j<numFloors; j++)
                delete [] storyDriftResponsesEarthquake[j];
            delete [] storyDriftResponsesEarthquake;
        }

        if (floorForcesEarthquake != 0) {
            for (int j=0; j<numFloors; j++)
                delete [] floorForcesEarthquake[j];
            delete [] floorForcesEarthquake;
        }

        if (dispResponsesWind != 0) {
            for (int j=0; j<numFloors+1; j++)
                delete [] dispResponsesWind[j];
            delete [] dispResponsesWind;
        }
        if (storyForceResponsesWind != 0) {
            for (int j=0; j<numFloors; j++)
                delete [] storyForceResponsesWind[j];
            delete [] storyForceResponsesWind;
        }
        if (storyDriftResponsesWind != 0) {
            for (int j=0; j<numFloors; j++)
                delete [] storyDriftResponsesWind[j];
            delete [] storyDriftResponsesWind;
        }
        if (floorForcesWind != 0) {
            for (int j=0; j<numFloors; j++)
                delete [] floorForcesWind[j];
            delete [] floorForcesWind;
        }


        dispResponsesEarthquake = 0;
        dispResponsesWind = 0;
        storyDriftResponsesEarthquake = 0;
        storyDriftResponsesWind = 0;
        storyForceResponsesEarthquake = 0;
        storyForceResponsesWind = 0;
        floorForcesEarthquake = 0;
        floorForcesWind = 0;

        // if invalid numFloor, return
        if (numF <= 0) {
            numF = 1;
        }

        // resize arrays
        if (weights != 0)
            delete [] weights;
        if (k != 0)
            delete [] k;
        if (fy != 0)
            delete [] fy;
        if (b != 0)
            delete [] b;
        if (floorHeights != 0)
            delete [] floorHeights;
        if (storyHeights != 0)
            delete [] storyHeights;
        if (dampRatios != 0)
            delete [] dampRatios;

        weights = new double[numF];
        k = new double[numF];
        fy = new double[numF];
        b = new double[numF];
        floorHeights = new double[numF+1];
        storyHeights = new double[numF];
        dampRatios = new double[numF];
    }

    // set values
    double floorW = W/(numF);
    buildingW = W;
    storyK = K;
    numFloors = numF;
    buildingH = H;
    dampingRatio = zeta;
    g=grav;
    buildingWidth = width;
    buildingDepth = depth;

    this->setData(numSteps, dt, eqData);

    for (int i=0; i<numF; i++) {
        weights[i] = floorW;
        k[i] = K;
        fy[i] = 1.0e100;
        b[i] = 0.;
        floorHeights[i] = i*buildingH/(1.*numF);
        storyHeights[i] = buildingH/(1.*numF);
        dampRatios[i] = zeta;
    }
    floorHeights[numF] = buildingH;

    this->updatePeriod();

    scaleFactorEQ->setText(QString::number(scaleFactor));
    inWeight->setText(QString::number(buildingW));
    inK->setText(QString::number(storyK));
    inFloors->setText(QString::number(numF));
    inHeight->setText(QString::number(buildingH));
    inDamping->setText(QString::number(zeta));

    squareHeight->setText((QString::number(buildingH)));
    rectangularHeight->setText((QString::number(buildingH)));
    circularHeight->setText((QString::number(buildingH)));
    squareWidth->setText((QString::number(width)));
    circularDiameter->setText((QString::number(width)));
    rectangularWidth->setText((QString::number(width)));
    rectangularDepth->setText((QString::number(depth)));

    needAnalysis = true;

    this->reset();

    theNodeResponse->setItem(numF);
    theForceDispResponse->setItem(1);
    theForceTimeResponse->setItem(1);
    theAppliedForcesResponse->setItem(numF);

    floorMassFrame->setVisible(false);
    storyPropertiesFrame->setVisible(false);
    spreadSheetFrame->setVisible(true);
}


void
MainWindow::on_includePDeltaChanged(int state)
{
    if (state == Qt::Checked)
        includePDelta = true;
    else
        includePDelta = false;

    this->reset();
}


void MainWindow::on_inFloors_editingFinished()
{
    QString textFloors =  inFloors->text();
    int numFloorsText = textFloors.toInt();
    if (numFloorsText != numFloors) {
        this->setBasicModel(numFloorsText, buildingW, buildingH, storyK, dampingRatio, g, buildingWidth, buildingDepth);
        floorSelected = -1;
        storySelected = -1;
        fMinSelected = -1;
        fMaxSelected = -1;
        sMinSelected = -1;
        sMaxSelected = -1;
       // this->setSelectionBoundary(-1.,-1.);
    }
}

void MainWindow::on_inWeight_editingFinished()
{
    QString textW =  inWeight->text();
    double textToDoubleW = textW.toDouble();
    if (textToDoubleW != buildingW) {
        buildingW = textToDoubleW;
        double floorW = buildingW/(numFloors);

        for (int i=0; i<numFloors; i++) {
            weights[i] = floorW;
        }
         this->updatePeriod();
        needAnalysis = true;
        this->reset();

        //inHeight->setFocus();
    }
}

void MainWindow::on_inHeight_editingFinished()
{
    QString textH =  inHeight->text();
    if (textH.isNull())
        return;
    double textToDoubleH = textH.toDouble();
    if (textToDoubleH != buildingH) {
        buildingH = textToDoubleH;

        for (int i=0; i<numFloors; i++) {
            floorHeights[i] = i*buildingH/(1.*numFloors);
            storyHeights[i] = buildingH/(1.*numFloors);
        }
        floorHeights[numFloors] = buildingH;

        this->updatePeriod();
        needAnalysis = true;
        this->reset();
    }
}


void MainWindow::on_inK_editingFinished()
{
    QString text =  inK->text();
    if (text.isNull())
        return;

    double textToDouble = text.toDouble();
    if (textToDouble != storyK) {
        storyK = textToDouble;

        for (int i=0; i<numFloors; i++) {
            k[i] = storyK;
        }

        this->updatePeriod();
        needAnalysis = true;
        this->reset();
        //this->setBasicModel(numFloors, buildingW, buildingH, storyK, dampingRatio, g);
    }
}

void MainWindow::on_inDamping_editingFinished()
{
    QString text =  inDamping->text();
    if (text.isNull())
        return;

    double textToDouble = text.toDouble();
    if (dampingRatio != textToDouble) {
        if (textToDouble < 0 || textToDouble > 1.0) {
            inDamping->setText(QString::number(dampingRatio));
            return;
        }
        dampingRatio = textToDouble;
        //    inGravity->setFocus();
        for (int i=0; i<numFloors; i++) {
            dampRatios[i]=dampingRatio;
        }
        this->reset();
    }
}

void MainWindow::on_dragCoefficient_editingFinished()
{
    QString text =  dragCoefficient->text();
    if (text.isNull())
        return;

    needAnalysis=true;
    this->reset();
}

void MainWindow::on_windGustSpeed_editingFinished()
{
    QString text =  windGustSpeed->text();
    if (text.isNull())
        return;

    needAnalysis=true;
    this->reset();
}
void MainWindow::on_expCatagory_indexChanged()
{
    needAnalysis=true;
    this->reset();
}


void MainWindow::on_Seed_editingFinished()
{
    QString text =  seed->text();
    if (text.isNull())
        return;

    needAnalysis=true;
    this->reset();
}

void MainWindow::on_scaleFactor_editingFinished()
{
    QString text =  scaleFactorEQ->text();

    if (text.isNull())
        return;

    double textToDouble = text.toDouble();

    if (scaleFactor != textToDouble) {

        scaleFactor = textToDouble;

        theCurrentRecord->setScaleFactor(scaleFactor);

        needAnalysis=true;

        this->reset();
    }
}

void MainWindow::on_inGravity_editingFinished()
{

}

void MainWindow::on_inFloorWeight_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inFloorWeight->text();
    if (text.isNull())
        return;

    double textToDouble = text.toDouble();
    for (int i=fMinSelected; i<=fMaxSelected; i++)
        weights[i] = textToDouble;

    buildingW = 0;
    for (int i=0; i<numFloors; i++)
        buildingW = buildingW+weights[i];

    inWeight->setText(QString::number(buildingW));
    this->reset();
}



void MainWindow::on_inStoryHeight_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryHeight->text();
    if (text.isNull())
        return;

    double newStoryHeight = text.toDouble();
    double currentStoryHeight = 0;
    double *newFloorHeights = new double[numFloors+1];

    // determine new floor heights, cludgy can rewrite now store storyHeights
    newFloorHeights[0] = 0;
    for (int i=0; i<sMinSelected; i++)
        newFloorHeights[i+1] = floorHeights[i+1];

    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        newFloorHeights[i+1] = newFloorHeights[i]+newStoryHeight;
        storyHeights[i] = newStoryHeight;
    }

    for (int i=sMaxSelected+1; i<numFloors; i++)
        newFloorHeights[i+1] = newFloorHeights[i]+floorHeights[i+1]-floorHeights[i];



    bool needReset = false;
    for (int i=0; i<=numFloors; i++) {
        if (floorHeights[i] != newFloorHeights[i]){
            needReset = true;
            i=numFloors+1;
        }
    }


    delete [] floorHeights;
    floorHeights = newFloorHeights;

    // move focus, update graphic and set analysis flag
    //  inStoryK->setFocus();
    if (needReset == true) {
        // delete old array and reset pointer
        this->reset();
        // delete old array and reset pointer
        buildingH = newFloorHeights[numFloors];
        inHeight->setText(QString::number(buildingH));
    }
}

void MainWindow::on_inStoryK_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryK->text();
    if (text.isNull())
        return;

    double textToDouble = text.toDouble();
    bool needReset = false;
    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        if (k[i] != textToDouble) {
            k[i] = textToDouble;
            needReset = true;
        }
    }

    //inStoryFy->setFocus();
    if (needReset == true)
        this->reset();
}

void MainWindow::on_inStoryFy_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryFy->text();
    if (text.isNull())
        return;

    bool needReset = false;
    double textToDouble = text.toDouble();
    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        if (fabs(fy[i]-textToDouble) > 1e-12) {
            fy[i] = textToDouble;
            needReset = true;
        }
    }
    //inStoryB->setFocus();
    if (needReset == true)
        this->reset();
}

void MainWindow::on_inStoryB_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryB->text();
    if (text.isNull())
        return;

    bool needReset = false;
    double textToDouble = text.toDouble();
    for (int i=sMinSelected; i<=sMaxSelected; i++)
        if (b[i] != textToDouble) {
            b[i] = textToDouble;
            needReset = true;
        }

    //inStoryHeight->setFocus();
    if (needReset == true)
        this->reset();
}


void
MainWindow::doEarthquakeAnalysis() {

    theDomain.clearAll();
    OPS_clearAllUniaxialMaterial();
    ops_Dt = 0.0;

    //
    // create the nodes
    //

    Node **theNodes = new Node *[numFloors+1];
    for (int i=0; i<= numFloors; i++) {
        Matrix theMass(1,1);
        Node *theNode=new Node(i+1,1, 0.0);
        theNodes[i] = theNode;
        theDomain.addNode(theNode);
        if (i == 0) {
            SP_Constraint *theSP = new SP_Constraint(i+1, 0, 0., true);
            theDomain.addSP_Constraint(theSP);
        } else {
            theMass(0,0) = weights[i-1]/g;
            theNode->setMass(theMass);
        }
    }


    // create the vectors for the element orientation
    Vector x(3); x(0) = 1.0; x(1) = 0.0; x(2) = 0.0;
    Vector y(3); y(0) = 0.0; y(1) = 1.0; y(2) = 0.0;

    double axialLoad = 0;
    ZeroLength **theElements = new ZeroLength *[numFloors];
    for (int i=numFloors;  i>0; i--) {
        UniaxialMaterial *theMat = new Steel01(i,fy[i-1],k[i-1],b[i-1]);
        //  ZeroLength *theEle = new ZeroLength(i+1, 1, i+1, i+2,
        //x, y, *theMat, 0);
        double PdivL = 0.0;
        if (includePDelta == true && storyHeights[i-1] != 0) {
            axialLoad = axialLoad + weights[i-1];
            PdivL = -axialLoad/storyHeights[i-1]; // negative for compression
        }
        ZeroLength *theEle = new ZeroLength(i, i, i+1, *theMat, PdivL);
        theElements[i-1] = theEle;
        theDomain.addElement(theEle);
        delete theMat; // each ele makes it's own copy
    }

    //
    // create load pattern and add loads
    //

    PathSeries *theSeries;
    theSeries = new PathSeries(1, *motionData, dt, g*scaleFactor);

    GroundMotion *theGroundMotion = new GroundMotion(0,0,theSeries);
    LoadPattern *theLoadPattern = new UniformExcitation(*theGroundMotion, 0, 1);

    theDomain.addLoadPattern(theLoadPattern);

    //
    // create the analysis
    //

    AnalysisModel     *theModel = new AnalysisModel();
    CTestNormDispIncr *theTest = new CTestNormDispIncr(1.0e-10, 20, 0);
    EquiSolnAlgo      *theSolnAlgo = new NewtonRaphson();//INITIAL_TANGENT);
    TransientIntegrator  *theIntegrator = new Newmark(0.5, 0.25);
    //ConstraintHandler *theHandler = new TransformationConstraintHandler();
    ConstraintHandler *theHandler = new PlainHandler();
    RCM               *theRCM = new RCM();
    DOF_Numberer      *theNumberer = new DOF_Numberer(*theRCM);
#ifdef _FORTRAN_LIBS
    BandGenLinSolver  *theSolver = new BandGenLinLapackSolver();
    LinearSOE         *theSOE = new BandGenLinSOE(*theSolver);
#else
    ProfileSPDLinSolver  *theSolver = new ProfileSPDLinDirectSolver();
    LinearSOE         *theSOE = new ProfileSPDLinSOE(*theSolver);
#endif
    DirectIntegrationAnalysis    theAnalysis(theDomain,
                                             *theHandler,
                                             *theNumberer,
                                             *theModel,
                                             *theSolnAlgo,
                                             *theSOE,
                                             *theIntegrator);
    theSolnAlgo->setConvergenceTest(theTest);

    SymBandEigenSolver *theEigenSolver = new SymBandEigenSolver();
    EigenSOE *theEigenSOE = new SymBandEigenSOE(*theEigenSolver, *theModel);
    theAnalysis.setEigenSOE(*theEigenSOE);

    int ok = theAnalysis.eigen(numFloors,true);
    const Vector &theEig = theDomain.getEigenvalues();
    if (eigValues->Size() != numFloors) {
        eigValues->resize(numFloors);
    }
    *eigValues = theEig;
    if (ok == 0)
        for (int i=0; i<numFloors; i++)
            if (theEig(i) <= 0)
                ok = -1;

    int numCombo = periodComboBox->count();

    if (numCombo != numFloors) {
        periodComboBox->clear();
        QString t1 = QString("Fundamental Period");
        periodComboBox->addItem(t1);
        for (int i=1; i<numFloors; i++) {
            QString t1 = QString("T") + QString::number(i+1);
            periodComboBox->addItem(t1);
        }
    }

    if (ok != 0) {
        needAnalysis = false;
        analysisFailed = true;
        for (int i=0; i<numSteps; i++) {
            for (int j=0; j<numFloors+1; j++) {
                dispResponsesEarthquake[j][i] = 0;
                if (j < numFloors) {
                    storyForceResponsesEarthquake[j][i]=0;
                    storyDriftResponsesEarthquake[j][i]=0;
                }
            }
        }
        maxEarthquakeDispLabel->setText(QString().setNum(0.0,'f',2));
        currentPeriod->setText(QString(tr("undefined")));
        delete [] theNodes;
        delete [] theElements;

        QMessageBox::warning(this, tr("Application"),
                             tr("Eigenvalue Analysis Failed.<p> Possible Causes: negstive mass, negative story stiffness, or "
                                "if PDelta is included, a story stiffness - axial load divided by L is negtive resulting "
                                "in non postive definite stiffness matrix"));

        return;
    }

    Vector dampValues(numFloors);
    for (int i=0; i<numFloors; i++) {
        dampValues(i)=dampRatios[i];
    }
    theDomain.setModalDampingFactors(&dampValues);
  // theDomain.setRayleighDampingFactors(0.001,9.,0.,0.);

    double T1 = 2*3.14159/sqrt(theEig(0));
    for (int i=0; i<=numSteps; i++) {

        int ok = theAnalysis.analyze(1, dt);
        if (ok != 0) {
            needAnalysis = false;
            analysisFailed = true;
            for (int k=i; k<numSteps; k++) {
                for (int j=0; j<numFloors+1; j++) {
                    dispResponsesEarthquake[j][i] = 0;
                    if (j < numFloors) {
                        storyForceResponsesEarthquake[j][k]=0;
                        storyDriftResponsesEarthquake[j][k]=0;
                    }
                }

            }
            QMessageBox::warning(this, tr("Application"),
                                 tr("Transient Analysis Failed under Earthquake Loading"));
            break;
        }
        for (int j=0; j<numFloors+1; j++) {
            double nodeDisp = theNodes[j]->getDisp()(0);
            dispResponsesEarthquake[j][i] = nodeDisp;
            if (fabs(nodeDisp) > maxDisp)
                maxDisp = fabs(nodeDisp);

            if (j < numFloors) {
                const Vector &load = theNodes[j+1]->getUnbalancedLoad();
                floorForcesEarthquake[j][i] = load(0);
                storyForceResponsesEarthquake[j][i]= theElements[j]->getForce();
                storyDriftResponsesEarthquake[j][i] = theElements[j]->getDrift();
            }
        }
    }

    // clean up memory
    delete [] theNodes;
    delete [] theElements;

    // reset values, i.e. slider position, current displayed step...
    maxEarthquakeDispLabel->setText(QString().setNum(maxDisp,'f',2));

    int num = periodComboBox->currentIndex();
    double eigenValue = (*eigValues)(num);
    if (eigenValue <= 0) {
        currentPeriod->setText(QString("undefined"));
    } else {
        double period = 2*3.14159/sqrt((eigenValue));
        currentPeriod->setText(QString().setNum(period,'f',2));
    }

}



void 
MainWindow::doWindAnalysis() {

    theDomain.clearAll();
    OPS_clearAllUniaxialMaterial();
    ops_Dt = 0.0;

    //
    // create the nodes
    //

    Node **theNodes = new Node *[numFloors+1];
    for (int i=0; i<= numFloors; i++) {
        Matrix theMass(1,1);
        Node *theNode=new Node(i+1,1, 0.0);
        theNodes[i] = theNode;
        theDomain.addNode(theNode);
        if (i == 0) {
            SP_Constraint *theSP = new SP_Constraint(i+1, 0, 0., true);
            theDomain.addSP_Constraint(theSP);
        } else {
            theMass(0,0) = weights[i-1]/g;
            theNode->setMass(theMass);
        }
    }

    // create the vectors for the element orientation
    Vector x(3); x(0) = 1.0; x(1) = 0.0; x(2) = 0.0;
    Vector y(3); y(0) = 0.0; y(1) = 1.0; y(2) = 0.0;

    double axialLoad = 0;
    ZeroLength **theElements = new ZeroLength *[numFloors];
    for (int i=numFloors;  i>0; i--) {
        UniaxialMaterial *theMat = new Steel01(i,fy[i-1],k[i-1],b[i-1]);
        //  ZeroLength *theEle = new ZeroLength(i+1, 1, i+1, i+2,
        //x, y, *theMat, 0);
        double PdivL = 0.0;
        if (includePDelta == true && storyHeights[i-1] != 0) {
            axialLoad = axialLoad + weights[i-1];
            PdivL = -axialLoad/storyHeights[i-1]; // negative for compression
        }
        ZeroLength *theEle = new ZeroLength(i, i, i+1, *theMat, PdivL);
        theElements[i-1] = theEle;
        theDomain.addElement(theEle);
        delete theMat; // each ele makes it's own copy
    }

    //
    // create load pattern and add loads
    //

   // std::vector<double> hfloors = {288.0, 144.0, 144.0, 144.0, 144.0};

    std::vector<double> hfloors(numFloors);
    for (int i=0; i<numFloors; i++) {
        hfloors.at(i) = storyHeights[i];
    }

    Wind::ExposureCategory catagory = Wind::ExposureCategory::B;
    int catIndex = expCatagory->currentIndex();
    if (catIndex == 1)
        catagory = Wind::ExposureCategory::C;
    else if (catIndex == 2)
        catagory = Wind::ExposureCategory::D;
    else if (catIndex == 3)

    double width = buildingWidth;
    double dragC = dragCoefficient->text().toDouble();
    double windGS = windGustSpeed->text().toDouble();
    double seedValue = seed->text().toDouble();

    WindForces forces = GetWindForces(catagory, windGS, dragC, buildingWidth, hfloors, seedValue);
    //std::vector<double> force1 = forces.getFloorForces(0);
    //std::vector<double> force2 = forces.getFloorForces(1);
    double wind_dt = forces.getTimeStep();
    for (int i=1; i<= numFloors; i++) {
        std::vector<double> force = forces.getFloorForces(i-1);
        Vector loadPath(force.size());
        for (int i=0; i<force.size(); i++) {
            loadPath(i)=force[i];
        }
        PathSeries *theSeries = new PathSeries(1, loadPath, wind_dt, 1.0/4448.22); // 4482 Newton to kips
        LoadPattern *theLoadPattern = new LoadPattern(i);
        theLoadPattern->setTimeSeries(theSeries);
        Vector theLoadVector(1);
        theLoadVector(0) = 1.0;
        NodalLoad *theLoad = new NodalLoad(i,i+1,theLoadVector);
        theLoadPattern->addNodalLoad(theLoad);
        theDomain.addLoadPattern(theLoadPattern);
    }

    //
    // create the analysis
    //

    AnalysisModel     *theModel = new AnalysisModel();
    CTestNormDispIncr *theTest = new CTestNormDispIncr(1.0e-10, 20, 0);
    EquiSolnAlgo      *theSolnAlgo = new NewtonRaphson();//INITIAL_TANGENT);
    TransientIntegrator  *theIntegrator = new Newmark(0.5, 0.25);
    //ConstraintHandler *theHandler = new TransformationConstraintHandler();
    ConstraintHandler *theHandler = new PlainHandler();
    RCM               *theRCM = new RCM();
    DOF_Numberer      *theNumberer = new DOF_Numberer(*theRCM);
#ifdef _FORTRAN_LIBS
    BandGenLinSolver  *theSolver = new BandGenLinLapackSolver();
    LinearSOE         *theSOE = new BandGenLinSOE(*theSolver);
#else
    ProfileSPDLinSolver  *theSolver = new ProfileSPDLinDirectSolver();
    LinearSOE         *theSOE = new ProfileSPDLinSOE(*theSolver);
#endif
    DirectIntegrationAnalysis    theAnalysis(theDomain,
                                             *theHandler,
                                             *theNumberer,
                                             *theModel,
                                             *theSolnAlgo,
                                             *theSOE,
                                             *theIntegrator);
    theSolnAlgo->setConvergenceTest(theTest);

    SymBandEigenSolver *theEigenSolver = new SymBandEigenSolver();
    EigenSOE *theEigenSOE = new SymBandEigenSOE(*theEigenSolver, *theModel);
    theAnalysis.setEigenSOE(*theEigenSOE);

    int ok = theAnalysis.eigen(numFloors,true);
    const Vector &theEig = theDomain.getEigenvalues();
    if (eigValues->Size() != numFloors) {
        eigValues->resize(numFloors);
    }
    *eigValues = theEig;
    if (ok == 0)
        for (int i=0; i<numFloors; i++)
            if (theEig(i) <= 0)
                ok = -1;

    if (ok != 0) {
        needAnalysis = false;
        analysisFailed = true;
        for (int i=0; i<numSteps; i++) {
            for (int j=0; j<numFloors+1; j++) {
                dispResponsesWind[j][i] = 0;
                if (j < numFloors) {
                    storyForceResponsesWind[j][i]=0;
                    storyDriftResponsesWind[j][i]=0;
                }
            }
        }
        maxWindDispLabel->setText(QString().setNum(0.0,'f',2));
        return;
    }

    Vector dampValues(numFloors);
    for (int i=0; i<numFloors; i++) {
        dampValues(i)=dampRatios[i];
    }
    theDomain.setModalDampingFactors(&dampValues);
    double T1 = 2*3.14159/sqrt(theEig(0));
    double maxWindDisp = 0;

    for (int i=0; i<=numSteps; i++) { // <= due to adding 0 at start
        int ok = theAnalysis.analyze(1, dt);
        if (ok != 0) {
            needAnalysis = false;
            analysisFailed = true;
            for (int k=i; k<numSteps; k++) {
                for (int j=0; j<numFloors+1; j++) {
                    dispResponsesWind[j][i] = 0;
                    if (j < numFloors) {
                        storyForceResponsesWind[j][k]=0;
                        storyDriftResponsesWind[j][k]=0;
                    }
                }

            }
            QMessageBox::warning(this, tr("Application"),
                                 tr("Transient Analysis Failed for Wind Loading"));
            break;
        }
        for (int j=0; j<numFloors+1; j++) {
            double nodeDisp = theNodes[j]->getDisp()(0);
            dispResponsesWind[j][i] = nodeDisp;
            if (fabs(nodeDisp) > maxDisp)
                maxDisp = fabs(nodeDisp);
            if (fabs(nodeDisp) > maxWindDisp)
                maxWindDisp = fabs(nodeDisp);

            if (j < numFloors) {
                LoadPattern *theLoadPattern = theDomain.getLoadPattern(j+1);
                double loadFactor = theLoadPattern->getLoadFactor();
                storyForceResponsesWind[j][i]= theElements[j]->getForce();
                storyDriftResponsesWind[j][i] = theElements[j]->getDrift();
                floorForcesWind[j][i] = loadFactor;
            }
        }
    }

    // clean up memory
    delete [] theNodes;
    delete [] theElements;

    // reset values, i.e. slider position, current displayed step...
    maxWindDispLabel->setText(QString().setNum(maxWindDisp,'f',2));

}


void MainWindow::doAnalysis()
{ 
    if (needAnalysis == true && analysisFailed == false) {

        maxDisp = 0;
        this->doEarthquakeAnalysis();
        this->doWindAnalysis();
        // currentPeriod->setText(QString().setNum(T1,'f',2));
        needAnalysis = false;
        currentStep = 0;
        //  groupTracer->setGraphKey(0);
        //      slider->setSliderPosition(0);
        myEarthquakeResult->update();

        analysisFailed = false;
        int nodeResponseFloor = theNodeResponse->getItem();
        int storyForceTime = theForceTimeResponse->getItem() -1;
        int appliedForceFloor = theAppliedForcesResponse->getItem() -1;
        //int storyForceDrift = theForceDispResponse->getItem() -1;
        nodeResponseValuesEarthquake.resize(numSteps);
        storyForceValuesEarthquake.resize(numSteps);
        storyDriftValuesEarthquake.resize(numSteps);
        nodeResponseValuesWind.resize(numSteps);
        storyForceValuesWind.resize(numSteps);
        storyDriftValuesWind.resize(numSteps);
        floorForceValueEarthquake.resize(numSteps);
        floorForceValuesWind.resize(numSteps);

        for (int i = 0; i < numSteps; ++i) {
            nodeResponseValuesEarthquake[i]=dispResponsesEarthquake[nodeResponseFloor][i];
            storyForceValuesEarthquake[i]=storyForceResponsesEarthquake[storyForceTime][i];
            storyDriftValuesEarthquake[i]=storyDriftResponsesEarthquake[storyForceTime][i];
            nodeResponseValuesWind[i]=dispResponsesWind[nodeResponseFloor][i];
            storyForceValuesWind[i]=storyForceResponsesWind[storyForceTime][i];
            storyDriftValuesWind[i]=storyDriftResponsesWind[storyForceTime][i];
            floorForceValueEarthquake[i] = i;
            floorForceValueEarthquake[i]=floorForcesEarthquake[appliedForceFloor][i];
            floorForceValuesWind[i]=floorForcesWind[appliedForceFloor][i];
        }


        theNodeResponse->setData(nodeResponseValuesEarthquake,nodeResponseValuesWind,time,numSteps,dt);
        theForceTimeResponse->setData(storyForceValuesEarthquake,storyForceValuesWind,time,numSteps,dt);
        theForceDispResponse->setData(storyForceValuesEarthquake, storyDriftValuesEarthquake,
                                      storyForceValuesWind, storyDriftValuesWind,
                                      numSteps);
        theAppliedForcesResponse->setData(floorForceValueEarthquake, floorForceValuesWind, time, numSteps, dt);
    }
}

void
MainWindow::setResponse(int floor, int mainItem)
{
    // node disp response
    if (mainItem == 0) {
        if (floor > 0 && floor <= numFloors) {
            for (int i = 0; i < numSteps; ++i) {
                nodeResponseValuesEarthquake[i]=dispResponsesEarthquake[floor][i];
                nodeResponseValuesWind[i]=dispResponsesWind[floor][i];
            }
            theNodeResponse->setData(nodeResponseValuesEarthquake,nodeResponseValuesWind, time,numSteps,dt);
        }
    }

    // changing the shear force response
    else if (mainItem == 1 || mainItem == 2) {
        if (floor < 1) floor = 1;
        if (floor > numFloors) floor = numFloors;
        if (floor > 0 && floor <= numFloors) {
            for (int i = 0; i < numSteps; ++i) {

                storyForceValuesEarthquake[i]=storyForceResponsesEarthquake[floor-1][i];
                storyDriftValuesEarthquake[i]=storyDriftResponsesEarthquake[floor-1][i];
                storyForceValuesWind[i]=storyForceResponsesWind[floor-1][i];
                storyDriftValuesWind[i]=storyDriftResponsesWind[floor-1][i];
            }
            theForceTimeResponse->setData(storyForceValuesEarthquake,storyForceValuesWind,time,numSteps,dt);
           // theForceDispResponse->setData(storyForceValuesEarthquake, storyForceValuesWind,time,numSteps,dt);
            theForceDispResponse->setData(storyForceValuesEarthquake,storyDriftValuesEarthquake,
                                          storyForceValuesWind,storyDriftValuesWind,
                                          numSteps);
            if (mainItem == 1)
                theForceDispResponse->setItem(floor);
            else
                theForceTimeResponse->setItem(floor);
        }
    }

    // changing applied story forces
    else if (mainItem == 3) {
        for (int i = 0; i < numSteps; ++i) {
            floorForceValueEarthquake[i] = floorForcesEarthquake[floor-1][i];
            floorForceValuesWind[i] = floorForcesWind[floor-1][i];
        }
        theAppliedForcesResponse->setData(floorForceValueEarthquake, floorForceValuesWind, time, numSteps, dt);
    }
}


void MainWindow::reset() {

    analysisFailed = false;
    needAnalysis = true;
    myEarthquakeResult->update();
    myWindResponseResult->update();

    // update the properties table

    theSpreadsheet->clear();
    theSpreadsheet->setColumnCount(6);
    theSpreadsheet->setRowCount(numFloors);
    theSpreadsheet->horizontalHeader()->setStretchLastSection(true);// horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    //theSpreadsheet->setFixedWidth(344);
    updatingPropertiesTable = true;
    theSpreadsheet->setHorizontalHeaderLabels(QString(" Weight ; Height ;    K    ;    Fy    ;    b    ;  zeta").split(";"));
    for (int i=0; i<numFloors; i++) {
        theSpreadsheet->setItem(i,0,new QTableWidgetItem(QString().setNum(weights[i])));
        theSpreadsheet->setItem(i,1,new QTableWidgetItem(QString().setNum(storyHeights[i])));
        theSpreadsheet->setItem(i,2,new QTableWidgetItem(QString().setNum(k[i])));
        theSpreadsheet->setItem(i,3,new QTableWidgetItem(QString().setNum(fy[i])));
        theSpreadsheet->setItem(i,4,new QTableWidgetItem(QString().setNum(b[i])));
        theSpreadsheet->setItem(i,5,new QTableWidgetItem(QString().setNum(dampRatios[i])));
    }
    theSpreadsheet->resizeRowsToContents();
    theSpreadsheet->resizeColumnsToContents();

    updatingPropertiesTable = false;

    floorSelected = -1;
    storySelected = -1;
}

void MainWindow::on_theSpreadsheet_cellChanged(int row, int column)
{
    if (updatingPropertiesTable == false) {
        QString text = theSpreadsheet->item(row,column)->text();

        bool ok;
        double textToDouble = text.toDouble(&ok);
        if (column == 0) {
            if (weights[row] == textToDouble)
                return;

            weights[row] = textToDouble;
            buildingW = 0;
            for (int i=0; i<numFloors; i++)
                buildingW += weights[i];
            inWeight->setText(QString::number(buildingW));
        } else if (column  == 1) {
            if (storyHeights[row] == textToDouble)
                return;

            storyHeights[row] = textToDouble;
            for (int i=row; i<numFloors; i++)
                floorHeights[i+1] = floorHeights[i]+storyHeights[i];
            buildingH = floorHeights[numFloors];
            inHeight->setText(QString::number(buildingH));

        } else if (column == 2) {
            if (k[row] == textToDouble)
                return;

            k[row] = textToDouble;
        } else if (column == 3) {         
            if (fy[row] == textToDouble)
                return;

            fy[row] = textToDouble;
        } else if (column == 4) {
            if (b[row] == textToDouble)
                return;

            b[row] = textToDouble;
        } else {
            if (dampRatios[row] == textToDouble)
                return;

            dampRatios[row] = textToDouble;
        }


        needAnalysis = true;
        myEarthquakeResult->update();
    }
}


void MainWindow::on_stopButton_clicked()
{
    stopRun = true;
}

void MainWindow::on_exitButton_clicked()
{
    close();
}

void MainWindow::on_runButton_clicked()
{
    stopRun = false;
    if (needAnalysis == true) {
        this->doAnalysis();
    }

    //currentStep = 0;
    do { //while (currentStep < numSteps && stopRun == false){

        slider->setSliderPosition(currentStep);
        myEarthquakeResult->repaint();
        myWindResponseResult->repaint();
        QCoreApplication::processEvents();

        currentStep++;

    } while (currentStep <= numSteps && stopRun == false); // <= added 0 to ground motion
}


void MainWindow::on_slider_valueChanged(int value)
{
    if (movingSlider == true) {
        stopRun = true;
        if (needAnalysis == true) {
            this->doAnalysis();
            myEarthquakeResult->update();
            myWindResponseResult->update();
        }
        currentStep = slider->value();

        myEarthquakeResult->repaint();
        myWindResponseResult->repaint();
    }
}

void MainWindow::on_slider_sliderPressed()
{
    movingSlider = true;
}

void MainWindow::on_slider_sliderReleased()
{
    movingSlider = false;
}

float
MainWindow::setSelectionBoundary(float y1, float y2)
{
    // determine min and max of y1,y2
    float yMin = 0;
    float yMax = 0;
    if (y1 < y2) {
        yMin = y1;
        yMax = y2;
    } else {
        yMin = y2;
        yMax = y1;
    }

    // determine min and max of nodes in [y1,y2] range
    fMinSelected = -1;
    fMaxSelected = -1;

    for (int i=0; i<=numFloors; i++) {
        if (floorHeights[i] >= yMin && floorHeights[i] <= yMax) {
            if (fMinSelected == -1)
                fMinSelected = i;
            fMaxSelected = i;
        }
    }

    // determine min and max of stories in [y1, y2] range;
    sMinSelected = -1;
    sMaxSelected = -1;
    for (int i=0; i<numFloors; i++) {
        double midStoryHeight = (floorHeights[i]+floorHeights[i+1])/2.;
        if (midStoryHeight >= yMin && midStoryHeight <= yMax) {
            if (sMinSelected == -1)
                sMinSelected = i;
            sMaxSelected = i;
        }
    }
    //   qDebug() << "sMinSelected: " << sMinSelected << " sMaxSelected: " << sMaxSelected;
    //   qDebug() << "fMinSelected: " << fMinSelected << " fMaxSelected: " << fMaxSelected;

    updatingPropertiesTable = true;

    if (fMinSelected == 0 && fMaxSelected == numFloors) {

        floorMassFrame->setVisible(false);
        storyPropertiesFrame->setVisible(false);
        spreadSheetFrame->setVisible(true);
        fMinSelected = -1;
        fMaxSelected = -1;
        sMinSelected = -1;
        sMaxSelected = -1;


    } else if (fMinSelected == fMaxSelected && fMinSelected != -1) {

        floorMassFrame->setVisible(true);
        if (sMinSelected == -1 && sMaxSelected == -1)
            storyPropertiesFrame->setVisible(false);
        else
            storyPropertiesFrame->setVisible(true);
        spreadSheetFrame->setVisible(false);
        floorSelected=-1;
        storySelected=-1;

    } else if (fMinSelected != -1 && fMaxSelected != -1) {
        floorMassFrame->setVisible(true);
        storyPropertiesFrame->setVisible(true);
        spreadSheetFrame->setVisible(false);
        floorSelected=-1;
        storySelected=-1;

    } else if (sMinSelected != -1 && sMaxSelected != -1) {
        floorMassFrame->setVisible(false);
        storyPropertiesFrame->setVisible(true);
        spreadSheetFrame->setVisible(false);
        floorSelected=-1;
        storySelected=-1;

    } else {

        floorMassFrame->setVisible(false);
        storyPropertiesFrame->setVisible(false);
        spreadSheetFrame->setVisible(true);
        floorSelected=-1;
        storySelected=-1;
    }

    updatingPropertiesTable = false;

    //
    // based on min, max nodes enable/disable lineEdits & set text
    //

    myEarthquakeResult->repaint();
    return 0;
}




void MainWindow::on_theSpreadsheet_cellClicked(int row, int column)
{
    if (column == 0) {
        floorSelected = row+1;
        storySelected = -1;
    }
    else if (column > 0 && column < 5) {
        storySelected = row;
        floorSelected = -1;
    } else {
        storySelected = -1;
        floorSelected = -1;
    }

    myEarthquakeResult->repaint();
    myWindResponseResult->repaint();
}

void MainWindow::replyFinished(QNetworkReply *pReply)
{
    return;
}

void MainWindow::on_PeriodSelectionChanged(const QString &arg1) {
    int num = periodComboBox->currentIndex();
    double eigenValue = (*eigValues)(num);
    if (eigenValue <= 0) {
     currentPeriod->setText(QString("undefined"));
    } else {
    double period = 2*3.14159/sqrt((eigenValue));
    currentPeriod->setText(QString().setNum(period,'f',2));
    }
}


void MainWindow::setData(int nStep, double deltaT, Vector *data) {

    if (numFloors == 0 || deltaT == 0. || data == 0)
        return;

    if (deltaT*nStep < 5*60)
        numSteps =  floor(5.0*60/deltaT); // nStep;
    else
        numSteps = nStep;

    //numSteps = nStep;
    excitationValues.resize(numSteps);
    dt = deltaT;

    double maxValue = 0;

    time.resize(numSteps);

    if (dispResponsesEarthquake != 0) {
        for (int j=0; j<numFloors+1; j++)
            delete [] dispResponsesEarthquake[j];
        delete [] dispResponsesEarthquake;
    }
    if (storyForceResponsesEarthquake != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] storyForceResponsesEarthquake[j];
        delete [] storyForceResponsesEarthquake;
    }
    if (storyDriftResponsesEarthquake != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] storyDriftResponsesEarthquake[j];
        delete [] storyDriftResponsesEarthquake;
    }
    if (floorForcesEarthquake != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] floorForcesEarthquake[j];
        delete [] floorForcesEarthquake;
    }

    dispResponsesEarthquake = new double *[numFloors+1];
    storyForceResponsesEarthquake = new double *[numFloors];
    storyDriftResponsesEarthquake = new double *[numFloors];
    floorForcesEarthquake = new double *[numFloors];

    for (int i=0; i<numFloors+1; i++) {
        dispResponsesEarthquake[i] = new double[numSteps+1]; // +1 as doing 0 at start
        if (i<numFloors) {
            storyForceResponsesEarthquake[i] = new double[numSteps+1];
            storyDriftResponsesEarthquake[i] = new double[numSteps+1];
            floorForcesEarthquake[i] = new double[numSteps+1];
        }
    }

    if (dispResponsesWind != 0) {
        for (int j=0; j<numFloors+1; j++)
            delete [] dispResponsesWind[j];
        delete [] dispResponsesWind;
    }
    if (storyForceResponsesWind != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] storyForceResponsesWind[j];
        delete [] storyForceResponsesWind;
    }
    if (storyDriftResponsesWind != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] storyDriftResponsesWind[j];
        delete [] storyDriftResponsesWind;
    }
    if (floorForcesWind != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] floorForcesWind[j];
        delete [] floorForcesWind;
    }

    dispResponsesWind = new double *[numFloors+1];
    storyForceResponsesWind = new double *[numFloors];
    storyDriftResponsesWind = new double *[numFloors];
    floorForcesWind = new double *[numFloors];

    for (int i=0; i<numFloors+1; i++) {
        dispResponsesWind[i] = new double[numSteps+1]; // +1 as doing 0 at start
        if (i<numFloors) {
            storyForceResponsesWind[i] = new double[numSteps+1];
            storyDriftResponsesWind[i] = new double[numSteps+1];
            floorForcesWind[i] = new double[numSteps+1];
        }
    }

    int motionDataSize = data->Size();
    motionData = data;
    for (int i = 0; i < numSteps; i++) {
        double value = 0.;
        if (i <  motionDataSize) {
           value = (*motionData)[i] * scaleFactor;
        }

        time[i]=i*dt;
        excitationValues[i]=value;
        if (fabs(value) > maxValue)
            maxValue = fabs(value);
    }

    // reset slider range
    slider->setRange(0, numSteps);

    needAnalysis = true;
  //  this->reset();
  //  myGL->update();
}


void MainWindow::on_inEarthquakeMotionSelectionChanged(const QString &arg1)
{
    std::map<QString, EarthquakeRecord *>::iterator it;
    it = records.find(arg1);
    if (it != records.end()) {
        theCurrentRecord = records.at(arg1);
        numStepEarthquake =  theCurrentRecord->numSteps;
        dtEarthquakeMotion =  theCurrentRecord->dt;
        eqData = theCurrentRecord->data;
        scaleFactor=theCurrentRecord->getScaleFactor();
        scaleFactorEQ->setText(QString::number(scaleFactor));
        this->setData(numStepEarthquake, dtEarthquakeMotion, eqData);
    } else {
        qDebug() << "ERROR: No motion found: " << arg1;
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    QApplication::quit();
}


bool MainWindow::save()
{
    if (currentFile.isEmpty()) {
        return saveAs();
    } else {
        return saveFile(currentFile);
    }
}

bool MainWindow::saveAs()
{
    //
    // get filename
    //

    QFileDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    // and save the file
    return saveFile(dialog.selectedFiles().first());
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
        loadFile(fileName);
    this->setCurrentFile(fileName);
}

void MainWindow::resetFile()
{
    // reset to original
    this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
    this->setSelectionBoundary(-1.,-1.);
    dragCoefficient->setText("1.3");
     windGustSpeed->setText("97.3");
     seed->setText("100");
    shapeSelectionBox->setCurrentIndex(0);
    expCatagory->setCurrentIndex(0);
    eqMotion->setCurrentIndex(0);

    this->on_inEarthquakeMotionSelectionChanged("ElCentro");

    // set currentFile blank
    setCurrentFile(QString());
}


void MainWindow::setCurrentFile(const QString &fileName)
{
    currentFile = fileName;
    //  setWindowModified(false);

    QString shownName = currentFile;
    if (currentFile.isEmpty())
        shownName = "untitled.json";

    setWindowFilePath(shownName);
}


void MainWindow::on_EarthquakeButtonPressed(){


}
void MainWindow::on_WindButtonPressed(){


}

void MainWindow::on_addMotion_clicked()
{

    //
    // open files
    //

    QString inputMotionName = QFileDialog::getOpenFileName(this);
    if (inputMotionName.isEmpty())
        return;

    QFile file(inputMotionName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(inputMotionName), file.errorString()));
        return;
    }

    // place contents of file into json object
    QString val;
    val=file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());

    QJsonObject jsonObject = doc.object();

    QJsonValue theValue = jsonObject["name"];
    if (theValue.isNull()) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot find \"name\" attribute in file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(inputMotionName),
                                  file.errorString()));
        return;

    }
    QString name =theValue.toString();

    // create a record and add to records map
    EarthquakeRecord *theRecord = new EarthquakeRecord();
    int ok = theRecord->inputFromJSON(jsonObject);

    if (ok == 0) {
        // inser into records list
        records.insert(std::make_pair(name, theRecord));

        // add the motion to ComboBox, & set it current
        eqMotion->addItem(name);
        int index = eqMotion->findText(name);
        eqMotion->setCurrentIndex(index);

        needAnalysis = true;
        analysisFailed = false;

    } else {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot find an attribute needed in file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(inputMotionName),
                                  file.errorString()));
    }

    // close file & return
    file.close();
    return;
}

bool MainWindow::saveFile(const QString &fileName)
{
    //
    // open file
    //

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName),
                                  file.errorString()));
        return false;
    }


    //
    // create a json object, fill it in & then use a QJsonDocument
    // to write the contents of the object to the file in JSON format
    //

    QJsonObject json;
    json["numFloors"]=numFloors;
    json["buildingHeight"]=buildingH;
    json["buildingWeight"]=buildingW;
    json["K"]=storyK;
    json["dampingRatio"]=dampingRatio;
    json["G"]=g;

    // earthquake
    json["currentMotion"]=eqMotion->currentText();
    json["currentMotionIndex"]=eqMotion->currentIndex();
    json["scaleFactorEQ"]=scaleFactorEQ->text();

    // wind
    json["expCatagoryIndex"]=expCatagory->currentIndex();
    json["squareWidth"]=squareWidth->text();
    json["shapeSelectionIndex"]=shapeSelectionBox->currentIndex();
    json["shapeSelection"]=shapeSelectionBox->currentText();

    json["windGustSpeed"]=windGustSpeed->text();
    json["seed"]=seed->text();
    json["dragCoefficient"] = dragCoefficient->text();
    json["squareHeight"] = squareHeight->text();
    json["squareWidth"] = squareWidth->text();
    json["rectangularHeight"] = rectangularHeight->text();
    json["rectangularWidth"] = rectangularWidth->text();
    json["rectangularDepth"] = rectangularDepth->text();
    json["circularHeight"] = circularHeight->text();
    json["circularDiameter"] = circularDiameter->text();

    //json["motionType"]=motionType->currentIndex();

    QJsonArray weightsArray;
    QJsonArray kArray;
    QJsonArray fyArray;
    QJsonArray bArray;
    QJsonArray heightsArray;
    QJsonArray dampArray;

    for (int i=0; i<numFloors; i++) {
        weightsArray.append(weights[i]);
        kArray.append(k[i]);
        fyArray.append(fy[i]);
        bArray.append(b[i]);
        heightsArray.append(storyHeights[i]);
        dampArray.append(dampRatios[i]);
    }


    json["floorWeights"]=weightsArray;
    json["storyK"]=kArray;
    json["storyFy"]=fyArray;
    json["storyB"]=bArray;
    json["storyHeights"]=heightsArray;
    json["dampRatios"]=dampArray;

    QJsonArray motionsArray;
    int numMotions = eqMotion->count();
    std::map<QString, EarthquakeRecord *>::iterator iter;
    for (int i=0; i<numMotions; i++) {
        QString eqName = eqMotion->itemText(i);
        iter = records.find(eqName);
        if (iter != records.end()) {
            QJsonObject obj;
            EarthquakeRecord *theRecord = iter->second ;
            theRecord->outputToJSON(obj);
            motionsArray.append(obj);
        }
    }

    json["records"]=motionsArray;

    QJsonDocument doc(json);
    file.write(doc.toJson());

    // close file
    file.close();

    // set current file
    setCurrentFile(fileName);

    return true;
}

void MainWindow::copyright()
{
    QString textCopyright = "\
        <p>\
        The source code is licensed under a BSD 2-Clause License:<p>\
        \"Copyright (c) 2017-2018, The Regents of the University of California (Regents).\"\
        All rights reserved.<p>\
        <p>\
        Redistribution and use in source and binary forms, with or without \
        modification, are permitted provided that the following conditions are met:\
        <p>\
         1. Redistributions of source code must retain the above copyright notice, this\
         list of conditions and the following disclaimer.\
         \
         \
         2. Redistributions in binary form must reproduce the above copyright notice,\
         this list of conditions and the following disclaimer in the documentation\
         and/or other materials provided with the distribution.\
         <p>\
         THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\
         ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\
         WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\
         DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR\
         ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\
         (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\
         LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\
            ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\
            (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\
            SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\
            <p>\
            The views and conclusions contained in the software and documentation are those\
            of the authors and should not be interpreted as representing official policies,\
            either expressed or implied, of the FreeBSD Project.\
            <p>\
            REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, \
            THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.\
            THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS \
            PROVIDED \"AS IS\". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,\
            UPDATES, ENHANCEMENTS, OR MODIFICATIONS.\
            <p>\
            ------------------------------------------------------------------------------------\
            <p>\
            The compiled binary form of this application is licensed under a GPL Version 3 license.\
            The licenses are as published by the Free Software Foundation and appearing in the LICENSE file\
            included in the packaging of this application. \
            <p>\
            ------------------------------------------------------------------------------------\
            <p>\
            This software makes use of the QT packages (unmodified): core, gui, widgets and network\
                                                                     <p>\
                                                                     QT is copyright \"The Qt Company Ltd&quot; and licensed under the GNU Lesser General \
                                                                     Public License (version 3) which references the GNU General Public License (version 3)\
      <p>\
      The licenses are as published by the Free Software Foundation and appearing in the LICENSE file\
      included in the packaging of this application. \
      <p>\
      ------------------------------------------------------------------------------------\
      <p>\
      This software makes use of the OpenSees Software Framework. OpenSees is copyright \"The Regents of the University of \
      California\". OpenSees is open-source software whose license can be\
      found at http://opensees.berkeley.edu.\
      <p>\
      ";


         QMessageBox msgBox;
    QSpacerItem *theSpacer = new QSpacerItem(700, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    msgBox.setText(textCopyright);
    QGridLayout *layout = (QGridLayout*)msgBox.layout();
    layout->addItem(theSpacer, layout->rowCount(),0,1,layout->columnCount());
    msgBox.exec();

}


void MainWindow::version()
{
    QMessageBox::about(this, tr("Version"),
                       tr("Version 0.1 "));
}

void MainWindow::about()
{
    QString textAbout = "\
            This is the Earthquake versus Wind (EvW) tool.  It allows the user to compare the response of\
            a building to both earthquake excitation and wind loading.  <p> \
            The building is represented by a shear building model: an idealization of a structure in which the mass \
            is lumped at the floor levels, the beams are assumed infinitely stiff in flexure and axially inextensible,\
            and the columns are axially inextensible.  The user inputs the floor weights and story properties (stiffness, \
            yield strength, hardening ratio) of the stories, and a damping ratio for the structure. Individual floor and \
            story values are possible by user edisting in the spreadsheet area or alternativily selecting an an appropriate\
            area in the graphic around area of interest.In addition nonlinear effects due to P-Delta can be studied.\
            <p>\
            All units are in sec, kips, inches.\
            <p>\
            For the arthquake loading, the equations of motions are set up using the uniform excitation approach, \
            i.e. MA + CV + KU = -MAg where Ag is the ground acceleration.\
            Additional motions can be added by user. The units for these additional motions must be in g. An\
            example is provided at https://github.com/NHERI-SimCenter/MDOF/blob/master/example/elCentro.json\
            <p>\
            For the Wind loading, the equations of motions are expressed as MA + CV + KU = F where\
            F is the dynamic wind force caused by the fluctuating winds. The Kaimal wind spectrum\
            and the Davenport coherence function are employed to describe correlated wind fields in\
            the frequency domain. With these models, fluctuating wind velocities at each floor level are\
            simulated in terms of the discrete frequency function scheme (Wittig and Sinha 1975),\
            which is adopted due to its simplicity and fast computation. The dynamic wind forces\
            are then determined by the simulated fluctuating winds.\
            <p>\
            The dynamic set of equations for each of the applied loading conditions are solved using the Newmark\
            constant acceleration method and Newton-Raphson solution algorithm. As a consequence, the analysis \
            will not always converge. Should convergence issues arise,a pop-up window will be displayed indicating\
            such circumstances to the user. The user needs to close pop-up and change the settings, e.g. turn off \
            P-Delta.\
            ";

            QMessageBox msgBox;
    QSpacerItem *theSpacer = new QSpacerItem(500, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    msgBox.setText(textAbout);
    QGridLayout *layout = (QGridLayout*)msgBox.layout();
    layout->addItem(theSpacer, layout->rowCount(),0,1,layout->columnCount());
    msgBox.exec();
}

void MainWindow::submitFeedback()
{
  QDesktopServices::openUrl(QUrl("https://github.com/NHERI-SimCenter/EVW/issues", QUrl::TolerantMode));
 // QDesktopServices::openUrl(QUrl("https://www.designsafe-ci.org/help/new-ticket/", QUrl::TolerantMode));
    }



void MainWindow::loadFile(const QString &fileName)
{

    //
    // open files

    //

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    // place contents of file into json object
    QString val;
    val=file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    if (doc.isNull() || doc.isEmpty()) {
        errorMessage("Error Loading File: Not a JSON file or empty");
        return;

    }
    QJsonObject jsonObject = doc.object();


  //
  // clean up old
  //

    if (dispResponsesEarthquake != 0) {
        for (int j=0; j<numFloors+1; j++)
            delete [] dispResponsesEarthquake[j];
        delete [] dispResponsesEarthquake;
    }
    if (storyForceResponsesEarthquake != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] storyForceResponsesEarthquake[j];
        delete [] storyForceResponsesEarthquake;
    }
    if (storyDriftResponsesEarthquake != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] storyDriftResponsesEarthquake[j];
        delete [] storyDriftResponsesEarthquake;
    }

    if (floorForcesEarthquake != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] floorForcesEarthquake[j];
        delete [] floorForcesEarthquake;
    }

    if (dispResponsesWind != 0) {
        for (int j=0; j<numFloors+1; j++)
            delete [] dispResponsesWind[j];
        delete [] dispResponsesWind;
    }
    if (storyForceResponsesWind != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] storyForceResponsesWind[j];
        delete [] storyForceResponsesWind;
    }
    if (storyDriftResponsesWind != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] storyDriftResponsesWind[j];
        delete [] storyDriftResponsesWind;
    }
    if (floorForcesWind != 0) {
        for (int j=0; j<numFloors; j++)
            delete [] floorForcesWind[j];
        delete [] floorForcesWind;
    }

    //

    dispResponsesEarthquake = 0;
    dispResponsesWind = 0;
    storyDriftResponsesEarthquake = 0;
    storyDriftResponsesWind = 0;
    storyForceResponsesEarthquake = 0;
    storyForceResponsesWind = 0;
    floorForcesEarthquake = 0;
    floorForcesWind = 0;


    if (weights != 0)
        delete [] weights;
    if (k != 0)
        delete [] k;
    if (fy != 0)
        delete [] fy;
    if (b != 0)
        delete [] b;
    if (floorHeights != 0)
        delete [] floorHeights;
    if (storyHeights != 0)
        delete [] storyHeights;
    if (dampRatios != 0)
        delete [] dampRatios;



    QJsonValue theValue = jsonObject["numFloors"];
    if (theValue.isUndefined()) {
        errorMessage("No \"numFloors\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
    }
    numFloors=theValue.toInt();
    inFloors->setText(QString::number(numFloors));

    theValue = jsonObject["buildingHeight"];
    if (theValue.isUndefined()) {
        errorMessage("No \"buildingHeight\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    buildingH=theValue.toDouble();
    inHeight->setText(QString::number(buildingH));

    theValue = jsonObject["buildingWeight"];
    if (theValue.isUndefined()) {
        errorMessage("No \"buildingWeight\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    buildingW=theValue.toDouble();
    inWeight->setText(QString::number(buildingW));

    theValue = jsonObject["K"];
    if (theValue.isUndefined()) {
        errorMessage("No \"K\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    storyK=theValue.toDouble();
    inK->setText(QString::number(storyK));

    theValue = jsonObject["G"];
    if (theValue.isUndefined()) {
        errorMessage("No \"G\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    g=theValue.toDouble();
    //inGravity->setText(QString::number(g));

    theValue = jsonObject["dampingRatio"];
    if (theValue.isUndefined()) {
        errorMessage("No \"dampingRatio\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    dampingRatio=theValue.toDouble();
    inDamping->setText(QString::number(dampingRatio));

    weights = new double[numFloors];
    k = new double[numFloors];
    fy = new double[numFloors];
    b = new double[numFloors];
    floorHeights = new double[numFloors+1];
    storyHeights = new double[numFloors];
    dampRatios = new double[numFloors];

    QJsonArray theArray;

    theValue = jsonObject["floorWeights"];
    if (theValue.isUndefined()) {
        errorMessage("No \"floorWeights\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    theArray=theValue.toArray();

    for (int i=0; i<numFloors; i++)
        weights[i] = theArray.at(i).toDouble();

    theValue = jsonObject["storyK"];
    if (theValue.isUndefined()) {
        errorMessage("No \"storyK\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    theArray=theValue.toArray();

    for (int i=0; i<numFloors; i++)
        k[i] = theArray.at(i).toDouble();

    theValue = jsonObject["storyFy"];
    if (theValue.isUndefined()) {
        errorMessage("No \"storyFy\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    theArray=theValue.toArray();

    for (int i=0; i<numFloors; i++)
        fy[i] = theArray.at(i).toDouble();

    theValue = jsonObject["storyB"];
    if (theValue.isUndefined()) {
        errorMessage("No \"storyB\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    theArray=theValue.toArray();

    for (int i=0; i<numFloors; i++)
        b[i] = theArray.at(i).toDouble();

    theValue = jsonObject["storyHeights"];
    if (theValue.isUndefined()) {
        errorMessage("No \"storyHeights\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    theArray=theValue.toArray();

    floorHeights[0] = 0;
    for (int i=0; i<numFloors; i++) {
        storyHeights[i] = theArray.at(i).toDouble();
        floorHeights[i+1] = floorHeights[i] + storyHeights[i];
    }

    theValue = jsonObject["dampRatios"];
    if (theValue.isUndefined()) {
        errorMessage("No \"dampRatios\" section");
        this->setBasicModel(5, 5*100, 5*144, 31.54, .05, 386.4, 5*144, 5*144);
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
        return;
    }
    theArray=theValue.toArray();

    for (int i=0; i<numFloors; i++)
        dampRatios[i] = theArray.at(i).toDouble();


    dispResponsesEarthquake = new double *[numFloors+1];
    for (int i=0; i<numFloors+1; i++) {
        dispResponsesEarthquake[i] = new double[numSteps+1]; // +1 as doing 0 at start
    }
    dispResponsesWind = new double *[numFloors+1];
    for (int i=0; i<numFloors+1; i++) {
        dispResponsesWind[i] = new double[numSteps+1]; // +1 as doing 0 at start
    }

    //
    // clear records and inMotion combo box
    //

    std::map<QString, EarthquakeRecord *>::iterator iter;

    // delete the earthqkaes before clear as clear does not invoke destructor if pointers
    for (iter = records.begin(); iter != records.end(); ++iter ) {
        delete iter->second;
    }

    records.clear();
    eqMotion->clear();

    theValue = jsonObject["records"];
    if (theValue.isUndefined()) {
        errorMessage("No \"records\" section");
        this->on_inEarthquakeMotionSelectionChanged("ElCentro");
        return;
    }
    theArray=theValue.toArray();

    //
    // now read records from file and populate records and comoboBox with new
    //

    for (int i=0; i<theArray.size(); i++) {
        EarthquakeRecord *theRecord = new EarthquakeRecord();
        QJsonObject theEarthquakeObj = theArray.at(i).toObject();
        theRecord->inputFromJSON(theEarthquakeObj);
        records.insert(std::make_pair(theRecord->name, theRecord));
        eqMotion->addItem(theRecord->name);
        qDebug() << "Loaded Record: " << theRecord->name;
    }

    //
    // set current record to what was saved in file
    //

    theValue = jsonObject["currentMotionIndex"];
    if (theValue.isUndefined()) {
        errorMessage("No \"currentMotionIndex\" section");
        return;
    }
    int currIndex = theValue.toInt();
    eqMotion->setCurrentIndex(theValue.toInt());
    theValue = jsonObject["currentMotion"];
    if (theValue.isUndefined()) {
        errorMessage("No \"currentMotion\" section");
        return;
    }
    QString currentMotionName = theValue.toString();
   // qDebug() << "INDEX & NAME: " << currIndex << " " << currentMotionName;

    theValue = jsonObject["scaleFactorEQ"];//=scaleFactorEQ->currentText();
    scaleFactorEQ->setText(theValue.toString());

    // wind

    theValue = jsonObject["expCatagoryIndex"];
    if (theValue.isUndefined()) {
        errorMessage("No \"expCatagoryIndex\" section");
        return;
    }
    expCatagory->setCurrentIndex(theValue.toInt());
    theValue = jsonObject["shapeSelectionIndex"];
    if (theValue.isUndefined()) {
        errorMessage("No \"shapeSelectionIndex\" section");
        return;
    }
    shapeSelectionBox->setCurrentIndex(theValue.toInt());

    theValue = jsonObject["windGustSpeed"];
    if (theValue.isUndefined()) {
        errorMessage("No \"windGustSpeed\" section");
        return;
    }
    windGustSpeed->setText(theValue.toString());
    theValue = jsonObject["seed"];
    if (theValue.isUndefined()) {
        errorMessage("No \"seeds\" section");
        return;
    }
    seed->setText(theValue.toString());
    theValue = jsonObject["dragCoefficient"];
    dragCoefficient->setText(theValue.toString());
    theValue = jsonObject["squareHeight"];
    squareHeight->setText(theValue.toString());
    theValue = jsonObject["squareWidth"];
    squareWidth->setText(theValue.toString());
    theValue = jsonObject["rectangularHeight"];
    rectangularHeight->setText(theValue.toString());
    theValue = jsonObject["rectangularWidth"];
    rectangularWidth->setText(theValue.toString());
    theValue = jsonObject["rectangularDepth"];
    rectangularDepth->setText(theValue.toString());
    theValue = jsonObject["circularHeight"];
    circularHeight->setText(theValue.toString());
    theValue = jsonObject["circularDiameter"];
    circularDiameter->setText(theValue.toString());

    this->reset();
    this->on_inEarthquakeMotionSelectionChanged(currentMotionName);

    theNodeResponse->setItem(numFloors);
    theForceDispResponse->setItem(1);
    theForceTimeResponse->setItem(1);
    theAppliedForcesResponse->setItem(1);

    // close file
    file.close();

    // given the json object, create the C++ objects
    // inputWidget->inputFromJSON(jsonObj);
    //setCurrentFile(fileName);
}


void MainWindow::createActions() {

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    //const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
    //const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));

    //QToolBar *fileToolBar = addToolBar(tr("File"));

    QAction *newAction = new QAction(tr("&Reset"), this);
    newAction->setShortcuts(QKeySequence::New);
    newAction->setStatusTip(tr("Create a new file"));
    connect(newAction, &QAction::triggered, this, &MainWindow::resetFile);
    fileMenu->addAction(newAction);
    //fileToolBar->addAction(newAction);


    QAction *openAction = new QAction(tr("&Open"), this);
    openAction->setShortcuts(QKeySequence::Open);
    openAction->setStatusTip(tr("Open an existing file"));
    connect(openAction, &QAction::triggered, this, &MainWindow::open);
    fileMenu->addAction(openAction);
    //fileToolBar->addAction(openAction);

    QAction *saveAction = new QAction(tr("&Save"), this);
    saveAction->setShortcuts(QKeySequence::Save);
    saveAction->setStatusTip(tr("Save the document to disk"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::save);
    fileMenu->addAction(saveAction);

    QAction *saveAsAction = new QAction(tr("&Save As"), this);
    saveAction->setStatusTip(tr("Save the document with new filename to disk"));
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveAs);
    fileMenu->addAction(saveAsAction);

    // strangely, this does not appear in menu (at least on a mac)!! ..
    // does Qt not allow as in tool menu by default?
    // check for yourself by changing Quit to drivel and it works
    QAction *exitAction = new QAction(tr("&Quit"), this);
    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    // exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Exit the application"));
    fileMenu->addAction(exitAction);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    QString tLabel("Time (sec)");
    QString dLabel("Relative Displacement (in)");
    //QString dLabel("Total Displacement (in)");
    QString fLabel("Floor");
    QString sLabel("Story");
    QString shearForceLabel("Shear Force (kip)");
    QString appliedForceLabel("Applied Force (kip)");
    QString dispLabel("Displacement (inch)");

    theNodeResponse = new ResponseWidget(this, 0, fLabel, tLabel, dLabel);
    QDockWidget *nodeResponseDock = new QDockWidget(tr("Floor Displacement History"), this);
    nodeResponseDock->setWidget(theNodeResponse);
    nodeResponseDock->setAllowedAreas(Qt::NoDockWidgetArea);
    nodeResponseDock->setFloating(true);
    nodeResponseDock->close();
    viewMenu->addAction(nodeResponseDock->toggleViewAction());

    theForceTimeResponse = new ResponseWidget(this, 1, sLabel, tLabel, shearForceLabel);
    QDockWidget *forceTimeResponseDock = new QDockWidget(tr("Story Force History"), this);
    forceTimeResponseDock->setWidget(theForceTimeResponse);
    forceTimeResponseDock->setAllowedAreas(Qt::NoDockWidgetArea);
    forceTimeResponseDock->setFloating(true);
    forceTimeResponseDock->close();
    viewMenu->addAction(forceTimeResponseDock->toggleViewAction());


    theForceDispResponse = new ResponseWidget(this, 2, sLabel, dispLabel,shearForceLabel, false);
    QDockWidget *forceDriftResponseDock = new QDockWidget(tr("Story Force-Displacement"), this);
    forceDriftResponseDock->setWidget(theForceDispResponse);
    forceDriftResponseDock->setAllowedAreas(Qt::NoDockWidgetArea);
    forceDriftResponseDock->setFloating(true);
    forceDriftResponseDock->close();
    viewMenu->addAction(forceDriftResponseDock->toggleViewAction());

    theAppliedForcesResponse = new ResponseWidget(this, 3, fLabel, tLabel,appliedForceLabel, true, false);
    QDockWidget *appliedForcesResponseDock = new QDockWidget(tr("Applied Floor Forces"), this);
    appliedForcesResponseDock->setWidget(theAppliedForcesResponse);
    appliedForcesResponseDock->setAllowedAreas(Qt::NoDockWidgetArea);
    appliedForcesResponseDock->setFloating(true);
    appliedForcesResponseDock->close();
    viewMenu->addAction(appliedForcesResponseDock->toggleViewAction());

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *infoAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    QAction *submitAct = helpMenu->addAction(tr("&Provide Feedback"), this, &MainWindow::submitFeedback);
    //aboutAct->setStatusTip(tr("Show the application's About box"));
    QAction *aboutAct = helpMenu->addAction(tr("&Version"), this, &MainWindow::version);
    //aboutAct->setStatusTip(tr("Show the application's About box"));

    QAction *copyrightAct = helpMenu->addAction(tr("&License"), this, &MainWindow::copyright);
    //aboutAct->setStatusTip(tr("Show the application's About box"));
}

void MainWindow::on_shapeChange(int rowSelected) {
    if (rowSelected == 0)
        dragCoefficient->setText("1.3");
    else if (rowSelected == 1)
        dragCoefficient->setText("1.3");
    else if (rowSelected == 2)
        dragCoefficient->setText("0.8");


    shapesWidget->setCurrentIndex(rowSelected);
}

void MainWindow::on_buildingWidthChanged(double newWidth)
{
    if (newWidth != buildingWidth) {
        buildingWidth = newWidth;
        this->updatePeriod();
        needAnalysis = true;
        this->reset();
    }
}

void MainWindow::viewNodeResponse(){

}

void MainWindow::viewStoryResponse(){

}

void MainWindow::createHeaderBox() {

    HeaderWidget *header = new HeaderWidget();
    header->setHeadingText(tr("Earthquake Versus Wind"));

    largeLayout->addWidget(header);
}

void MainWindow::createFooterBox() {

    FooterWidget *footer = new FooterWidget();

    largeLayout->addWidget(footer);
}

void MainWindow::createInputPanel() {

  //
   // create input layout, to fill in wih widgets as we go hrough this method
  //

    inputLayout = new QVBoxLayout;

    //
    // some QStrings to avoid duplication of units
    //

    QString blank(tr("   "));
    QString kips(tr("k"  ));
    QString g(tr("g"  ));
    QString kipsInch(tr("k/in"));
    QString inch(tr("in  "));
    QString sec(tr("sec"));
    QString percent(tr("\%   "));

    //
    // first create a section header for input forces & add to layout
    //

    SectionTitle *inForces = new SectionTitle(this);
    inForces->setTitle(tr("Input Forces"));
    inputLayout->addWidget(inForces);

    //
    // now into a frame we are going to create and then place input widgets side-by-side for:
    //   - 1. earthquake forces
    //   - 2. wind forces
    // 

    QFrame *forcesInput = new QFrame();
    QHBoxLayout *forcesInputLayout = new QHBoxLayout();

    //
    // first the earthquake forces options into QGroupbox widget
    // - input motion from pull down, scale factor & ability to add more motions to pull down
    //

    QGroupBox *earthquakBox = new QGroupBox("Earthquake");
    QGridLayout *earthquakeForcesLayout = new QGridLayout;

    // input motion

    QLabel *entryLabel = new QLabel();
    entryLabel->setText(tr("Input Motion"));
    earthquakeForcesLayout->addWidget(entryLabel,0,0);

    eqMotion = new QComboBox();
    earthquakeForcesLayout->addWidget(eqMotion,0,1);

    // scale factor

    QLabel *scaleLabel = new QLabel(tr("Scale Factor"));
    earthquakeForcesLayout->addWidget(scaleLabel,1,0);
    scaleFactorEQ = new QLineEdit();
    earthquakeForcesLayout->addWidget(scaleFactorEQ,1,1);

    // button to add more

    addMotion = new QPushButton("Add");
    earthquakeForcesLayout->addWidget(addMotion,2,1);
    earthquakBox->setLayout(earthquakeForcesLayout);

    //
    // now wind forcing function options in another QGroupBox widgets
    //  - exposure category, gust speed and choice on force generator
    //

    // exposure catagory
    QGroupBox *windBox = new QGroupBox("Wind");
    QGridLayout *windBoxLayout = new QGridLayout;

    QLabel *categoryLabel = new QLabel();
    categoryLabel->setText(tr("ASCE 7 Exposure Category"));
    windBoxLayout->addWidget(categoryLabel,0,0);

    expCatagory = new QComboBox();
   // expCatagory->addItem("A");
    expCatagory->addItem("B");
    expCatagory->addItem("C");
    expCatagory->addItem("D");
    windBoxLayout->addWidget(expCatagory,0,1);

    // gust speed
    QLabel *gustLabel = new QLabel();
    gustLabel->setText(tr("Gust Wind Speed (mph)"));
    windBoxLayout->addWidget(gustLabel,1,0);

    windGustSpeed = new QLineEdit();
    windGustSpeed->setText("97.3");
    windBoxLayout->addWidget(windGustSpeed,1,1);

    /*
    QLabel *mphLabel = new QLabel();
    mphLabel->setText(tr("mph"));
     windBoxLayout->addWidget(mphLabel,1,2);
*/
    /*
    QLabel *schemeLabel = new QLabel();
    schemeLabel->setText(tr("Simulation Scheme"));
    windBoxLayout->addWidget(schemeLabel,2,0);

    QComboBox *simScheme = new QComboBox();
    simScheme->addItem("Discrete Frequency");
    simScheme->addItem("Schur Decomposition");
    simScheme->addItem("Ergodic Spectral");
    simScheme->addItem("Conventional Spectral");
    windBoxLayout->addWidget(simScheme,2,1);
*/
    // gust speed
    QLabel *seedLabel = new QLabel();
    seedLabel->setText(tr("seed"));
    windBoxLayout->addWidget(seedLabel,2,0);

    seed = new QLineEdit();
    seed->setText("100");
    windBoxLayout->addWidget(seed,2,1);

    windBox->setLayout(windBoxLayout);

    // add earthquake & wind options to the layout, set forcesInput widgets layout & add to inputs layout
    forcesInputLayout->addWidget(earthquakBox);
    forcesInputLayout->addWidget(windBox);

    forcesInput->setLayout(forcesInputLayout);
    inputLayout->addWidget(forcesInput);

    //
    // Now create a section for building propertes
    //

    SectionTitle *inBuilding = new SectionTitle(this);
    inBuilding->setTitle(tr("Buiding Properties"));
    inputLayout->addWidget(inBuilding);

    //
    // create frame to hold major building property inputs. into that frame we will place:
    //  - numFloors
    //  - building Weight
    //  - building shape (this is a selection combo box), shape displayed depends on selection
    //     -QStackedWidget containing inputs for standard shapes, which displayed depends on selection
    //  - story stiffnesses
    //  - modal damping ratios
    //  - a checkbox for pDelta analysis
    //

    // create the frame & layout for property inputs

    QFrame *mainProperties = new QFrame();
    mainProperties->setObjectName(QString::fromUtf8("mainProperties")); //styleSheet
    QVBoxLayout *mainPropertiesLayout = new QVBoxLayout();

    // add num floors and buiding weight

    inFloors = createTextEntry(tr("Number Floors"), mainPropertiesLayout, 100, 100, &blank);
    inWeight = createTextEntry(tr("Building Weight"), mainPropertiesLayout, 100, 100, &kips);

    // now add a layout for building shape selection

    QHBoxLayout *shapeLayout = new QHBoxLayout();
    QLabel *shapeLabel = new QLabel();
    shapeLabel->setText("Shape");
    shapeSelectionBox = new QComboBox();
    shapeSelectionBox->addItem("Square");
    shapeSelectionBox->addItem("Rectangular");
    shapeSelectionBox->addItem("Circular");

    shapeSelectionBox->setMinimumWidth(100);
    shapeSelectionBox->setMaximumWidth(100);

    shapeLayout->addWidget(shapeLabel);
    shapeLayout->addWidget(shapeSelectionBox);
    shapeLayout->addStretch();

    shapeLayout->setSpacing(10);
    shapeLayout->setMargin(0);

    mainPropertiesLayout->addLayout(shapeLayout);

    // create a QStackedWidget to hold widgets for different shape selections

    shapesWidget = new QStackedWidget;

    //
    // create frame widget for square shapes and add to stacked widget
    //

    QFrame *squareWidget = new QFrame();
    QHBoxLayout *squareLayout= new QHBoxLayout();

    squareHeight = new QLineEdit();
    squareWidth = new QLineEdit();

    squareHeight->setMaximumWidth(100);
    squareWidth->setMaximumWidth(100);
    QLabel *squareHeightLabel = new QLabel("Height");
    squareHeightLabel->setStyleSheet("border: 0px solid white; border-radius: 2px");
    QLabel *squareWidthLabel = new QLabel("Width");
    squareWidthLabel->setStyleSheet("border: 0px solid white; border-radius: 2px");
    QLabel *squareUnit = new QLabel();
    squareUnit->setText(inch);
    squareUnit->setStyleSheet("border: 0px solid white; border-radius: 2px");
    squareLayout->addStretch();
    squareLayout->addWidget(squareHeightLabel);
    squareLayout->addWidget(squareHeight);
    squareLayout->addWidget(squareUnit);
    squareLayout->addStretch();

    squareLayout->addWidget(squareWidthLabel);
    squareLayout->addWidget(squareWidth);
    squareUnit = new QLabel();
    squareUnit->setText(inch);
    squareUnit->setStyleSheet("border: 0px solid white; border-radius: 2px");
    squareLayout->addWidget(squareUnit);

    squareLayout->addStretch();
    squareWidget->setLayout(squareLayout);
    squareWidget->setLineWidth(2);

    // add square frame to QStackedWidget
    shapesWidget->addWidget(squareWidget);

    // set default inHeight (so as not to break code .. remove later!)
    inHeight = squareHeight;

    //
    // create frame widget for rectangular shapes and add to stacked widget
    //

    QFrame *rectangularWidget = new QFrame();
    QHBoxLayout *rectangularLayout= new QHBoxLayout();

    rectangularHeight = new QLineEdit();
    rectangularWidth = new QLineEdit();
    rectangularDepth = new QLineEdit();


    rectangularHeight->setMaximumWidth(100);
    rectangularWidth->setMaximumWidth(100);
    rectangularDepth->setMaximumWidth(100);

    QLabel *rectangularHeightLabel = new QLabel("Height");
    rectangularHeightLabel->setStyleSheet("border: 0px solid white; border-radius: 2px");

    QLabel *rectangularWidthLabel = new QLabel("Width");
    rectangularWidthLabel->setStyleSheet("border: 0px solid white; border-radius: 2px");
    QLabel *rectangularDepthLabel = new QLabel("Depth");
    rectangularDepthLabel->setStyleSheet("border: 0px solid white; border-radius: 2px");

    QLabel *rectangularUnit = new QLabel();
    rectangularUnit->setText(inch);
    rectangularUnit->setStyleSheet("border: 0px solid white; border-radius: 2px");
    rectangularLayout->addStretch();
    rectangularLayout->addWidget(rectangularHeightLabel);
    rectangularLayout->addWidget(rectangularHeight);
    rectangularLayout->addWidget(rectangularUnit);
    rectangularLayout->addStretch();

    //rectangularLayout->addStretch();
    rectangularLayout->addWidget(rectangularWidthLabel);
    rectangularLayout->addWidget(rectangularWidth);
    rectangularUnit = new QLabel();
    rectangularUnit->setText(inch);
    rectangularUnit->setStyleSheet("border: 0px solid white; border-radius: 2px");
    rectangularLayout->addWidget(rectangularUnit);
    rectangularLayout->addStretch();

    rectangularLayout->addWidget(rectangularDepthLabel);
    rectangularLayout->addWidget(rectangularDepth);
    rectangularUnit = new QLabel();
    rectangularUnit->setText(inch);
    rectangularUnit->setStyleSheet("border: 0px solid white; border-radius: 2px");
    rectangularLayout->addWidget(rectangularUnit);

    rectangularLayout->addStretch();
    rectangularWidget->setLayout(rectangularLayout);
    rectangularWidget->setLineWidth(2);

    // add rectaangular frame to QStackedWidget
    shapesWidget->addWidget(rectangularWidget);

    //
    // finally frame for circular shapes and add to stacked widgets
    //

    QFrame *circularWidget = new QFrame();
    QHBoxLayout *circularLayout= new QHBoxLayout();
    circularHeight = new QLineEdit();
    circularHeight->setMaximumWidth(100);
    circularDiameter = new QLineEdit();
    circularDiameter->setMaximumWidth(100);
    QLabel *circularHeightLabel = new QLabel("Height");
    QLabel *circularDiameterLabel = new QLabel("Diameter");
    QLabel *circularUnit = new QLabel();
    circularUnit->setText(inch);
    circularLayout->addStretch();
    circularLayout->addWidget(circularHeightLabel);
    circularLayout->addWidget(circularHeight);
    circularLayout->addWidget(circularUnit);
    //squareLayout->addStretch();
    circularLayout->addWidget(circularDiameterLabel);
    circularLayout->addWidget(circularDiameter);
    circularUnit = new QLabel();
    circularUnit->setStyleSheet("border: 0px solid white; border-radius: 2px");
    circularUnit->setText(inch);
    circularLayout->addWidget(circularUnit);

    circularLayout->addStretch();
    circularWidget->setLayout(circularLayout);
    circularWidget->setLineWidth(2);
    shapesWidget->setStyleSheet("border: 1px solid #E0E0E0; border-radius: 2px");

    // add frame to QStackedWidget
    shapesWidget->addWidget(circularWidget);

    // connect shape selection changed and which widget to show
    connect(shapeSelectionBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_shapeChange(int)));

    //
    // add the stacked widget to layout
    //

    mainPropertiesLayout->addWidget(shapesWidget);

   //
   // finally to finish off building properties we add the rest of building properties
   //   - drag coefficient, story stiffness, modal damping ratio and a checkbox for inclusion of PDelta
   //
    dragCoefficient = createTextEntry(tr("Drag Coefficient"), mainPropertiesLayout, 100, 100, &blank);
    dragCoefficient->setText("1.3");

    inK = createTextEntry(tr("Story Stiffness"), mainPropertiesLayout, 100, 100, &kipsInch);
    inDamping = createTextEntry(tr("Damping Ratio"), mainPropertiesLayout, 100, 100, &blank);
    pDeltaBox = new QCheckBox(tr("Include PDelta"), 0);
    pDeltaBox->setCheckState(Qt::Checked);

    mainPropertiesLayout->addWidget(pDeltaBox);
    mainProperties->setLayout(mainPropertiesLayout);

    // after setting some style options we add properties fame to input layout
    mainProperties->setLayout(mainPropertiesLayout);
    mainProperties->setFrameStyle(QFrame::Raised);
    mainProperties->setLineWidth(1);
    mainProperties->setFrameShape(QFrame::Box);
    inputLayout->addWidget(mainProperties);


    //
    // create frames to hold story and floor selections, these made visible on graph selection
    //  - presently an undoumented feature
    //

    floorMassFrame = new QFrame();
    floorMassFrame->setObjectName(QString::fromUtf8("floorMassFrame")); //styleSheet
    QVBoxLayout *floorMassFrameLayout = new QVBoxLayout();
    inFloorWeight = createTextEntry(tr("Floor Weight"), floorMassFrameLayout);
    floorMassFrame->setLayout(floorMassFrameLayout);
    floorMassFrame->setLineWidth(1);
    floorMassFrame->setFrameShape(QFrame::Box);
    inputLayout->addWidget(floorMassFrame);
    floorMassFrame->setVisible(false);

    storyPropertiesFrame = new QFrame();
    storyPropertiesFrame->setObjectName(QString::fromUtf8("storyPropertiesFrame"));
    QVBoxLayout *storyPropertiesFrameLayout = new QVBoxLayout();
    inStoryHeight = createTextEntry(tr("Story Height"), storyPropertiesFrameLayout);
    inStoryK = createTextEntry(tr("Stiffness"), storyPropertiesFrameLayout);
    inStoryFy = createTextEntry(tr("Yield Strength"), storyPropertiesFrameLayout);
    inStoryB = createTextEntry(tr("Hardening Ratio"), storyPropertiesFrameLayout);
    storyPropertiesFrame->setLayout(storyPropertiesFrameLayout);
    storyPropertiesFrame->setLineWidth(1);
    storyPropertiesFrame->setFrameShape(QFrame::Box);
    inputLayout->addWidget(storyPropertiesFrame);
    storyPropertiesFrame->setVisible(false);

    spreadSheetFrame = new QFrame();
    QVBoxLayout *spreadsheetFrameLayout = new QVBoxLayout();
    headings << tr("Weight") << tr("Heighht") << tr("K") << tr("Fy") << tr("b") << tr("zeta");
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;

    theSpreadsheet = new SimpleSpreadsheetWidget(6, 4, headings, dataTypes,0);
    spreadsheetFrameLayout->addWidget(theSpreadsheet, 1.0);
    spreadSheetFrame->setObjectName(QString::fromUtf8("inputMotion")); //styleSheet
    spreadSheetFrame->setLayout(spreadsheetFrameLayout);
    spreadSheetFrame->setLineWidth(1);
    spreadSheetFrame->setFrameShape(QFrame::Box);

    inputLayout->addWidget(spreadSheetFrame,1);
    spreadSheetFrame->setVisible(false);

    inputLayout->addStretch();

    //
    // finally create a frame to hold the push buttons:
    //   -  run, stop and Exit
    //

    QFrame *pushButtons = new QFrame();
    pushButtons->setObjectName(QString::fromUtf8("pushButtons")); //styleSheet
    QHBoxLayout *pushButtonsLayout = new QHBoxLayout();
    runButton = new QPushButton("Run");
    pushButtonsLayout->addWidget(runButton);
    stopButton = new QPushButton("Stop");
    pushButtonsLayout->addWidget(stopButton);
    exitButton = new QPushButton("Exit");
    pushButtonsLayout->addWidget(exitButton);
    pushButtons->setLayout(pushButtonsLayout);
    pushButtons->setLineWidth(1);
    pushButtons->setFrameShape(QFrame::Box);

    inputLayout->addWidget(pushButtons);

    mainLayout->addLayout(inputLayout);

    //
    // set validators for QLineEdits
    //

    scaleFactorEQ->setValidator(new QDoubleValidator);

    inFloors->setValidator(new QIntValidator);
    inWeight->setValidator(new QDoubleValidator);
    inHeight->setValidator(new QDoubleValidator);
    inK->setValidator(new QDoubleValidator);
    inDamping->setValidator(new QDoubleValidator);
    inFloorWeight->setValidator(new QDoubleValidator);
    inStoryB->setValidator(new QDoubleValidator);
    inStoryFy->setValidator(new QDoubleValidator);
    inStoryHeight->setValidator(new QDoubleValidator);
    inStoryK->setValidator(new QDoubleValidator);
    squareHeight->setValidator(new QDoubleValidator);
    squareWidth->setValidator(new QDoubleValidator);
    circularHeight->setValidator(new QDoubleValidator);
    circularDiameter->setValidator(new QDoubleValidator);
    rectangularHeight->setValidator(new QDoubleValidator);
    rectangularWidth->setValidator(new QDoubleValidator);
    rectangularDepth->setValidator(new QDoubleValidator);
    dragCoefficient->setValidator(new QDoubleValidator);
    windGustSpeed->setValidator(new QDoubleValidator);
    seed->setValidator(new QDoubleValidator);

    //
    // connect signals & slots
    //

    connect(scaleFactorEQ,SIGNAL(editingFinished()),this,SLOT(on_scaleFactor_editingFinished()));
    connect(pDeltaBox, SIGNAL(stateChanged(int)), this, SLOT(on_includePDeltaChanged(int)));
    connect(addMotion,SIGNAL(clicked()), this, SLOT(on_addMotion_clicked()));
    connect(inFloors,SIGNAL(editingFinished()), this, SLOT(on_inFloors_editingFinished()));
    connect(inWeight,SIGNAL(editingFinished()), this, SLOT(on_inWeight_editingFinished()));
    connect(inHeight,SIGNAL(editingFinished()), this, SLOT(on_inHeight_editingFinished()));

    connect(squareWidth, &QLineEdit::editingFinished, this, [this]()
    {
        double width = squareWidth->text().toDouble();
        on_buildingWidthChanged(width);
    });

    connect(rectangularWidth, &QLineEdit::editingFinished, this, [this]()
    {
        double width = rectangularWidth->text().toDouble();
        on_buildingWidthChanged(width);
    });

    connect(circularDiameter, &QLineEdit::editingFinished, this, [this]()
    {
        double width = circularDiameter->text().toDouble();
        on_buildingWidthChanged(width);
    });

    connect(inK,SIGNAL(editingFinished()), this, SLOT(on_inK_editingFinished()));
    connect(inDamping,SIGNAL(editingFinished()), this, SLOT(on_inDamping_editingFinished()));
    connect(inFloorWeight,SIGNAL(editingFinished()), this, SLOT(on_inFloorWeight_editingFinished()));
    connect(inStoryB, SIGNAL(editingFinished()),this,SLOT(on_inStoryB_editingFinished()));
    connect(inStoryFy,SIGNAL(editingFinished()),this, SLOT(on_inStoryFy_editingFinished()));
    connect(inStoryHeight, SIGNAL(editingFinished()), this, SLOT(on_inStoryHeight_editingFinished()));
    connect(inStoryK, SIGNAL(editingFinished()), this, SLOT(on_inStoryK_editingFinished()));
    connect(dragCoefficient,SIGNAL(editingFinished()),this,SLOT(on_dragCoefficient_editingFinished()));
    connect(windGustSpeed,SIGNAL(editingFinished()),this,SLOT(on_windGustSpeed_editingFinished()));
    connect(seed,SIGNAL(editingFinished()),this,SLOT(on_Seed_editingFinished()));

    connect(eqMotion, SIGNAL(currentIndexChanged(QString)), this, SLOT(on_inEarthquakeMotionSelectionChanged(QString)));
    connect(expCatagory,SIGNAL(currentIndexChanged(int)), this, SLOT(on_expCatagory_indexChanged()));

    connect(theSpreadsheet, SIGNAL(cellClicked(int,int)), this, SLOT(on_theSpreadsheet_cellClicked(int,int)));
    connect(theSpreadsheet, SIGNAL(cellEntered(int,int)), this,SLOT(on_theSpreadsheet_cellClicked(int,int)));
    connect(theSpreadsheet, SIGNAL(cellChanged(int,int)), this,SLOT(on_theSpreadsheet_cellChanged(int,int)));

    connect(runButton, SIGNAL(clicked()), this, SLOT(on_runButton_clicked()));
    connect(stopButton, SIGNAL(clicked()), this, SLOT(on_stopButton_clicked()));
    connect(exitButton, SIGNAL(clicked()), this, SLOT(on_exitButton_clicked()));
}

void MainWindow::createOutputPanel() {

    // output layout is the layout we give this widget
    QVBoxLayout *outputLayout = new QVBoxLayout;

    // place everything in one Widget, a QGroupBox to get 1 unit with heading
    // using a vertical layout for the widget so one after another vertically
    //  in this Box we will place 2 other boxes, one for periods and the other displacements

    QGroupBox *outputBox = new QGroupBox("Output");
    QVBoxLayout *outputBoxLayout = new QVBoxLayout;

    QGroupBox *periods = new QGroupBox("Periods");
    QVBoxLayout *periodsLayout = new QVBoxLayout;
    periods->setLayout(periodsLayout);

    QGroupBox *displacements = new QGroupBox("Displacements");
    QVBoxLayout *displacementsLayout = new QVBoxLayout;
    displacements->setLayout(displacementsLayout);

    // will place earthquake display in another

    QGroupBox *earthquakeBox = new QGroupBox("Earthquake");
    QVBoxLayout *earthquakeBoxLayout = new QVBoxLayout;

    // will place wind results in another
    QGroupBox *windBox = new QGroupBox("Wind");
    QVBoxLayout *windBoxLayout = new QVBoxLayout;

    // the earthquake & wind we will place side by side in resonsesLayout:
    QHBoxLayout *responsesLayout = new QHBoxLayout();

    // some unit variables
    QString inch(tr("in  "));
    QString sec(tr("sec"));

    //
    // create GroupBox for periods and combobox for period selection
    //

    QHBoxLayout *periodLayout = new QHBoxLayout();
    periodComboBox = new QComboBox();
    QString t1("FundamentalPeriod");
    periodComboBox->addItem(t1);

    QLabel *res = new QLabel();
    res->setMinimumWidth(100);
    res->setMaximumWidth(100);
    res->setAlignment(Qt::AlignRight);
    currentPeriod=res;

    periodLayout->addWidget(periodComboBox);
    periodLayout->addStretch();
    periodLayout->addWidget(currentPeriod);

    QLabel *unitLabel = new QLabel();
    unitLabel->setText(sec);
    unitLabel->setMinimumWidth(40);
    unitLabel->setMaximumWidth(100);
    periodLayout->addWidget(unitLabel);

    periodLayout->setSpacing(10);
    periodLayout->setMargin(0);

    periodsLayout->addLayout(periodLayout);
    outputBoxLayout->addWidget(periods);

 //   outputBoxLayout->addWidget(periods);

   //
   // earthquake results
   //

   // opengl window
    myEarthquakeResult = new MyGlWidget(0);
    myEarthquakeResult->setMinimumHeight(300);
    myEarthquakeResult->setMinimumWidth(250);
    myEarthquakeResult->setModel(this);
    earthquakeBoxLayout->addWidget(myEarthquakeResult, 1.0);

    // entries for max responses
    currentDispEarthquake = createLabelEntry(tr("Current Disp"), earthquakeBoxLayout, 100, 100, &inch);
    maxEarthquakeDispLabel = createLabelEntry(tr("Max Disp"), earthquakeBoxLayout, 100,100,&inch);
    earthquakeBox->setLayout(earthquakeBoxLayout);

    //
    // wind results similar (need identical so layouts match!)
    //

    myWindResponseResult = new MyGlWidget(1);
    myWindResponseResult->setMinimumHeight(300);
    myWindResponseResult->setMinimumWidth(250);
    myWindResponseResult->setModel(this);
    windBoxLayout->addWidget(myWindResponseResult, 1.0);
    currentDispWind = createLabelEntry(tr("Current Disp"), windBoxLayout, 100, 100, &inch);
    maxWindDispLabel = createLabelEntry(tr("Max Disp"), windBoxLayout, 100,100,&inch);
    windBox->setLayout(windBoxLayout);

    responsesLayout->addWidget(earthquakeBox);
    responsesLayout->addWidget(windBox);

    displacementsLayout->addLayout(responsesLayout,1.0);

   outputBox->setLayout(outputBoxLayout);
   outputLayout->addWidget(outputBox);

   /* *************************************************************
   // add some buttons to give user control over what is displayed
   // putting in main menu view dropdown instead .. review later
   QHBoxLayout *pushButtonsLayout = new QHBoxLayout();
   QPushButton *dispButton = new QPushButton("Displacement");
   QPushButton *driftButton = new QPushButton("Interstory Drift");
   QPushButton *accelButton = new QPushButton("Acceleration");
   QPushButton *shearForceButton = new QPushButton("ShearForce");
   pushButtonsLayout->addWidget(dispButton);
   pushButtonsLayout->addWidget(accelButton);
   pushButtonsLayout->addWidget(driftButton);
   pushButtonsLayout->addWidget(shearForceButton);

   outputBoxLayout->addLayout(pushButtonsLayout);
   ***************************************************************** */

   slider=new QSlider(Qt::Horizontal);
    displacementsLayout->addWidget(slider);

    // output frame to show current time
    QFrame *outputDataFrame = new QFrame();
    outputDataFrame->setObjectName(QString::fromUtf8("outputDataFrame"));
    QVBoxLayout *outputDataLayout = new QVBoxLayout();
    currentTime = createLabelEntry(tr("Current Time"), outputDataLayout, 100,100, &sec);
//    currentDisp = createLabelEntry(tr("Current Roof Disp"), outputDataLayout, 100,100,  &inch);
    outputDataFrame->setLayout(outputDataLayout);
    outputDataFrame->setLineWidth(1);
    outputDataFrame->setFrameShape(QFrame::Box);
    outputDataFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);


    //outputBoxLayout->addWidget(outputDataFrame);
    displacementsLayout->addWidget(outputDataFrame);

     outputBoxLayout->addWidget(displacements, 1.0);

    // add layout to mainLayout and to largeLayout
    mainLayout->addLayout(outputLayout,1.0);
    largeLayout->addLayout(mainLayout); //styleSheet

    // signal and slot connects for slider
    connect(periodComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(on_PeriodSelectionChanged(QString)));
    connect(slider, SIGNAL(sliderPressed()),  this, SLOT(on_slider_sliderPressed()));
    connect(slider, SIGNAL(sliderReleased()), this, SLOT(on_slider_sliderReleased()));
    connect(slider, SIGNAL(valueChanged(int)),this, SLOT(on_slider_valueChanged(int)));

    //outputLayout->addStretch();
}

