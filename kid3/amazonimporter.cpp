/**
 * \file amazonimporter.cpp
 * Amazon database importer.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 13 Dec 2009
 *
 * Copyright (C) 2009-2011  Urs Fleisch
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

#include "amazonimporter.h"
#include <QRegExp>
#include <QDomDocument>
#include "trackdatamodel.h"
#include "configstore.h"
#include "qtcompatmac.h"

/**
 * Constructor.
 *
 * @param parent          parent object
 * @param trackDataModel track data to be filled with imported values
 */
AmazonImporter::AmazonImporter(
  QObject* parent,
  TrackDataModel* trackDataModel)
  : ServerImporter(parent, trackDataModel)
{
  setObjectName("AmazonImporter");
}

/**
 * Destructor.
 */
AmazonImporter::~AmazonImporter()
{
}

/**
 * Name of import source.
 * @return name.
 */
const char* AmazonImporter::name() const { return I18N_NOOP("Amazon"); }

/** NULL-terminated array of server strings, 0 if not used */
const char** AmazonImporter::serverList() const
{
  static const char* servers[] = {
    // Parsing only works with English text
    "www.amazon.com:80",
    "www.amazon.co.uk:80",
    0                  // end of StrList
  };
  return servers;
}

/** default server, 0 to disable */
const char* AmazonImporter::defaultServer() const { return "www.amazon.com:80"; }

/** anchor to online help, 0 to disable */
const char* AmazonImporter::helpAnchor() const { return "import-amazon"; }

/** configuration, 0 if not used */
ServerImporterConfig* AmazonImporter::config() const { return &ConfigStore::s_amazonCfg; }

/** additional tags option, false if not used */
bool AmazonImporter::additionalTags() const { return true; }

/**
 * Process finished findCddbAlbum request.
 *
 * @param searchStr search data received
 */
void AmazonImporter::parseFindResults(const QByteArray& searchStr)
{
  /* products have the following format (depending on browser):
<td class="dataColumn"><table cellpadding="0" cellspacing="0" border="0"><tr><td>
<a href="http://www.amazon.com/Avenger-Amon-Amarth/dp/B001VROVHO/ref=sr_1_1/178-1209985-8853325?ie=UTF8&s=music&qid=1260707733&sr=1-1"><span class="srTitle">The Avenger</span></a>
   by <a href="/Amon-Amarth/e/B000APIBHO/ref=sr_ntt_srch_lnk1/178-1209985-8853325?_encoding=UTF8&amp;qid=1260707733&amp;sr=1-1">Amon Amarth</a> <span class="bindingBlock">(<span class="binding">Audio CD</span> - 2009)</span> - <span class="formatText">Original recording reissued</span></td></tr>
<td></td>
     or:
<div class="productTitle"><a href="http://www.amazon.com/Avenger-Amon-Amarth/dp/B001VROVHO/ref=sr_1_1?ie=UTF8&s=music&qid=1260607141&sr=1-1"> The Avenger</a> <span class="ptBrand">by <a href="/Amon-Amarth/e/B000APIBHO/ref=sr_ntt_srch_lnk_1?_encoding=UTF8&amp;qid=1260607141&amp;sr=1-1">Amon Amarth</a></span><span class="binding"> (<span class="format">Audio CD</span> - 2009)</span> - <span class="format">Original recording reissued</span></div>
   */
  QString str = QString::fromLatin1(searchStr);
  QRegExp catIdTitleArtistRe(
    "<a href=\"[^\"]+/(dp|ASIN|images|product|-)/([A-Z0-9]+)[^\"]+\">"
    "<span class=\"srTitle\">([^<]+)<.*>\\s*by\\s*(?:<[^>]+>)?([^<]+)<");
  QStringList lines = str.remove('\r').split(QRegExp("\\n{2,}"));
  m_albumListModel->clear();
  for (QStringList::const_iterator it = lines.begin(); it != lines.end(); ++it) {
    QString line(*it);
    line.remove('\n');
    if (catIdTitleArtistRe.indexIn(line) != -1) {
      m_albumListModel->appendRow(new AlbumListItem(
        removeHtml(catIdTitleArtistRe.cap(4)) + " - " +
        removeHtml(catIdTitleArtistRe.cap(3)),
        catIdTitleArtistRe.cap(1),
        catIdTitleArtistRe.cap(2)));
    }
  }
}

