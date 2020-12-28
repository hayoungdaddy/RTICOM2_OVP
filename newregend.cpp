#include "newregend.h"
#include "ui_newregend.h"

NewRegend::NewRegend(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewRegend)
{
    ui->setupUi(this);
}

NewRegend::~NewRegend()
{
    delete ui;
}
