/**
 * \file importsource.h
 * Generic baseclass to import from an external source.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 09 Oct 2006
 *
 * Copyright (C) 2006-2011  Urs Fleisch
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

#ifndef IMPORTSOURCE_H
#define IMPORTSOURCE_H

#include "importsourceclient.h"
#include <QString>
#include <QStandardItem>

class QStandardItemModel;
class ImportSourceConfig;
class ImportSourceClient;
class ImportTrackDataVector;

/**
 * Generic baseclass to import from an external source.
 */
class ImportSource : public ImportSourceClient
{
	Q_OBJECT

public:
	/**
	 * Constructor.
	 *
	 * @param parent  parent object
	 * @param trackDataVector track data to be filled with imported values
	 */
	ImportSource(QObject* parent,
							 ImportTrackDataVector& trackDataVector);

	/**
	 * Destructor.
	 */
	virtual ~ImportSource();

	/**
	 * Name of import source.
	 * @return name.
	 */
	virtual QString name() const = 0;

	/** NULL-terminated array of server strings, 0 if not used */
	virtual const char** serverList() const;

	/** default server, 0 to disable */
	virtual const char* defaultServer() const;

	/** default CGI path, 0 to disable */
	virtual const char* defaultCgiPath() const;

	/** anchor to online help, 0 to disable */
	virtual const char* helpAnchor() const;

	/** configuration, 0 if not used */
	virtual ImportSourceConfig* cfg() const;

	/** additional tags option, false if not used */
	virtual bool additionalTags() const;

	/**
	 * Parse result of find request and populate m_albumListBox with results.
	 * This method has to be reimplemented for the specific result data.
	 *
	 * @param searchStr search data received
	 */
	virtual void parseFindResults(const QByteArray& searchStr) = 0;

	/**
	 * Parse result of album request and populate m_trackDataVector with results.
	 * This method has to be reimplemented for the specific result data.
	 *
	 * @param albumStr album data received
	 */
	virtual void parseAlbumResults(const QByteArray& albumStr) = 0;

	/**
	 * Get model with album list.
	 *
	 * @return album list item model.
	 */
	QStandardItemModel* getAlbumListModel() const { return m_albumListModel; }

	/**
	 * Clear model data.
	 */
	void clear();

	/**
	 * Get additional tags option.
	 *
	 * @return true if additional tags are enabled.
	 */
	bool getAdditionalTags() const { return m_additionalTagsEnabled; }

	/**
	 * Set additional tags option.
	 *
	 * @param enable true if additional tags are enabled
	 */
	void setAdditionalTags(bool enable) { m_additionalTagsEnabled = enable; }

	/**
	 * Get cover art option.
	 *
	 * @return true if cover art are enabled.
	 */
	bool getCoverArt() const { return m_coverArtEnabled; }

	/**
	 * Set cover art option.
	 *
	 * @param enable true if cover art are enabled
	 */
	void setCoverArt(bool enable) { m_coverArtEnabled = enable; }

	/**
	 * Replace HTML entities in a string.
	 *
	 * @param str string with HTML entities (e.g. &quot;)
	 *
	 * @return string with replaced HTML entities.
	 */
	static QString replaceHtmlEntities(QString str);

	/**
	 * Replace HTML entities and remove HTML tags.
	 *
	 * @param str string containing HTML
	 *
	 * @return clean up string
	 */
	static QString removeHtml(QString str);

protected:
	QStandardItemModel* m_albumListModel; /**< albums to select */
	ImportTrackDataVector& m_trackDataVector; /**< vector with tracks to import */

private:
	bool m_additionalTagsEnabled;
	bool m_coverArtEnabled;
};

/**
 * QStandardItem subclass for album list.
 */
class AlbumListItem : public QStandardItem {
public:
	/**
	 * Constructor.
	 * @param text    title
	 * @param cat     category
	 * @param idStr   ID
	 */
	AlbumListItem(const QString& text,
				  const QString& cat, const QString& idStr) : 
		QStandardItem(text) {
		setData(cat, Qt::UserRole + 1);
		setData(idStr, Qt::UserRole + 2);
	}

	/**
	 * Get category.
	 * @return category.
	 */
	QString getCategory() const { return data(Qt::UserRole + 1).toString(); }

	/**
	 * Get ID.
	 * @return ID.
	 */
	QString getId() const { return data(Qt::UserRole + 2).toString(); }
};

#endif