/**
 * Parse result of album request and populate m_trackDataModel with results.
 *
 * @param albumStr album data received
 */
void AmazonImporter::parseAlbumResults(const QByteArray& albumStr)
{
  /*
    title (empty lines removed):
<div class="buying"><h1 class="parseasinTitle"><span id="btAsinTitle" style="">Avenger</span></h1>
<span >
<a href="/Amon-Amarth/e/B000APIBHO/ref=ntt_mus_dp_pel">Amon Amarth</a>
</span>
</div>

    details (empty lines removed):
<a name="productDetails" id="productDetails"></a>
<hr noshade="noshade" size="1" class="bucketDivider" />
<table cellpadding="0" cellspacing="0" border="0">
  <tr>
    <td class="bucket">
<h2>Product Details</h2>
  <div class="content">
<ul>
<li><b>Audio CD</b>  (November 2, 1999)</li>
<li><b>Original Release Date:</b> November 2, 1999</li>
<li><b>Number of Discs:</b> 1</li>
<li><b>Label:</b> Metal Blade</li>

    tracks:
<tr class='rowEven'><td class="playCol"><a href="/gp/dmusic/media/sample.m3u/ref=dm_mu_dp_trk1_smpl/175-1810673-7649752?ie=UTF8&catalogItemType=track&ASIN=B0016OAHCK&DownloadLocation=CD" onclick='return cd_trackPreviewPressed("B0016OAHCK");'><img src="http://g-ecx.images-amazon.com/images/G/01/digital/music/dp/play-control-2._V223646478_.gif" width="19" alt="listen" id="cd_trackPreviewB0016OAHCK" title="listen" height="19" border="0" /></a></td><td class="titleCol">&nbsp; 1. <a href="http://www.amazon.com/gp/product/B0016OAHCK/ref=dm_mu_dp_trk1/175-1810673-7649752">Bleed For Ancient Gods</a></td><td class="runtimeCol"> 4:31</td><td class="priceCol">$0.99</td><td class="buyCol">

    alternatively (empty lines removed):
<tr class="listRowEven">
<td>
1. Before the Devil Knows You're Dead
</td>
   */
  QString str = QString::fromLatin1(albumStr);
  FrameCollection framesHdr;
  // search for 'id="btAsinTitle"', text after '>' until ' [' or '<' => album
  int end = 0;
  int start = str.indexOf("id=\"btAsinTitle\"");
  if (start >= 0) {
    start = str.indexOf(">", start);
    if (start >= 0) {
      end = str.indexOf("<", start);
      if (end > start) {
        int bracketPos = str.indexOf(" [", start);
        if (bracketPos >= 0 && bracketPos < end) {
          end = bracketPos;
        }
        framesHdr.setAlbum(
          replaceHtmlEntities(str.mid(start + 1, end - start - 1)));

        // next '<a href=', text after '>' until '<' => artist
        start = str.indexOf("<a href=", end);
        if (start >= 0) {
          start = str.indexOf(">", start);
          if (start >= 0) {
            end = str.indexOf("<", start);
            if (end > start) {
              framesHdr.setArtist(
                replaceHtmlEntities(str.mid(start + 1, end - start - 1)));
            }
          }
        }
      }
    }
  }

  // search for >Product Details<, >Original Release Date:<, >Label:<
  const bool additionalTags = getAdditionalTags();
  QString albumArtist;
  start = str.indexOf(">Product Details<");
  if (start >= 0) {
    int detailStart = str.indexOf(">Original Release Date:<", start);
    if (detailStart < 0) {
      detailStart  = str.indexOf(">Audio CD<", start);
    }
    if (detailStart >= 0) {
      int detailEnd = str.indexOf("\n", detailStart + 10);
      if (detailEnd > detailStart + 10) {
        QRegExp yearRe("(\\d{4})");
        if (yearRe.indexIn(
              str.mid(detailStart + 10, detailEnd - detailStart - 11)) >= 0) {
          framesHdr.setYear(yearRe.cap(1).toInt());
        }
      }
    }
    if (additionalTags) {
      detailStart = str.indexOf(">Label:<", start);
      if (detailStart > 0) {
        int detailEnd = str.indexOf("\n", detailStart + 8);
        if (detailEnd > detailStart + 8) {
          QRegExp labelRe(">\\s*([^<]+)<");
          if (labelRe.indexIn(
                str.mid(detailStart + 8, detailEnd - detailStart - 9)) >= 0) {
            framesHdr.setValue(Frame::FT_Publisher, removeHtml(labelRe.cap(1)));
          }
        }
      }
      detailStart = str.indexOf(">Performer:<", start);
      if (detailStart > 0) {
        int detailEnd = str.indexOf("</li>", detailStart + 12);
        if (detailEnd > detailStart + 12) {
          framesHdr.setValue(
            Frame::FT_Performer,
            removeHtml(str.mid(detailStart + 11, detailEnd - detailStart - 11)));
        }
      }
      detailStart = str.indexOf(">Orchestra:<", start);
      if (detailStart > 0) {
        int detailEnd = str.indexOf("</li>", detailStart + 12);
        if (detailEnd > detailStart + 12) {
          albumArtist =
            removeHtml(str.mid(detailStart + 11, detailEnd - detailStart - 11));
        }
      }
      detailStart = str.indexOf(">Conductor:<", start);
      if (detailStart > 0) {
        int detailEnd = str.indexOf("</li>", detailStart + 12);
        if (detailEnd > detailStart + 12) {
          framesHdr.setValue(
            Frame::FT_Conductor,
            removeHtml(str.mid(detailStart + 11, detailEnd - detailStart - 11)));
        }
      }
      detailStart = str.indexOf(">Composer:<", start);
      if (detailStart > 0) {
        int detailEnd = str.indexOf("</li>", detailStart + 11);
        if (detailEnd > detailStart + 11) {
          framesHdr.setValue(
            Frame::FT_Composer,
            removeHtml(str.mid(detailStart + 10, detailEnd - detailStart - 10)));
        }
      }
    }
  }

  ImportTrackDataVector trackDataVector(m_trackDataModel->getTrackData());
  if (getCoverArt()) {
    // <input type="hidden" id="ASIN" name="ASIN" value="B0025AY48W" />
    start = str.indexOf("id=\"ASIN\"");
    if (start > 0) {
      start = str.indexOf("value=\"", start);
      if (start > 0) {
        end = str.indexOf("\"", start + 7);
        if (end > start) {
          trackDataVector.setCoverArtUrl(
            QString("http://www.amazon.com/dp/") +
            str.mid(start + 7, end - start - 7));
        }
      }
    }
  }

  bool hasTitleCol = false;
  bool hasArtist = str.indexOf("<td>Song Title</td><td>Artist</td>") != -1;
  // search 'class="titleCol"', next '<a href=', text after '>' until '<'
  // => title
  // if not found: alternatively look for 'class="listRow'
  start = str.indexOf("class=\"titleCol\"");
  if (start >= 0) {
    hasTitleCol = true;
  } else {
    start = str.indexOf("class=\"listRow");
  }
  if (start >= 0) {
    QRegExp durationRe("(\\d+):(\\d+)");
    QRegExp nrTitleRe("\\s*\\d+\\.\\s+(.*\\S)");
    FrameCollection frames(framesHdr);
    ImportTrackDataVector::iterator it = trackDataVector.begin();
    bool atTrackDataListEnd = (it == trackDataVector.end());
    int trackNr = 1;
    while (start >= 0) {
      QString title;
      QString artist;
      int duration = 0;
      if (hasTitleCol) {
        end = str.indexOf("\n", start);
        if (end > start) {
          QString line = str.mid(start, end - start);
          int titleStart = line.indexOf("<a href=");
          if (titleStart >= 0) {
            titleStart = line.indexOf(">", titleStart);
            if (titleStart >= 0) {
              int titleEnd = line.indexOf("<", titleStart);
              if (titleEnd > titleStart) {
                title = line.mid(titleStart + 1, titleEnd - titleStart - 1);
                // if there was an Artist title,
                // search for artist in a second titleCol
                if (hasArtist) {
                  int artistStart =
                    line.indexOf("class=\"titleCol\"", titleEnd);
                  if (artistStart >= 0) {
                    artistStart = line.indexOf("<a href=", artistStart);
                    if (artistStart >= 0) {
                      artistStart = line.indexOf(">", artistStart);
                      if (artistStart >= 0) {
                        int artistEnd = line.indexOf("<", artistStart);
                        if (artistEnd > artistStart) {
                          artist = line.mid(
                            artistStart + 1, artistEnd - artistStart - 1);
                          if (albumArtist.isEmpty()) {
                            albumArtist = frames.getArtist();
                          }
                        }
                      }
                    }
                  }
                }
                // search for next 'class="', if it is 'class="runtimeCol"',
                // text after '>' until '<' => duration
                int runtimeStart =
                  line.indexOf("class=\"runtimeCol\"", titleEnd);
                if (runtimeStart >= 0) {
                  runtimeStart = line.indexOf(">", runtimeStart + 18);
                  if (runtimeStart >= 0) {
                    int runtimeEnd = line.indexOf("<", runtimeStart);
                    if (runtimeEnd > runtimeStart) {
                      if (durationRe.indexIn(
                            line.mid(runtimeStart + 1,
                                     runtimeEnd - runtimeStart - 1)) >= 0) {
                        duration = durationRe.cap(1).toInt() * 60 +
                          durationRe.cap(2).toInt();
                      }
                    }
                  }
                }
                start = str.indexOf("class=\"titleCol\"", end);
              } else {
                start = -1;
              }
            }
          }
        }
      } else {
        // 'class="listRow' found
        start = str.indexOf("<td>", start);
        if (start >= 0) {
          end = str.indexOf("</td>", start);
          if (end > start &&
              nrTitleRe.indexIn(str.mid(start + 4, end - start - 4)) >= 0) {
            title = nrTitleRe.cap(1);
            start = str.indexOf("class=\"listRow", end);
          } else {
            start = -1;
          }
        }
      }
      if (!title.isEmpty()) {
        frames.setTitle(replaceHtmlEntities(title));
        if (!artist.isEmpty()) {
          frames.setArtist(replaceHtmlEntities(artist));
        }
        if (!albumArtist.isEmpty() && additionalTags) {
          frames.setValue(Frame::FT_AlbumArtist, albumArtist);
        }
        frames.setTrack(trackNr);
        if (atTrackDataListEnd) {
          ImportTrackData trackData;
          trackData.setFrameCollection(frames);
          trackData.setImportDuration(duration);
          trackDataVector.push_back(trackData);
        } else {
          while (!atTrackDataListEnd && !it->isEnabled()) {
            ++it;
            atTrackDataListEnd = (it == trackDataVector.end());
          }
          if (!atTrackDataListEnd) {
            (*it).setFrameCollection(frames);
            (*it).setImportDuration(duration);
            ++it;
            atTrackDataListEnd = (it == trackDataVector.end());
          }
        }
        ++trackNr;
        frames = framesHdr;
      }
    }

    // handle redundant tracks
    frames.clear();
    while (!atTrackDataListEnd) {
      if (it->isEnabled()) {
        if ((*it).getFileDuration() == 0) {
          it = trackDataVector.erase(it);
        } else {
          (*it).setFrameCollection(frames);
          (*it).setImportDuration(0);
          ++it;
        }
      } else {
        ++it;
      }
      atTrackDataListEnd = (it == trackDataVector.end());
    }
  } else if (!framesHdr.empty()) {
    // if there are no track data, fill frame header data
    for (ImportTrackDataVector::iterator it = trackDataVector.begin();
         it != trackDataVector.end();
         ++it) {
      if (it->isEnabled()) {
        (*it).setFrameCollection(framesHdr);
      }
    }
  }
  m_trackDataModel->setTrackData(trackDataVector);
}

/**
 * Send a query command to search on the server.
 *
 * @param cfg      import source configuration
 * @param artist   artist to search
 * @param album    album to search
 */
void AmazonImporter::sendFindQuery(
  const ServerImporterConfig* cfg,
  const QString& artist, const QString& album)
{
  /*
   * Query looks like this:
   * http://www.amazon.com/gp/search/ref=sr_adv_m_pop/?search-alias=popular&field-artist=amon+amarth&field-title=the+avenger
   */
  sendRequest(cfg->m_server,
              QString("/gp/search/ref=sr_adv_m_pop/"
                      "?search-alias=popular&field-artist=") +
              encodeUrlQuery(artist) + "&field-title=" + encodeUrlQuery(album));
}

/**
 * Send a query command to fetch the track list
 * from the server.
 *
 * @param cfg      import source configuration
 * @param cat      category
 * @param id       ID
 */
void AmazonImporter::sendTrackListQuery(
  const ServerImporterConfig* cfg, const QString& cat, const QString& id)
{
  /*
   * Query looks like this:
   * http://www.amazon.com/dp/B001VROVHO
   */
  sendRequest(cfg->m_server, QString("/") + cat + '/' + id);
}
