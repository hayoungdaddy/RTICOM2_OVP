#ifndef NEWREGEND_H
#define NEWREGEND_H

#include <QDialog>

namespace Ui {
class NewRegend;
}

class NewRegend : public QDialog
{
    Q_OBJECT

public:
    explicit NewRegend(QWidget *parent = nullptr);
    ~NewRegend();

private:
    Ui::NewRegend *ui;
};

#endif // NEWREGEND_H
