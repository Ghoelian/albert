// Copyright (c) 2023 Manuel Schneider

#pragma once
#include "albert/extension/queryhandler/globalqueryhandler.h"
#include <QCoreApplication>

class AppQueryHandler : public albert::GlobalQueryHandler
{
    Q_DECLARE_TR_FUNCTIONS(AppQueryHandler)

public:
    AppQueryHandler();
    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString defaultTrigger() const override;
    std::vector<albert::RankItem> handleGlobalQuery(const GlobalQuery*) const override;

private:
    std::vector<std::shared_ptr<albert::Item>> items_;
    static const QStringList icon_urls;

};
