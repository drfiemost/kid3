/**
 * \file timeeventmodel.cpp
 * Time event model (synchronized lyrics or event timing codes).
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 14 Mar 2014
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

#include "timeeventmodel.h"
#include <QApplication>
#include <QTextStream>
#include <QtEndian>
#include "eventtimingcode.h"

/**
 * Constructor.
 * @param parent parent widget
 */
TimeEventModel::TimeEventModel(QObject* parent) :
  QAbstractTableModel(parent), m_type(SynchronizedLyrics), m_markedRow(-1)
{
  setObjectName(QLatin1String("TimeEventModel"));
}

/**
 * Destructor.
 */
TimeEventModel::~TimeEventModel()
{
}

/**
 * Get item flags for index.
 * @param index model index
 * @return item flags
 */
Qt::ItemFlags TimeEventModel::flags(const QModelIndex& index) const
{
  Qt::ItemFlags theFlags = QAbstractTableModel::flags(index);
  if (index.isValid())
    theFlags |= Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
  return theFlags;
}

/**
 * Get data for a given role.
 * @param index model index
 * @param role item data role
 * @return data for role
 */
QVariant TimeEventModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() ||
      index.row() < 0 || index.row() >= m_timeEvents.size() ||
      index.column() < 0 || index.column() >= CI_NumColumns)
    return QVariant();
  const TimeEvent& timeEvent = m_timeEvents.at(index.row());
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    if (index.column() == CI_Time)
      return timeEvent.time;
    else
      return timeEvent.data;
  } else if (role == Qt::BackgroundColorRole && index.column() == CI_Data) {
    return index.row() == m_markedRow
        ? QApplication::palette().mid() : Qt::NoBrush;
  }
  return QVariant();
}

/**
 * Set data for a given role.
 * @param index model index
 * @param value data value
 * @param role item data role
 * @return true if successful
 */
bool TimeEventModel::setData(const QModelIndex& index,
                             const QVariant& value, int role)
{
  if (!index.isValid() || role != Qt::EditRole ||
      index.row() < 0 || index.row() >= m_timeEvents.size() ||
      index.column() < 0 || index.column() >= CI_NumColumns)
    return false;
  TimeEvent& timeEvent = m_timeEvents[index.row()];
  if (index.column() == CI_Time) {
    timeEvent.time = value.toTime();
  } else {
    timeEvent.data = value;
  }
  emit dataChanged(index, index);
  return true;
}

/**
 * Get data for header section.
 * @param section column or row
 * @param orientation horizontal or vertical
 * @param role item data role
 * @return header data for role
 */
QVariant TimeEventModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant();
  if (orientation == Qt::Horizontal && section < CI_NumColumns) {
    if (section == CI_Time) {
      return tr("Time");
    } else if (m_type == EventTimingCodes) {
      return tr("Event Code");
    } else {
      return tr("Text");
    }
  }
  return section + 1;
}

/**
 * Get number of rows.
 * @param parent parent model index, invalid for table models
 * @return number of rows,
 * if parent is valid number of children (0 for table models)
 */
int TimeEventModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : m_timeEvents.size();
}

/**
 * Get number of columns.
 * @param parent parent model index, invalid for table models
 * @return number of columns,
 * if parent is valid number of children (0 for table models)
 */
int TimeEventModel::columnCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : CI_NumColumns;
}

/**
 * Insert rows.
 * @param row rows are inserted before this row, if 0 at the begin,
 * if rowCount() at the end
 * @param count number of rows to insert
 * @param parent parent model index, invalid for table models
 * @return true if successful
 */
bool TimeEventModel::insertRows(int row, int count,
                        const QModelIndex&)
{
  beginInsertRows(QModelIndex(), row, row + count - 1);
  for (int i = 0; i < count; ++i)
    m_timeEvents.insert(row, TimeEvent(QTime(), QVariant()));
  endInsertRows();
  return true;
}

/**
 * Remove rows.
 * @param row rows are removed starting with this row
 * @param count number of rows to remove
 * @param parent parent model index, invalid for table models
 * @return true if successful
 */
bool TimeEventModel::removeRows(int row, int count,
                        const QModelIndex&)
{
  beginRemoveRows(QModelIndex(), row, row + count - 1);
  for (int i = 0; i < count; ++i)
    m_timeEvents.removeAt(row);
  endRemoveRows();
  return true;
}

/**
 * Set the model from a list of time events.
 * @param events list of time events
 */
void TimeEventModel::setTimeEvents(const QList<TimeEvent>& events)
{
#if QT_VERSION >= 0x040600
  beginResetModel();
#endif
  m_timeEvents = events;
#if QT_VERSION >= 0x040600
  endResetModel();
#else
  reset();
#endif
}

