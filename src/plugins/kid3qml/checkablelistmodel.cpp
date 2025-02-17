/**
 * \file checkablelistmodel.h
 * Proxy model to use QAbstractItemModel with QML.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 23 Sep 2014
 *
 * Copyright (C) 2014-2017  Urs Fleisch
 *
 * This file is part of Kid3.
 *
 * Kid3 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Kid3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "checkablelistmodel.h"
#include <QItemSelectionModel>

CheckableListModel::CheckableListModel(QObject* parent) :
  QAbstractProxyModel(parent), m_selModel(0)
{
}

CheckableListModel::~CheckableListModel()
{
}

QItemSelectionModel* CheckableListModel::selectionModel() const
{
  return m_selModel;
}

void CheckableListModel::setSelectionModel(QItemSelectionModel* selModel)
{
  if (m_selModel != selModel) {
    if (m_selModel) {
      disconnect(m_selModel, 0, this, 0);
    }
    m_selModel = selModel;
    if (m_selModel) {
      connect(m_selModel,
              SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
              this, SLOT(onSelectionChanged(QItemSelection,QItemSelection)));
      connect(m_selModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
              this, SLOT(onCurrentChanged(QModelIndex,QModelIndex)));
    }
    emit selectionModelChanged();
  }
}

void CheckableListModel::setSelectionModelObject(QObject *obj)
{
  if (QItemSelectionModel* selModel = qobject_cast<QItemSelectionModel*>(obj)) {
    setSelectionModel(selModel);
  }
}

QModelIndex CheckableListModel::rootIndex() const
{
  return m_rootIndex;
}

void CheckableListModel::setRootIndex(const QModelIndex& rootIndex)
{
  if (m_rootIndex != rootIndex) {
    beginResetModel();
    m_rootIndex = rootIndex;
    endResetModel();
    emit rootIndexChanged();
  }
}

QModelIndex CheckableListModel::modelIndex(int row) const
{
  QAbstractItemModel* srcModel = sourceModel();
  return srcModel ? srcModel->index(row, 0, m_rootIndex) : QModelIndex();
}

QModelIndex CheckableListModel::parentModelIndex() const
{
  return m_rootIndex.parent();
}

bool CheckableListModel::setDataValue(int row, const QByteArray& roleName,
                                      const QVariant& value)
{
  QHash<int,QByteArray> roleHash = roleNames();
  for (QHash<int,QByteArray>::const_iterator it = roleHash.constBegin();
       it != roleHash.constEnd();
       ++it) {
    if (it.value() == roleName) {
      return setData(index(row, 0), value, it.key());
    }
  }
  return false;
}

QVariant CheckableListModel::getDataValue(int row,
                                          const QByteArray& roleName) const
{
  QHash<int,QByteArray> roleHash = roleNames();
  for (QHash<int,QByteArray>::const_iterator it = roleHash.constBegin();
       it != roleHash.constEnd();
       ++it) {
    if (it.value() == roleName) {
      return data(index(row, 0), it.key());
    }
  }
  return QVariant();
}

bool CheckableListModel::hasModelChildren(int row) const
{
  QAbstractItemModel* srcModel = sourceModel();
  return srcModel && srcModel->hasChildren(mapToSource(index(row, 0)));
}

int CheckableListModel::currentRow() const
{
  return m_selModel ? mapFromSource(m_selModel->currentIndex()).row() : -1;
}

void CheckableListModel::setCurrentRow(int row)
{
  if (m_selModel) {
    m_selModel->setCurrentIndex(
          mapToSource(index(row, 0)),
          QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }
}

Qt::ItemFlags CheckableListModel::flags(const QModelIndex& index) const
{
  Qt::ItemFlags itemFlags = QAbstractProxyModel::flags(index);
  if (index.isValid() && index.column() == 0 && m_selModel) {
    itemFlags |= Qt::ItemIsUserCheckable;
  }
  return itemFlags;
}

QVariant CheckableListModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::CheckStateRole) {
    if (index.column() != 0)
      return QVariant();
    if (!m_selModel)
      return Qt::Unchecked;
    return m_selModel->selection().contains(mapToSource(index))
        ? Qt::Checked : Qt::Unchecked;
  }
  return QAbstractProxyModel::data(index, role);
}

bool CheckableListModel::setData(const QModelIndex& index,
                                 const QVariant& value, int role)
{
  if (role == Qt::CheckStateRole) {
    if (index.column() != 0)
      return false;
    if (!m_selModel)
      return false;

    Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
    const QModelIndex srcIndex = mapToSource(index);
    m_selModel->setCurrentIndex(srcIndex, state == Qt::Checked
      ? QItemSelectionModel::Select | QItemSelectionModel::Rows
      : QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
    emit dataChanged(index, index);
    return true;
  }
  return QAbstractProxyModel::setData(index, value, role);
}

void CheckableListModel::setSourceModel(QAbstractItemModel* srcModel)
{
  if (sourceModel() != srcModel) {
    QAbstractProxyModel::setSourceModel(srcModel);
    emit sourceModelChanged();

    if (sourceModel()) {
      disconnect(sourceModel(), 0, this, 0);
    }
    QAbstractProxyModel::setSourceModel(srcModel);
    if (srcModel) {
#if QT_VERSION < 0x050000
      setRoleNames(srcModel->roleNames());
#endif
      connect(srcModel, SIGNAL(modelAboutToBeReset()),
              this, SLOT(onModelAboutToBeReset()));
      connect(srcModel, SIGNAL(modelReset()),
              this, SLOT(onModelReset()));
      connect(srcModel, SIGNAL(layoutAboutToBeChanged()),
              this, SIGNAL(layoutAboutToBeChanged()));
      connect(srcModel, SIGNAL(layoutChanged()),
              this, SIGNAL(layoutChanged()));
      connect(srcModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
              this, SLOT(onDataChanged(QModelIndex,QModelIndex)));
      connect(srcModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
              this, SLOT(onRowsAboutToBeRemoved(QModelIndex,int,int)));
      connect(srcModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
              this, SLOT(onRowsRemoved(QModelIndex,int,int)));
      connect(srcModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
              this, SLOT(onRowsAboutToBeInserted(QModelIndex,int,int)));
      connect(srcModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
              this, SLOT(onRowsInserted(QModelIndex,int,int)));
    }
  }
}

void CheckableListModel::setSourceModelObject(QObject* obj)
{
  if (QAbstractItemModel* srcModel = qobject_cast<QAbstractItemModel*>(obj)) {
    setSourceModel(srcModel);
  }
}

void CheckableListModel::onModelAboutToBeReset()
{
  beginResetModel();
}

void CheckableListModel::onModelReset()
{
  endResetModel();
}

void CheckableListModel::onDataChanged(const QModelIndex& topLeft,
                                           const QModelIndex& bottomRight)
{
  QModelIndex first = mapFromSource(topLeft);
  QModelIndex last = mapFromSource(bottomRight);
  if (first.isValid() && last.isValid() &&
      first.parent() == last.parent() && first.column() == last.column()) {
    emit dataChanged(first, last);
  }
}

void CheckableListModel::onRowsAboutToBeRemoved(const QModelIndex& parent,
                                                    int first, int last)
{
  if (parent == m_rootIndex) {
    beginRemoveRows(mapFromSource(parent), first, last);
  }
}

void CheckableListModel::onRowsRemoved(const QModelIndex &parent,
                                           int first, int last)
{
  Q_UNUSED(first);
  Q_UNUSED(last);
  if (parent == m_rootIndex) {
    endRemoveRows();
  }
}

void CheckableListModel::onRowsAboutToBeInserted(const QModelIndex& parent,
                                                     int first, int last)
{
  if (parent == m_rootIndex) {
    beginInsertRows(mapFromSource(parent), first, last);
  }
}

void CheckableListModel::onRowsInserted(const QModelIndex& parent,
                                            int first, int last)
{
  Q_UNUSED(first);
  Q_UNUSED(last);
  if (parent == m_rootIndex) {
    endInsertRows();
  }
}

void CheckableListModel::onSelectionChanged(const QItemSelection& selected,
                                          const QItemSelection& deselected)
{
  foreach (const QItemSelectionRange& range, mapSelectionFromSource(selected))
    emit dataChanged(range.topLeft(), range.bottomRight());
  foreach (const QItemSelectionRange& range, mapSelectionFromSource(deselected))
    emit dataChanged(range.topLeft(), range.bottomRight());
}

void CheckableListModel::onCurrentChanged(const QModelIndex& current,
                                          const QModelIndex& previous)
{
  QModelIndex idx = mapFromSource(current);
  emit currentRowChanged(idx.row());
  emit dataChanged(idx, idx);
  idx = mapFromSource(previous);
  emit dataChanged(idx, idx);
}

QModelIndex CheckableListModel::index(int row, int column,
                                      const QModelIndex& parent) const
{
  return parent.isValid() ? QModelIndex() : createIndex(row, column);
}

QModelIndex CheckableListModel::parent(const QModelIndex&) const
{
  return QModelIndex();
}

int CheckableListModel::rowCount(const QModelIndex& parent) const
{
  QAbstractItemModel* srcModel = sourceModel();
  return !parent.isValid() && srcModel ? srcModel->rowCount(m_rootIndex) : 0;
}

int CheckableListModel::columnCount(const QModelIndex& parent) const
{
  QAbstractItemModel* srcModel = sourceModel();
  return !parent.isValid() && srcModel ?srcModel->columnCount(m_rootIndex) : 0;
}

QModelIndex CheckableListModel::mapToSource(const QModelIndex& proxyIndex) const
{
  QAbstractItemModel* srcModel = sourceModel();
  return proxyIndex.isValid() && srcModel
      ? srcModel->index(proxyIndex.row(), proxyIndex.column(), m_rootIndex)
      : QModelIndex();
}

QModelIndex CheckableListModel::mapFromSource(const QModelIndex& srcIndex) const
{
  return srcIndex.parent() == m_rootIndex
      ? createIndex(srcIndex.row(), srcIndex.column()) : QModelIndex();
}
