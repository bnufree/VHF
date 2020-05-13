#include "volume.h"
#include "ui_volume.h"

Volume::Volume(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Volume)
{
    ui->setupUi(this);

    void (QSpinBox:: *spinSingle)(int) = &QSpinBox::valueChanged;   // 该接口有重载，需定义指明参数为int型
    QObject::connect(ui->spinBox, spinSingle, ui->horizontalSlider, &QSlider::setValue);

    QObject::connect(ui->horizontalSlider, &QSlider::valueChanged, ui->spinBox, &QSpinBox::setValue);
}

Volume::~Volume()
{
    delete ui;
}

void Volume::setValue(const int value)
{
    ui->horizontalSlider->setValue(value);
}

int Volume::getValue()
{
    return  ui->horizontalSlider->value();
}