/**
 * Get time event list from the model.
 * @return list of time events.
 */
QList<TimeEventModel::TimeEvent> TimeEventModel::getTimeEvents() const
{
  return m_timeEvents;
}

/**
 * Set the model from a SYLT frame.
 * @param fields ID3v2 SYLT frame fields
 */
void TimeEventModel::fromSyltFrame(const Frame::FieldList& fields)
{
  QByteArray bytes;
  Frame::Field::TextEncoding enc = Frame::Field::TE_ISO8859_1;
  bool unitIsFrames = false;
  for (Frame::FieldList::const_iterator it = fields.constBegin();
       it != fields.constEnd();
       ++it) {
    const Frame::Field& fld = *it;
    if (fld.m_id == Frame::Field::ID_TextEnc) {
      enc = static_cast<Frame::Field::TextEncoding>(fld.m_value.toInt());
    } else if (fld.m_id == Frame::Field::ID_TimestampFormat) {
      unitIsFrames = fld.m_value.toInt() == 1;
    } else if (fld.m_value.type() == QVariant::ByteArray) {
      bytes = fld.m_value.toByteArray();
    }
  }

  QList<TimeEvent> timeEvents;
  const int numBytes = bytes.size();
  int textBegin = 0, textEnd;
  while (textBegin < numBytes) {
    if (enc == Frame::Field::TE_ISO8859_1 || enc == Frame::Field::TE_UTF8) {
      textEnd = bytes.indexOf('\0', textBegin);
      if (textEnd != -1) {
        ++textEnd;
      }
    } else {
      const ushort* unicode =
          reinterpret_cast<const ushort*>(bytes.constData() + textBegin);
      textEnd = textBegin;
      while (textEnd < numBytes) {
        textEnd += 2;
        if (*unicode++ == 0) {
          break;
        }
      }
    }
    if (textEnd < 0 || textEnd >= numBytes)
      break;

    QString str;
    QByteArray text = bytes.mid(textBegin, textEnd - textBegin);
    switch (enc) {
    case Frame::Field::TE_UTF16:
    case Frame::Field::TE_UTF16BE:
      str = QString::fromUtf16(reinterpret_cast<const ushort*>(text.constData()));
      break;
    case Frame::Field::TE_UTF8:
      str = QString::fromUtf8(text.constData());
      break;
    case Frame::Field::TE_ISO8859_1:
    default:
      str = QString::fromLatin1(text.constData());
    }
    textBegin = textEnd + 4;
    if (textBegin > numBytes)
      break;

    if (str.startsWith(QLatin1Char('\n'))) {
      // New lines start with a new line character, which is removed.
      // If the resulting line starts with one of the special characters
      // (' ', '-', '_'), it is escaped with '#'.
      str.remove(0, 1);
      if (str.length() > 0) {
        QChar ch = str.at(0);
        if (ch == QLatin1Char(' ') || ch == QLatin1Char('-') ||
            ch == QLatin1Char('_')) {
          str.prepend(QLatin1Char('#'));
        }
      }
    } else if (!(str.startsWith(QLatin1Char(' ')) ||
                 str.startsWith(QLatin1Char('-')))) {
      // Continuations of the current line do not start with a new line
      // character. They must start with ' ' or '-'. If the line starts with
      // another character, it is escaped with '_'.
      str.prepend(QLatin1Char('_'));
    }

    quint32 milliseconds = qFromBigEndian<quint32>(
          reinterpret_cast<const uchar*>(bytes.constData()) + textEnd);
    QVariant timeStamp = unitIsFrames
        ? QVariant(milliseconds)
        : QVariant(QTime(0, 0).addMSecs(milliseconds));
    timeEvents.append(TimeEvent(timeStamp, str));
  }
  setTimeEvents(timeEvents);
}

/**
 * Get the model as a SYLT frame.
 * @param fields ID3v2 SYLT frame fields to set
 */
