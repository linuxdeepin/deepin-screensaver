// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slideshowconfigdialog.h"
#include "slideshowconfig.h"

#include <DApplication>

#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QIntValidator>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QGroupBox>
#include <QDebug>
#include <QIcon>

DWIDGET_USE_NAMESPACE

SlideShowConfigDialog::SlideShowConfigDialog(QWidget *parent)
    : QWidget (parent)
{
    //add dconfig
    m_dCfg = DConfig::create(kCfgAppId, kCfgName, "", this);

//    if (!m_dCfg->isValid()) {
//        qApp->exit(-1);
//    }
    initUI();

//    QVBoxLayout *h = new QVBoxLayout(this);
//    h->addWidget(path);
}

SlideShowConfigDialog::~SlideShowConfigDialog()
{

}

bool SlideShowConfigDialog::dconfigValid()
{
    return m_dCfg->isValid();
}

void SlideShowConfigDialog::startConfig()
{
    setWindowFlags(this->windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(QSize(682, 532));
    QScreen *screen = qApp->primaryScreen();
    move(screen->geometry().width() / 2 - this->width() / 2,
         screen->geometry().height() / 2 - this->height() / 2);



    setFocus();
    show();
}

void SlideShowConfigDialog::mousePressEvent(QMouseEvent *)
{
    setFocus();
}

void SlideShowConfigDialog::setSlideShowPath()
{
    QString path = QFileDialog::getExistingDirectory(this);
    if (!path.isEmpty()) {
        m_slideShowPathEdit->setText(path);

        m_dCfg->setValue(kSlideshowPath, path);
    }
}

void SlideShowConfigDialog::setIntervalTime()
{
    int time = m_intervalTimeEdit->text().toInt();

    if (time < kMinIntervalTime || time > kMaxIntervalTime) {
        time = kDefaultTime;
        m_intervalTimeEdit->setText(QString::number(time));
    }
    qInfo() << "set interval time";

    m_dCfg->setValue(kIntervalTime, time);

    this->setFocus();
}

void SlideShowConfigDialog::setShuffle(const bool shuffle)
{
    m_dCfg->setValue(kEnableShuffle, shuffle);
}

void SlideShowConfigDialog::initUI()
{
    QString path = m_dCfg->value(kSlideshowPath).toString();
    QString tipTime = "(" + QString::number(kMinIntervalTime) + "s~" + QString::number(kMaxIntervalTime) + "s)";
    int time = m_dCfg->value(kIntervalTime).toInt();
    if (time < kMinIntervalTime || time > kMaxIntervalTime)
        time = kDefaultTime;
    QIntValidator *v = new QIntValidator(kMinIntervalTime, kMaxIntervalTime);

    QLabel *slideShowPathLabel = new QLabel(tr("Select pisctures path"));
    QLabel *intervalTimeLabel = new QLabel(tr("interval time"));
    QLabel *intervalTimeTipLabel = new QLabel(tipTime);
    QPushButton *openFileBtn = new QPushButton(tr("..."));

    m_slideShowPathEdit = new QLineEdit(path);

    m_intervalTimeEdit = new QLineEdit(QString::number(time));
    m_intervalTimeEdit->setValidator(v);

    m_enableShuffleBox = new QCheckBox(tr("shuffle"));
    m_enableShuffleBox->setChecked(m_dCfg->value(kEnableShuffle).toBool());

    QFont font;
    font.setBold(true);
    font.setPointSize(15);

    QGroupBox *pathGroup = new QGroupBox(this);
    pathGroup->setTitle(tr("Slideshow Path"));
    pathGroup->setAlignment(Qt::AlignLeft);
    pathGroup->setFont(font);
    pathGroup->setFixedWidth(this->width());
    pathGroup->setFlat(true);
    pathGroup->move(0, this->height() / 6);

    font.setBold(false);
    font.setPointSize(12);
    m_slideShowPathEdit->setFont(font);
    slideShowPathLabel->setFont(font);
    openFileBtn->setFont(font);

    m_slideShowPathEdit->setClearButtonEnabled(true);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(slideShowPathLabel);
    pathLayout->addWidget(m_slideShowPathEdit);
    pathLayout->addWidget(openFileBtn);
    pathGroup->setLayout(pathLayout);

    font.setBold(true);
    font.setPointSize(15);

    QGroupBox *settingGroup = new QGroupBox(this);
    settingGroup->setTitle(tr("Slideshow Settings"));
    settingGroup->setAlignment(Qt::AlignLeft);
    settingGroup->setFont(font);
    settingGroup->setFixedWidth(this->width());
    settingGroup->setFlat(true);
    settingGroup->move(0, this->height() / 2);

    font.setBold(false);
    font.setPointSize(12);

    intervalTimeLabel->setFont(font);
    m_intervalTimeEdit->setFont(font);
    m_enableShuffleBox->setFont(font);
    intervalTimeTipLabel->setFont(font);
    m_intervalTimeEdit->move(slideShowPathLabel->width(), m_intervalTimeEdit->y());

    QGridLayout *settingLayout = new QGridLayout();
    settingLayout->addWidget(intervalTimeLabel, 0, 0);
    settingLayout->addWidget(m_intervalTimeEdit, 0, 1);
    settingLayout->addWidget(intervalTimeTipLabel, 0, 2);
    settingLayout->addWidget(m_enableShuffleBox, 1, 0);
    settingGroup->setLayout(settingLayout);

    connect(openFileBtn, &QPushButton::clicked, this, &SlideShowConfigDialog::setSlideShowPath);
    connect(m_slideShowPathEdit, &QLineEdit::editingFinished, this, [ = ]{
        if (!m_slideShowPathEdit->text().isEmpty()) {
            m_dCfg->setValue(kSlideshowPath, m_slideShowPathEdit->text());
        }
    });
    connect(m_intervalTimeEdit, &QLineEdit::editingFinished, this, &SlideShowConfigDialog::setIntervalTime);
    connect(m_enableShuffleBox, &QCheckBox::stateChanged, this, &SlideShowConfigDialog::setShuffle);
}

