// Copyright (c) 2022 Manuel Schneider

#include "albert/extensions/queryhandler.h"
#include "queryengine.h"
#include "triggerwidget.h"
#include <QHeaderview>
#include <QTableView>
#include <QVBoxLayout>
using namespace albert;
using namespace std;

enum class Column {
    Name,
    Trigger,
    Description
};

struct TriggerModel : public QAbstractTableModel, ExtensionWatcher<QueryHandler>
{
    struct Entry {
        QueryHandler *handler;
        QString trigger;
        bool enabled;
    };
    vector<Entry> query_handlers;
    QueryEngine &engine;

    explicit TriggerModel(albert::ExtensionRegistry &registry, QueryEngine &engine):
            ExtensionWatcher<QueryHandler>(registry), engine(engine)
    {
        update();
    }

    void update()
    {
        query_handlers.clear();
        for (auto &[handler, config] : engine.handlerConfig()){
            Entry entry{handler, config.trigger, config.enabled};
            query_handlers.emplace_back(entry);
        }
        ::sort(begin(query_handlers), end(query_handlers),
               [](const auto& a,const auto& b){ return a.handler->id() < b.handler->id(); });
    }

    void onAdd(QueryHandler *t) override
    {
        beginResetModel();
        update();
        endResetModel();
    }

    void onRem(QueryHandler *t) override
    {
        beginResetModel();
        update();
        endResetModel();
    }

    int rowCount(const QModelIndex &parent) const override
    {
        return (int)query_handlers.size();
    }

    int columnCount(const QModelIndex &parent) const override
    {
        return 3;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (index.column() == (int)Column::Name){
            if (role == Qt::DisplayRole)
                return query_handlers[index.row()].handler->name();

        } else if (index.column() == (int)Column::Description){
            if (role == Qt::DisplayRole)
                return query_handlers[index.row()].handler->description();

        } else if (index.column() == (int)Column::Trigger){
            auto &entry = query_handlers[index.row()];
            if (role == Qt::DisplayRole) {
                return QString(entry.trigger).replace(" ", " ");

            } else if (role == Qt::EditRole) {
                return entry.trigger;

            } else if (role == Qt::ToolTipRole) {
                QStringList sl;
                if (!entry.handler->allow_trigger_remap())
                    sl << "This extension does not allow trigger remapping.";
                if (entry.enabled && engine.activeTriggers().at(entry.trigger) != entry.handler)
                    sl << QString("Trigger conflict: '%1' reserved by extension '%2'.")
                            .arg(entry.trigger, engine.activeTriggers().at(entry.trigger)->name());
                if (!sl.isEmpty())
                    return sl.join(" ");

            }else if (role == Qt::CheckStateRole){
                return query_handlers[index.row()].enabled ? Qt::Checked : Qt::Unchecked;

            } else if (role == Qt::FontRole) {
                if (!query_handlers[index.row()].handler->allow_trigger_remap()){
                    QFont f;
                    f.setItalic(true);
                    return f;
                }

            } else if (role == Qt::ForegroundRole) {
                if (entry.enabled && engine.activeTriggers().at(entry.trigger) != entry.handler)
                    return QColor(Qt::red);

            }
        }
        return {};
    }

    bool setData(const QModelIndex &idx, const QVariant &value, int role) override
    {
        if (idx.column() == (int) Column::Trigger) {
            if (role == Qt::EditRole) {

                if (value.toString().isEmpty())
                    return false;

                engine.setTrigger(query_handlers[idx.row()].handler, value.toString());
                update();
                emit dataChanged(index(0, (int) Column::Trigger),
                                 index(0, (int) Column::Trigger),
                                 {Qt::DisplayRole});
                return true;
            } else if (role == Qt::CheckStateRole) {
                engine.setEnabled(query_handlers[idx.row()].handler,
                                  static_cast<Qt::CheckState>(value.toUInt()) == Qt::Checked);
                update();
                emit dataChanged(idx, idx, {Qt::CheckStateRole});
                return true;
            }
        }
        return false;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (role == Qt::DisplayRole)
            switch ((Column)section) {
                case Column::Name: return "Extension";
                case Column::Trigger: return "Trigger";
                case Column::Description: return "Description";
            }
        return {};
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        switch ((Column)index.column()) {
            case Column::Name:
            case Column::Description:
                return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
            case Column::Trigger:
                if (query_handlers[index.row()].handler->allow_trigger_remap())
                    return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEditable;
                else
                    return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable;
            default: return {};
        }
    }
};

TriggerWidget::TriggerWidget(albert::ExtensionRegistry &er, QueryEngine &qe)
{
    auto *layout = new QVBoxLayout(this);
    auto *model = new TriggerModel(er, qe);
    auto *view = new QTableView(this);
    layout->addWidget(view);
    
    view->verticalHeader()->hide();
    view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->horizontalHeader()->setStretchLastSection(true);
    view->setShowGrid(false);
    view->setAlternatingRowColors(true);
    view->setModel(model);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setEditTriggers(QTableView::DoubleClicked|QTableView::SelectedClicked|QTableView::EditKeyPressed);

    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [view, model](const QModelIndex &current, const QModelIndex &previous){
        view->blockSignals(true);
        view->setCurrentIndex(model->index(current.row(), (int)Column::Trigger));
        view->blockSignals(false);
    });


}