void TimeEventModel::toSyltFrame(Frame::FieldList& fields) const
{
  Frame::Field::TextEncoding enc = Frame::Field::TE_ISO8859_1;
  Frame::FieldList::iterator timeStampFormatIt = fields.end();
  Frame::FieldList::iterator dataIt = fields.end();
  for (Frame::FieldList::iterator it = fields.begin();
       it != fields.end();
       ++it) {
    if (it->m_id == Frame::Field::ID_TextEnc) {
      enc = static_cast<Frame::Field::TextEncoding>(it->m_value.toInt());
    } else if (it->m_id == Frame::Field::ID_TimestampFormat) {
      timeStampFormatIt = it;
    } else if (it->m_value.type() == QVariant::ByteArray) {
      dataIt = it;
    }
  }

  QByteArray bytes;
  bool hasMsTimeStamps = false;
  foreach (const TimeEvent& timeEvent, m_timeEvents) {
    if (!timeEvent.time.isNull()) {
      QString str = timeEvent.data.toString();
      // Remove escaping, restore new line characters.
      if (str.startsWith(QLatin1Char('_'))) {
        str.remove(0, 1);
      } else if (str.startsWith(QLatin1Char('#'))) {
        str.replace(0, 1, QLatin1Char('\n'));
      } else if (!(str.startsWith(QLatin1Char(' ')) ||
                   str.startsWith(QLatin1Char('-')))) {
        str.prepend(QLatin1Char('\n'));
      }

      quint32 milliseconds;
      if (timeEvent.time.type() == QVariant::Time) {
        hasMsTimeStamps = true;
        milliseconds = QTime(0, 0).msecsTo(timeEvent.time.toTime());
      } else {
        milliseconds = timeEvent.data.toUInt();
      }
      switch (enc) {
      case Frame::Field::TE_UTF16:
      case Frame::Field::TE_UTF16BE:
      {
        const ushort* unicode = str.utf16();
        do {
          uchar lsb = *unicode & 0xff;
          uchar msb = *unicode >> 8;
          if (enc == Frame::Field::TE_UTF16) {
            bytes.append(static_cast<char>(lsb));
            bytes.append(static_cast<char>(msb));
          } else {
            bytes.append(static_cast<char>(msb));
            bytes.append(static_cast<char>(lsb));
          }
        } while (*unicode++);
        break;
      }
      case Frame::Field::TE_UTF8:
        bytes.append(str.toUtf8());
        bytes.append('\0');
        break;
      case Frame::Field::TE_ISO8859_1:
      default:
        bytes.append(str.toLatin1());
        bytes.append('\0');
      }
      uchar timeStamp[4];
      qToBigEndian(milliseconds, timeStamp);
      bytes.append(reinterpret_cast<const char*>(timeStamp), sizeof(timeStamp));
    }
  }

  if (hasMsTimeStamps && timeStampFormatIt != fields.end()) {
    timeStampFormatIt->m_value = 2;
  }
  if (dataIt != fields.end()) {
    if (bytes.isEmpty()) {
      // id3lib bug: Empty binary fields are not written, so add a minimal field
      bytes = QByteArray(4 + (enc == Frame::Field::TE_UTF16 ||
                              enc == Frame::Field::TE_UTF16BE ? 2 : 1),
                         '\0');
    }
    dataIt->m_value = bytes;
  }
}

/**
 * Set the model from a ETCO frame.
 * @param fields ID3v2 ETCO frame fields
 */
void TimeEventModel::fromEtcoFrame(const Frame::FieldList& fields)
{
  QByteArray bytes;
  bool hasTimestampFormatField = false;
  bool unitIsFrames = false;
  for (Frame::FieldList::const_iterator it = fields.constBegin();
       it != fields.constEnd();
       ++it) {
    const Frame::Field& fld = *it;
    if (fld.m_id == Frame::Field::ID_TimestampFormat) {
      hasTimestampFormatField = true;
      unitIsFrames = fld.m_value.toInt() == 1;
    } else if (fld.m_value.type() == QVariant::ByteArray) {
      bytes = fld.m_value.toByteArray();
    }
  }
  if (!hasTimestampFormatField && !bytes.isEmpty()) {
    // id3lib bug: There is only a single data field for ETCO frames,
    // but it should be preceeded by an ID_TimestampFormat field.
    unitIsFrames = bytes.at(0) == 1;
    bytes.remove(0, 1);
  }

  QList<TimeEvent> timeEvents;
  const int numBytes = bytes.size();
  int pos = 0;
  while (pos < numBytes) {
    int code = static_cast<uchar>(bytes.at(pos));
    ++pos;
    if (pos + 4 > numBytes)
      break;

    quint32 milliseconds = qFromBigEndian<quint32>(
          reinterpret_cast<const uchar*>(bytes.constData()) + pos);
    pos += 4;
    QVariant timeStamp = unitIsFrames
        ? QVariant(milliseconds)
        : QVariant(QTime(0, 0).addMSecs(milliseconds));
    timeEvents.append(TimeEvent(timeStamp, code));
  }
  setTimeEvents(timeEvents);
}

