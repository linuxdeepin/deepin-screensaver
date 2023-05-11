// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commandlinemanager.h"

#include <QCommandLineOption>

class CommandLineManagerGlobal : public CommandLineManager
{
};
Q_GLOBAL_STATIC(CommandLineManagerGlobal, commandLineManagerGlobal);

CommandLineManager::CommandLineManager()
    : m_commandParser(new QCommandLineParser)
{
    // 必须设置为长选项模式，否则解析时 -window-id 将识别错误
    m_commandParser->setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    initOptions();
}

CommandLineManager *CommandLineManager::instance()
{
    return commandLineManagerGlobal;
}

CommandLineManager::~CommandLineManager()
{
}

void CommandLineManager::initOptions()
{
    QCommandLineOption subWindowOption("window-id", "displays as a child of the given ID window.\nNOTE:is '-window-id',not '--window-id'", "ID");
    m_commandParser->addOption(subWindowOption);
}

void CommandLineManager::process(const QStringList &arguments)
{
    m_commandParser->process(arguments);
}

bool CommandLineManager::isSet(const QString &name) const
{
    return m_commandParser->isSet(name);
}

QString CommandLineManager::value(const QString &name) const
{
    return m_commandParser->value(name);
}

QStringList CommandLineManager::positionalArguments()
{
    return m_commandParser->positionalArguments();
}

QStringList CommandLineManager::unknownOptionNames()
{
    return m_commandParser->unknownOptionNames();
}
