// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slideshowconfigdialog.h"
#include "slideshowconfig.h"

#include <DLabel>
#include <DPushButton>
#include <DLineEdit>
#include <DFileDialog>

#include <QScreen>
#include <QVBoxLayout>

#define SPECIFYLINEEDITLENGTH 65

SlideShowConfigDialog::SlideShowConfigDialog(QWidget *parent)
    : DDialog (parent)
{
    setModal(true);
    initUI();
    initConnect();
}

SlideShowConfigDialog::~SlideShowConfigDialog()
{

}

void SlideShowConfigDialog::initUI()
{
    DWidget *selectWidget = new DWidget(this);
    selectWidget->setFixedSize(this->width() + 100, 50);
    selectWidget->move(50, 50);

    DLabel *slideShowPathLabel = new DLabel(tr("Picture path"));

    m_openFileBtn = new DPushButton(tr("..."));
    m_openFileBtn->setFocusPolicy(Qt::NoFocus);

    m_slideShowPathEdit = new DLineEdit();
    m_slideShowPathEdit->lineEdit()->setReadOnly(true);
    m_slideShowPathEdit->setFocusPolicy(Qt::NoFocus);
    m_slideShowPathEdit->setContextMenuPolicy(Qt::NoContextMenu);

    m_slideShowPathEdit->setPlaceholderText(tr("Select a path of pictures"));
    m_slideShowPathEdit->setClearButtonEnabled(false);

    QHBoxLayout *layoutPath = new QHBoxLayout(selectWidget);
    layoutPath->addWidget(slideShowPathLabel);
    layoutPath->addWidget(m_slideShowPathEdit);
    layoutPath->addWidget(m_openFileBtn);

    DWidget *timeIntervalWidget = new DWidget(this);
    timeIntervalWidget->setFixedSize(selectWidget->size());
    timeIntervalWidget->move(50, 150);

    m_intervalTimeEdit = new DLineEdit();
    m_intervalTimeEdit->setFixedWidth(SPECIFYLINEEDITLENGTH);
    m_intervalTimeEdit->setClearButtonEnabled(false);
    DLabel *intervalTimeLabel = new DLabel(tr("interval time"));
    DLabel *intervalTimeTipLabel = new DLabel(QString("%1: %2-%3")
                                              .arg(tr("Range"))
                                              .arg(QString::number(kMinIntervalTime))
                                              .arg(QString::number(kMaxIntervalTime)));
    QHBoxLayout *layoutTime = new QHBoxLayout(timeIntervalWidget);
    layoutTime->addWidget(intervalTimeLabel);
    layoutTime->addWidget(m_intervalTimeEdit);
    layoutTime->addWidget(intervalTimeTipLabel);
    layoutTime->addStretch(3);

    m_enableShuffleBox = new DCheckBox(tr("shuffle"), this);
    m_enableShuffleBox->move(50, 250);

    initUiValue();
}

void SlideShowConfigDialog::initConnect()
{
    connect(m_openFileBtn, &DPushButton::clicked, this, &SlideShowConfigDialog::setSlideShowPath);
    connect(m_intervalTimeEdit, &DLineEdit::editingFinished, this, &SlideShowConfigDialog::setIntervalTime);
    connect(m_enableShuffleBox, &QCheckBox::stateChanged, this, &SlideShowConfigDialog::setPlayMode);
}

void SlideShowConfigDialog::initUiValue()
{
    m_slideShowPathEdit->setText(SlideshowConfig::instance()->slideshowPath());
    m_intervalTimeEdit->setText(QString::number(SlideshowConfig::instance()->intervalTime()));
    m_enableShuffleBox->setChecked(SlideshowConfig::instance()->isShuffle());
}

QString SlideShowConfigDialog::defaultPath()
{
    return QDir::homePath() + "/Pictures";
}

void SlideShowConfigDialog::startConfig()
{
    setWindowFlags(this->windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(QSize(582, 332));
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
    QString currentPath = m_slideShowPathEdit->text();

    if (currentPath.isEmpty() || !QFile::exists(currentPath)) {
        currentPath = defaultPath();
        qWarning() << "The default is empty or does not exist,set path is " << currentPath;
    }

    QString selectPath = DFileDialog::getExistingDirectory(nullptr, tr("Picture path"), currentPath,
                                                           QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (selectPath.isEmpty())
        return;

    m_slideShowPathEdit->setText(selectPath);
    SlideshowConfig::instance()->setSlideShowPath(selectPath);
}

void SlideShowConfigDialog::setIntervalTime()
{
    int time = m_intervalTimeEdit->text().toInt();

    if (time < kMinIntervalTime || time > kMaxIntervalTime) {
        m_intervalTimeEdit->setText(QString::number(SlideshowConfig::instance()->intervalTime()));
        return;
    }

    SlideshowConfig::instance()->setIntervalTime(time);

    this->setFocus();
}

void SlideShowConfigDialog::setPlayMode(const bool &mode)
{
    SlideshowConfig::instance()->setShuffle(mode);
}