/**
 * Get the model as an ETCO frame.
 * @param fields ID3v2 ETCO frame fields to set
 */
void TimeEventModel::toEtcoFrame(Frame::FieldList& fields) const
{
  Frame::FieldList::iterator timeStampFormatIt = fields.end();
  Frame::FieldList::iterator dataIt = fields.end();
  for (Frame::FieldList::iterator it = fields.begin();
       it != fields.end();
       ++it) {
    if (it->m_id == Frame::Field::ID_TimestampFormat) {
      timeStampFormatIt = it;
    } else if (it->m_value.type() == QVariant::ByteArray) {
      dataIt = it;
    }
  }

  QByteArray bytes;
  bool hasMsTimeStamps = false;
  foreach (const TimeEvent& timeEvent, m_timeEvents) {
    if (!timeEvent.time.isNull()) {
      int code = timeEvent.data.toInt();
      bytes.append(static_cast<char>(code));

      quint32 milliseconds;
      if (timeEvent.time.type() == QVariant::Time) {
        hasMsTimeStamps = true;
        milliseconds = QTime(0, 0).msecsTo(timeEvent.time.toTime());
      } else {
        milliseconds = timeEvent.data.toUInt();
      }
      uchar timeStamp[4];
      qToBigEndian(milliseconds, timeStamp);
      bytes.append(reinterpret_cast<const char*>(timeStamp), sizeof(timeStamp));
    }
  }

  if (timeStampFormatIt == fields.end()) {
    // id3lib bug: There is only a single data field for ETCO frames,
    // but it should be preceeded by an ID_TimestampFormat field.
    bytes.prepend(2);
  } else if (hasMsTimeStamps) {
    timeStampFormatIt->m_value = 2;
  }
  if (dataIt != fields.end()) {
    dataIt->m_value = bytes;
  }
}

/**
 * Mark row for a time stamp.
 * Marks the first row with time >= @a timeStamp.
 * @param timeStamp time
 * @see getMarkedRow()
 */
void TimeEventModel::markRowForTimeStamp(const QTime& timeStamp)
{
  int row = 0, oldRow = m_markedRow, newRow = -1;
  for (QList<TimeEvent>::const_iterator it = m_timeEvents.constBegin();
       it != m_timeEvents.constEnd();
       ++it) {
    const TimeEvent& timeEvent = *it;
    QTime time = timeEvent.time.toTime();
    if (time.isValid() && time >= timeStamp) {
      if (timeStamp.msecsTo(time) > 1000 && row > 0) {
        --row;
      }
      if (row == 0 && timeStamp == QTime(0, 0) &&
          m_timeEvents.at(0).time.toTime() != timeStamp) {
        row = -1;
      }
      newRow = row;
      break;
    }
    ++row;
  }
  if (newRow != oldRow &&
      !(newRow == -1 && oldRow == m_timeEvents.size() - 1)) {
    m_markedRow = newRow;
    if (oldRow != -1) {
      QModelIndex idx = index(oldRow, CI_Data);
      emit dataChanged(idx, idx);
    }
    if (newRow != -1) {
      QModelIndex idx = index(newRow, CI_Data);
      emit dataChanged(idx, idx);
    }
  }
}

/**
 * Clear the marked row.
 */
void TimeEventModel::clearMarkedRow()
{
  if (m_markedRow != -1) {
    QModelIndex idx = index(m_markedRow, CI_Data);
    m_markedRow = -1;
    emit dataChanged(idx, idx);
  }
}

/**
 * Set the model from an LRC file.
 * @param stream LRC file stream
 */
