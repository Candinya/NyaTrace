#include "nyatrace_about.h"
#include "ui_nyatrace_about.h"

NyaTraceAbout::NyaTraceAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NyaTraceAbout)
{
    ui->setupUi(this);
}

NyaTraceAbout::~NyaTraceAbout()
{
    delete ui;
}

void NyaTraceAbout::SetVersion(QString & versionString) {
    ui->labelNyaTraceVersion->setText(versionString);
}
