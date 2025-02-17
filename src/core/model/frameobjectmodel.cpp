/**
 * \file frameobjectmodel.cpp
 * Object model with frame information.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 02 Sep 2014
 *
 * Copyright (C) 2014  Urs Fleisch
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

#include "frameobjectmodel.h"
#include <QCoreApplication>

/**
 * Constructor.
 * @param parent parent object
 */
FrameObjectModel::FrameObjectModel(QObject* parent) : QObject(parent)
{
}

/**
 * Destructor.
 */
FrameObjectModel::~FrameObjectModel()
{
}

/**
 * Get frame name.
 * @return translated frame name.
 */
QString FrameObjectModel::name() const
{
  return m_frame.getExtendedType().getTranslatedName();
}

/**
 * Get internal frame name.
 * @return internal frame name, e.g. "TXXX - User defined text information"
 */
QString FrameObjectModel::internalName() const
{
  QString name(m_frame.getInternalName());
  if (name.isEmpty()) {
    name = m_frame.getName();
  }
  if (!name.isEmpty()) {
    int nlPos = name.indexOf(QLatin1Char('\n'));
    if (nlPos > 0) {
      // probably "TXXX - User defined text information\nDescription" or
      // "WXXX - User defined URL link\nDescription"
      name.truncate(nlPos);
    }
    name = QCoreApplication::translate("@default", name.toLatin1().data());
  }
  return name;
}

/**
 * Get frame type.
 * @return type, type Frame::Type.
 */
int FrameObjectModel::type() const
{
  return m_frame.getType();
}

/**
 * Get frame value.
 * @return frame value.
 */
QString FrameObjectModel::value() const
{
  return m_frame.getValue();
}

/**
 * Set frame value.
 * @param value value
 */
void FrameObjectModel::setValue(const QString& value)
{
  if (m_frame.getValue() != value) {
    m_frame.setValueIfChanged(value);
    emit valueChanged(m_frame.getValue());
  }
}

/**
 * Get field list.
 * @return fields.
 */
QList<QObject*> FrameObjectModel::fields()
{
  QList<QObject*> lst;
  const int numFields = m_frame.getFieldList().size();
  if (numFields > 0) {
    for (int i = 0; i < numFields; ++i) {
      FrameFieldObject* fieldObj = new FrameFieldObject(i, this);
      connect(fieldObj, SIGNAL(valueChanged(QVariant)),
              this, SIGNAL(fieldsChanged()));
      lst.append(fieldObj);
    }
  } else {
    FrameFieldObject* fieldObj = new FrameFieldObject(-1, this);
    connect(fieldObj, SIGNAL(valueChanged(QVariant)),
            this, SIGNAL(fieldsChanged()));
    lst.append(fieldObj);
  }
  return lst;
}

/**
 * Set from frame.
 * @param frame frame
 */
void FrameObjectModel::setFrame(const Frame& frame)
{
  m_frame = frame;
}

/**
 * Get frame from object information.
 * @return frame.
 */
Frame FrameObjectModel::getFrame() const
{
  return m_frame;
}

/**
 * Get binary data from data field.
 * @return binary data, empty if not available.
 */
QByteArray FrameObjectModel::getBinaryData() const
{
  QVariant var(Frame::getField(m_frame, Frame::ID_Data));
  if (var.isValid()) {
    return var.toByteArray();
  }
  return QByteArray();
}


/**
 * Constructor.
 * @param index index in field list
 * @param parent parent object
 */
FrameFieldObject::FrameFieldObject(int index, FrameObjectModel* parent) :
  QObject(parent), m_index(index)
{
}

/**
 * Destructor.
 */
FrameFieldObject::~FrameFieldObject()
{
}

/**
 * Get field name.
 * @return translated field name.
 */
QString FrameFieldObject::name() const
{
  if (FrameObjectModel* fom = frameObject()) {
    const Frame::FieldList& fields = fom->m_frame.getFieldList();
    if (m_index >= 0 && m_index < fields.size()) {
      return Frame::Field::getFieldIdName(
            static_cast<Frame::FieldId>(fields.at(m_index).m_id));
    }
  }
  return tr("Text");
}

/**
 * Get field ID.
 * @return id, type Frame::FieldId.
 */
int FrameFieldObject::id() const {
  if (FrameObjectModel* fom = frameObject()) {
    const Frame::FieldList& fields = fom->m_frame.getFieldList();
    if (m_index >= 0 && m_index < fields.size()) {
      return fields.at(m_index).m_id;
    }
  }
  return 0;
}

/**
 * Get field value.
 * @return field value.
 */
QVariant FrameFieldObject::value() const {
  if (FrameObjectModel* fom = frameObject()) {
    const Frame::FieldList& fields = fom->m_frame.getFieldList();
    if (m_index >= 0 && m_index < fields.size()) {
      return fields.at(m_index).m_value;
    } else {
      return fom->value();
    }
  }
  return QVariant();
}

/**
 * Set field value.
 * @param value value
 */
void FrameFieldObject::setValue(const QVariant& value)
{
  if (FrameObjectModel* fom = frameObject()) {
    Frame::FieldList& fields = fom->m_frame.fieldList();
    if (m_index >= 0 && m_index < fields.size()) {
      Frame::Field& fld = fields[m_index];
      if (fld.m_value != value) {
        fld.m_value = value;
        emit valueChanged(fld.m_value);
      }
    } else {
      fom->setValue(value.toString());
    }
  }
}

/**
 * Get frame type.
 * @return type, type Frame::Type.
 */
int FrameFieldObject::type() const
{
  if (FrameObjectModel* fom = frameObject()) {
    return fom->type();
  }
  return Frame::FT_UnknownFrame;
}