void TimeEventModel::fromLrcFile(QTextStream& stream)
{
  QRegExp timeStampRe(QLatin1String(
                        "([[<])(\\d\\d):(\\d\\d)(?:\\.(\\d{1,3}))?([\\]>])"));
  QList<TimeEvent> timeEvents;
  forever {
    QString line = stream.readLine();
    if (line.isNull())
      break;

    QList<QTime> emptyEvents;
    char firstChar = '\0';
    int pos = timeStampRe.indexIn(line, 0);
    while (pos != -1) {
      bool newLine = timeStampRe.cap(1) == QLatin1String("[");
      QString millisecondsStr = timeStampRe.cap(4);
      int milliseconds = millisecondsStr.toInt();
      if (millisecondsStr.length() == 2) {
        milliseconds *= 10;
      } else if (millisecondsStr.length() == 1) {
        milliseconds *= 100;
      }
      QTime timeStamp(0,
                      timeStampRe.cap(2).toInt(),
                      timeStampRe.cap(3).toInt(),
                      milliseconds);
      int textBegin = pos + timeStampRe.matchedLength();

      pos = timeStampRe.indexIn(line, textBegin);
      int textLen = pos != -1 ? pos - textBegin : -1;
      QString str = line.mid(textBegin, textLen);
      if (m_type == EventTimingCodes) {
        EventTimeCode etc =
            EventTimeCode::fromString(str.toLatin1().constData());
        if (etc.isValid()) {
          timeEvents.append(TimeEvent(timeStamp, etc.getCode()));
        }
      } else {
        if (firstChar != '\0') {
          str.prepend(QChar::fromLatin1(firstChar));
          firstChar = '\0';
        }
        if (newLine) {
          if (str.startsWith(QLatin1Char(' ')) ||
              str.startsWith(QLatin1Char('-')) ||
              str.startsWith(QLatin1Char('_'))) {
            str.prepend(QLatin1Char('#'));
          }
        } else if (!(str.startsWith(QLatin1Char(' ')) ||
                     str.startsWith(QLatin1Char('-')))) {
          str.prepend(QLatin1Char('_'));
        }
        if (pos != -1) {
          if (timeStampRe.cap(1) == QLatin1String("<")) {
            if (str.endsWith(QLatin1Char(' ')) ||
                str.endsWith(QLatin1Char('-'))) {
              firstChar = str.at(str.length() - 1).toLatin1();
              str.truncate(str.length() - 1);
            }
          }
          if (str.isEmpty()) {
            // The next time stamp follows immediately with a common text.
            emptyEvents.append(timeStamp);
            continue;
          }
        }
        foreach (const QTime& time, emptyEvents) {
          timeEvents.append(TimeEvent(time, str));
        }
        timeEvents.append(TimeEvent(timeStamp, str));
      }
    }
  }
  qSort(timeEvents);
  setTimeEvents(timeEvents);
}

/**
 * Store the model in an LRC file.
 * @param stream LRC file stream
 * @param title optional title
 * @param artist optional artist
 * @param album optional album
 */
void TimeEventModel::toLrcFile(QTextStream& stream, const QString& title,
                               const QString& artist, const QString& album)
{
  bool atBegin = true;
  if (!title.isEmpty()) {
    stream << QLatin1String("[ti:") << title << QLatin1String("]\r\n");
    atBegin = false;
  }
  if (!artist.isEmpty()) {
    stream << QLatin1String("[ar:") << artist << QLatin1String("]\r\n");
    atBegin = false;
  }
  if (!album.isEmpty()) {
    stream << QLatin1String("[al:") << album << QLatin1String("]\r\n");
    atBegin = false;
  }
  foreach (const TimeEvent& timeEvent, m_timeEvents) {
    QTime time = timeEvent.time.toTime();
    if (time.isValid()) {
      char firstChar = '\0';
      bool newLine = true;
      QString str;
      if (m_type == EventTimingCodes) {
        str = EventTimeCode(timeEvent.data.toInt()).toString();
      } else {
        str = timeEvent.data.toString();
        if (str.startsWith(QLatin1Char('_'))) {
          str.remove(0, 1);
          newLine = false;
        } else if (str.startsWith(QLatin1Char('#'))) {
          str.remove(0, 1);
        } else if (str.startsWith(QLatin1Char(' ')) ||
                   str.startsWith(QLatin1Char('-'))) {
          firstChar = str.at(0).toLatin1();
          str.remove(0, 1);
          newLine = false;
        }
      }

      if (newLine) {
        if (!atBegin) {
          stream << QLatin1String("\r\n");
        }
        stream << QLatin1Char('[') << timeStampToString(time).toLatin1()
               << QLatin1Char(']') << str.toLatin1();
      } else {
        if (firstChar != '\0') {
          stream << firstChar;
        }
        stream << QLatin1Char('<') << timeStampToString(time).toLatin1()
               << QLatin1Char('>') << str.toLatin1();
      }
      atBegin = false;
    }
  }
  if (!atBegin) {
    stream << QLatin1String("\r\n");
  }
}

/**
 * Format a time suitable for a time stamp.
 * @param time time stamp
 * @return string of the format "mm:ss.zz"
 */
QString TimeEventModel::timeStampToString(const QTime& time)
{
  QString text = QString(QLatin1String("%1:%2.%3")).
      arg(time.minute(), 2, 10, QLatin1Char('0')).
      arg(time.second(), 2, 10, QLatin1Char('0')).
      arg(time.msec() / 10, 2, 10, QLatin1Char('0'));
  if (time.hour() != 0) {
    text.prepend(QString::number(time.hour()) + QLatin1Char(':'));
  }
  return text;
}
