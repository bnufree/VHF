#ifndef VOLUME_H
#define VOLUME_H

#include <QWidget>

namespace Ui {
class Volume;
}

class Volume : public QWidget
{
    Q_OBJECT

public:
    explicit Volume(QWidget *parent = nullptr);
    ~Volume();

    void setValue(const int value);
    int getValue(void);
private:
    Ui::Volume *ui;
};

#endif // VOLUME_H
