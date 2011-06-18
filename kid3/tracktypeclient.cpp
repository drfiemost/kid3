/**
 * \file tracktypeclient.cpp
 * TrackType.org client.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 26 Apr 2007
 *
 * Copyright (C) 2007-2011  Urs Fleisch
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

#include "tracktypeclient.h"
#include "importsourceconfig.h"
#include "kid3.h"

static const char trackTypeServer[] = "tracktype.org:80";

/**
 * Constructor.
 *
 * @param parent          parent object
 * @param trackDataVector track data to be filled with imported values
 */
TrackTypeClient::TrackTypeClient(QObject* parent,
																 ImportTrackDataVector& trackDataVector) :
	FreedbClient(parent, trackDataVector)
{
	setObjectName("TrackTypeClient");
}

/**
 * Destructor.
 */
TrackTypeClient::~TrackTypeClient()
{
}

/**
 * Name of import source.
 * @return name.
 */
QString TrackTypeClient::name() const { return "TrackType"; }

/** NULL-terminated array of server strings, 0 if not used */
const char** TrackTypeClient::serverList() const
{
	static const char* servers[] = {
		"tracktype.org:80",
		0                  // end of StrList
	};
	return servers;
}

/** default server, 0 to disable */
const char* TrackTypeClient::defaultServer() const { return "tracktype.org:80"; }

/** configuration, 0 if not used */
ImportSourceConfig* TrackTypeClient::cfg() const { return &Kid3App::s_trackTypeCfg; }

/**
 * Process finished findCddbAlbum request.
 *
 * @param searchStr search data received
 */
void TrackTypeClient::parseFindResults(const QByteArray& searchStr)
{
/*
210 exact matches found
categ discid dtitle
(more matches...)
.
or
211 close matches found
rock 920b810c Catharsis / Imago
.
theoretically, but never seen
200	categ discid dtitle
*/
	QString str = QString::fromUtf8(searchStr);
	QRegExp catIdTitleRe("([a-z]+)\\s+([0-9a-f]+)\\s+([^/]+ / .+)");
	QStringList lines = str.split(QRegExp("[\\r\\n]+"));
	bool inEntries = false;
	m_albumListModel->clear();
	for (QStringList::const_iterator it = lines.begin(); it != lines.end(); ++it) {
		if (*it == ".") {
			break;
		}
		if (inEntries) {
			if (catIdTitleRe.exactMatch(*it)) {
				m_albumListModel->appendRow(new AlbumListItem(
					catIdTitleRe.cap(3),
					catIdTitleRe.cap(1),
					catIdTitleRe.cap(2)));
			}
		} else {
			if ((*it).startsWith("21") && (*it).indexOf(" match") != -1) {
				inEntries = true;
			} else if ((*it).startsWith("200 ")) {
				if (catIdTitleRe.exactMatch((*it).mid(4))) {
					m_albumListModel->appendRow(new AlbumListItem(
						catIdTitleRe.cap(3),
						catIdTitleRe.cap(1),
						catIdTitleRe.cap(2)));
				}
			}
		}
	}
}

/**
 * Send a query command to search on the server.
 *
 * @param cfg      import source configuration
 * @param artist   artist to search
 * @param album    album to search
 */
void TrackTypeClient::sendFindQuery(
	const ImportSourceConfig* cfg,
	const QString& artist, const QString& album)
{
	// At the moment, only TrackType.org recognizes cddb album commands,
	// so we always use this server for find queries.
	sendRequest(trackTypeServer,
							cfg->m_cgiPath + "?cmd=cddb+album+" +
							encodeUrlQuery(artist + " / " + album) +
							"&hello=noname+localhost+Kid3+" VERSION "&proto=6");
}
