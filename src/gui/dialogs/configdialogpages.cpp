/**
 * \file configdialogpages.cpp
 * Pages for configuration dialog.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 17 Sep 2003
 *
 * Copyright (C) 2003-2018  Urs Fleisch
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

#include "configdialogpages.h"
#include <QLayout>
#include <QPushButton>
#include <QLabel>
#include <QString>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTabWidget>
#include <QListView>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QStringListModel>
#include <QStandardItemModel>
#include "formatconfig.h"
#include "filenameformatbox.h"
#include "tagformatbox.h"
#include "tablemodeledit.h"
#include "tagconfig.h"
#include "fileconfig.h"
#include "useractionsconfig.h"
#include "guiconfig.h"
#include "networkconfig.h"
#include "importconfig.h"
#include "stringlistedit.h"
#include "configtable.h"
#include "commandstablemodel.h"
#include "checkablestringlistmodel.h"
#include "starratingmappingsmodel.h"
#include "fileproxymodel.h"
#include "contexthelp.h"
#include "frame.h"
#include "iplatformtools.h"

namespace {

/**
 * @brief Get string representation of folder patterns.
 * @param folders folder patterns
 * @param asteriskIfEmpty if true "*" is returned if @a folders is empty
 * @return string representation of folder patterns.
 */
QString folderPatternListToString(const QStringList& folders,
                                  bool asteriskIfEmpty = false)
{
  QStringList patterns;
  QChar sep = QLatin1Char(' ');
  foreach (const QString& folder, folders) {
    QString pattern = folder.trimmed();
    if (!pattern.isEmpty()) {
      if (pattern.contains(QLatin1Char(' '))) {
        sep = QLatin1Char(';');
      }
      patterns.append(pattern);
    }
  }
  patterns.removeDuplicates();
  if (patterns.isEmpty())
    return QLatin1String(asteriskIfEmpty ? "*" : "");

  // Keep semicolon to mark that space is not a separator.
  if (patterns.size() == 1 && sep == QLatin1Char(';'))
    return patterns.first() + sep;

  return patterns.join(sep);
}

/**
 * @brief Get folder patterns from string representation.
 * @param patterns string representation of folder patterns
 * @param asteriskIfEmpty if true an empty list is returned for "*"
 * @return folder pattern list.
 */
QStringList folderPatternListFromString(const QString& patterns,
                                        bool asteriskIfEmpty = false)
{
  if (asteriskIfEmpty && patterns == QLatin1String("*"))
    return QStringList();

  QStringList folders;
  const QChar sep = patterns.contains(QLatin1Char(';'))
      ? QLatin1Char(';') : QLatin1Char(' ');
  foreach (const QString& pattern, patterns.split(sep)) {
    QString folder = pattern.trimmed();
    if (!folder.isEmpty()) {
      folders.append(folder);
    }
  }

  return folders;
}

}

/**
 * Constructor.
 */
ConfigDialogPages::ConfigDialogPages(IPlatformTools* platformTools,
                                     QObject* parent) : QObject(parent),
  m_platformTools(platformTools),
  m_loadLastOpenedFileCheckBox(0), m_preserveTimeCheckBox(0),
  m_markChangesCheckBox(0), m_coverFileNameLineEdit(0),
  m_nameFilterComboBox(0), m_includeFoldersLineEdit(0),
  m_excludeFoldersLineEdit(0), m_showHiddenFilesCheckBox(0),
  m_fileTextEncodingComboBox(0),
  m_markTruncationsCheckBox(0), m_textEncodingV1ComboBox(0),
  m_totalNumTracksCheckBox(0), m_commentNameComboBox(0),
  m_pictureNameComboBox(0), m_markOversizedPicturesCheckBox(0),
  m_maximumPictureSizeSpinBox(0), m_genreNotNumericCheckBox(0),
  m_lowercaseId3ChunkCheckBox(0), m_markStandardViolationsCheckBox(0),
  m_textEncodingComboBox(0), m_id3v2VersionComboBox(0),
  m_trackNumberDigitsSpinBox(0), m_fnFormatBox(0), m_tagFormatBox(0),
  m_onlyCustomGenresCheckBox(0), m_genresEditModel(0),
  m_quickAccessTagsModel(0), m_starRatingMappingsModel(0),
  m_trackNameComboBox(0), m_playOnDoubleClickCheckBox(0),
  m_commandsTable(0), m_commandsTableModel(0), m_browserLineEdit(0),
  m_proxyCheckBox(0), m_proxyLineEdit(0), m_proxyAuthenticationCheckBox(0),
  m_proxyUserNameLineEdit(0), m_proxyPasswordLineEdit(0),
  m_enabledMetadataPluginsModel(0), m_enabledPluginsModel(0)
{
}

/**
 * Destructor.
 */
ConfigDialogPages::~ConfigDialogPages()
{
}

/**
 * Create page with tags settings.
 * @return tags page.
 */
QWidget* ConfigDialogPages::createTagsPage()
{
  QWidget* tagsPage = new QWidget;
  QVBoxLayout* vlayout = new QVBoxLayout(tagsPage);

  QWidget* tag1Page = new QWidget;
  QVBoxLayout* tag1Layout = new QVBoxLayout(tag1Page);
  QGroupBox* v1GroupBox = new QGroupBox(tr("ID3v1"), tag1Page);
  QGridLayout* v1GroupBoxLayout = new QGridLayout(v1GroupBox);
  m_markTruncationsCheckBox = new QCheckBox(tr("&Mark truncated fields"), v1GroupBox);
  v1GroupBoxLayout->addWidget(m_markTruncationsCheckBox, 0, 0, 1, 2);
  QLabel* textEncodingV1Label = new QLabel(tr("Text &encoding:"), v1GroupBox);
  m_textEncodingV1ComboBox = new QComboBox(v1GroupBox);
  m_textEncodingV1ComboBox->addItems(TagConfig::getTextCodecNames());
  m_textEncodingV1ComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
  textEncodingV1Label->setBuddy(m_textEncodingV1ComboBox);
  v1GroupBoxLayout->addWidget(textEncodingV1Label, 1, 0);
  v1GroupBoxLayout->addWidget(m_textEncodingV1ComboBox, 1, 1);
  tag1Layout->addWidget(v1GroupBox);
  tag1Layout->addStretch();

  QWidget* tag2Page = new QWidget;
  QHBoxLayout* tag2Layout = new QHBoxLayout(tag2Page);
  QVBoxLayout* tag2LeftLayout = new QVBoxLayout;
  QGroupBox* v2GroupBox = new QGroupBox(tr("ID3v2"), tag2Page);
  QGridLayout* v2GroupBoxLayout = new QGridLayout(v2GroupBox);
  m_totalNumTracksCheckBox = new QCheckBox(tr("Use &track/total number of tracks format"), v2GroupBox);
  v2GroupBoxLayout->addWidget(m_totalNumTracksCheckBox, 0, 0, 1, 2);
  QLabel* trackNumberDigitsLabel = new QLabel(tr("Track number &digits:"), v2GroupBox);
  m_trackNumberDigitsSpinBox = new QSpinBox(v2GroupBox);
  m_trackNumberDigitsSpinBox->setMaximum(5);
  m_genreNotNumericCheckBox = new QCheckBox(tr("&Genre as text instead of numeric string"), v2GroupBox);
  m_lowercaseId3ChunkCheckBox = new QCheckBox(tr("&WAV files with lowercase id3 chunk"), v2GroupBox);
  m_markStandardViolationsCheckBox = new QCheckBox(tr("Mar&k standard violations"));
  QLabel* textEncodingLabel = new QLabel(tr("Text &encoding:"), v2GroupBox);
  m_textEncodingComboBox = new QComboBox(v2GroupBox);
  m_textEncodingComboBox->addItems(TagConfig::getTextEncodingNames());
  m_textEncodingComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
  textEncodingLabel->setBuddy(m_textEncodingComboBox);
  v2GroupBoxLayout->addWidget(m_genreNotNumericCheckBox, 1, 0, 1, 2);
  v2GroupBoxLayout->addWidget(m_lowercaseId3ChunkCheckBox, 2, 0, 1, 2);
  v2GroupBoxLayout->addWidget(m_markStandardViolationsCheckBox, 3, 0, 1, 2);
  v2GroupBoxLayout->addWidget(textEncodingLabel, 4, 0);
  v2GroupBoxLayout->addWidget(m_textEncodingComboBox, 4, 1);
  const TagConfig& tagCfg = TagConfig::instance();
  if (!(tagCfg.taggedFileFeatures() &
        (TaggedFile::TF_ID3v22 | TaggedFile::TF_ID3v23 | TaggedFile::TF_ID3v24))) {
    m_genreNotNumericCheckBox->hide();
    textEncodingLabel->hide();
    m_textEncodingComboBox->hide();
  }
  QLabel* id3v2VersionLabel = new QLabel(tr("&Version used for new tags:"), v2GroupBox);
  m_id3v2VersionComboBox = new QComboBox(v2GroupBox);
  if (tagCfg.taggedFileFeatures() & TaggedFile::TF_ID3v23)
    m_id3v2VersionComboBox->addItem(tr("ID3v2.3.0"), TagConfig::ID3v2_3_0);
  if (tagCfg.taggedFileFeatures() & TaggedFile::TF_ID3v24)
    m_id3v2VersionComboBox->addItem(tr("ID3v2.4.0"), TagConfig::ID3v2_4_0);
  m_id3v2VersionComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
  id3v2VersionLabel->setBuddy(m_id3v2VersionComboBox);
  v2GroupBoxLayout->addWidget(id3v2VersionLabel, 5, 0);
  v2GroupBoxLayout->addWidget(m_id3v2VersionComboBox, 5, 1);
  if (m_id3v2VersionComboBox->count() < 2) {
    id3v2VersionLabel->hide();
    m_id3v2VersionComboBox->hide();
  }
  trackNumberDigitsLabel->setBuddy(m_trackNumberDigitsSpinBox);
  v2GroupBoxLayout->addWidget(trackNumberDigitsLabel, 6, 0);
  v2GroupBoxLayout->addWidget(m_trackNumberDigitsSpinBox, 6, 1);
  tag2LeftLayout->addWidget(v2GroupBox);
  QGroupBox* vorbisGroupBox = new QGroupBox(tr("Ogg/Vorbis"), tag2Page);
  QLabel* commentNameLabel = new QLabel(tr("Co&mment field name:"), vorbisGroupBox);
  m_commentNameComboBox = new QComboBox(vorbisGroupBox);
  QLabel* pictureNameLabel = new QLabel(tr("&Picture field name:"), vorbisGroupBox);
  m_pictureNameComboBox = new QComboBox(vorbisGroupBox);
  m_commentNameComboBox->setEditable(true);
  m_commentNameComboBox->addItems(TagConfig::getCommentNames());
  m_commentNameComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
  commentNameLabel->setBuddy(m_commentNameComboBox);
  m_pictureNameComboBox->addItems(TagConfig::getPictureNames());
  m_pictureNameComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
  pictureNameLabel->setBuddy(m_pictureNameComboBox);
  QGridLayout* vorbisGroupBoxLayout = new QGridLayout(vorbisGroupBox);
  vorbisGroupBoxLayout->addWidget(commentNameLabel, 0, 0);
  vorbisGroupBoxLayout->addWidget(m_commentNameComboBox, 0, 1);
  vorbisGroupBoxLayout->addWidget(pictureNameLabel, 1, 0);
  vorbisGroupBoxLayout->addWidget(m_pictureNameComboBox, 1, 1);
  vorbisGroupBox->setLayout(vorbisGroupBoxLayout);
  tag2LeftLayout->addWidget(vorbisGroupBox);
  if (!(tagCfg.taggedFileFeatures() & TaggedFile::TF_OggPictures)) {
    vorbisGroupBox->hide();
  }
  QGroupBox* pictureGroupBox = new QGroupBox(tr("Picture"), tag2Page);
  QHBoxLayout* pictureGroupBoxLayout = new QHBoxLayout(pictureGroupBox);
  m_markOversizedPicturesCheckBox =
      new QCheckBox(tr("Mark if &larger than (bytes):"));
  m_maximumPictureSizeSpinBox = new QSpinBox;
  m_maximumPictureSizeSpinBox->setRange(0, INT_MAX);
  pictureGroupBoxLayout->addWidget(m_markOversizedPicturesCheckBox);
  pictureGroupBoxLayout->addWidget(m_maximumPictureSizeSpinBox);
  tag2LeftLayout->addWidget(pictureGroupBox);
  tag2LeftLayout->addStretch();
  tag2Layout->addLayout(tag2LeftLayout);

  QVBoxLayout* tag2RightLayout = new QVBoxLayout;
  QGroupBox* genresGroupBox = new QGroupBox(tr("Custom &Genres"), tag2Page);
  m_onlyCustomGenresCheckBox = new QCheckBox(tr("&Show only custom genres"), genresGroupBox);
  m_genresEditModel = new QStringListModel(genresGroupBox);
  StringListEdit* genresEdit = new StringListEdit(m_genresEditModel, genresGroupBox);
  QVBoxLayout* vbox = new QVBoxLayout;
  vbox->addWidget(m_onlyCustomGenresCheckBox);
  vbox->addWidget(genresEdit);
  genresGroupBox->setLayout(vbox);
  tag2RightLayout->addWidget(genresGroupBox);

  QGroupBox* quickAccessTagsGroupBox = new QGroupBox(tr("&Quick Access Frames"));
  QVBoxLayout* quickAccessTagsLayout = new QVBoxLayout(quickAccessTagsGroupBox);
  QListView* quickAccessTagsListView = new QListView;
  m_quickAccessTagsModel = new QStandardItemModel(quickAccessTagsGroupBox);
  quickAccessTagsListView->setModel(m_quickAccessTagsModel);
  quickAccessTagsListView->setAcceptDrops(true);
  quickAccessTagsListView->setDragEnabled(true);
  quickAccessTagsListView->setDragDropMode(QAbstractItemView::InternalMove);
  quickAccessTagsListView->setDragDropOverwriteMode(false);
  quickAccessTagsListView->setDefaultDropAction(Qt::MoveAction);
  quickAccessTagsListView->setDropIndicatorShown(true);
  quickAccessTagsLayout->addWidget(quickAccessTagsListView);
  QLabel* reorderLabel =
      new QLabel(tr("Use drag and drop to reorder the items"));
  reorderLabel->setWordWrap(true);
  quickAccessTagsLayout->addWidget(reorderLabel);
  tag2RightLayout->addWidget(quickAccessTagsGroupBox);
  tag2Layout->addLayout(tag2RightLayout);

  QWidget* tag3Page = new QWidget;
  QVBoxLayout* tag3Layout = new QVBoxLayout(tag3Page);
  QGroupBox* riffGroupBox = new QGroupBox(tr("RIFF INFO"), tag3Page);
  QLabel* trackNameLabel = new QLabel(tr("Track nu&mber field name:"), riffGroupBox);
  m_trackNameComboBox = new QComboBox(riffGroupBox);
  m_trackNameComboBox->setEditable(true);
  m_trackNameComboBox->addItems(TagConfig::getRiffTrackNames());
  m_trackNameComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
  trackNameLabel->setBuddy(m_trackNameComboBox);
  QGridLayout* riffGroupBoxLayout = new QGridLayout(riffGroupBox);
  riffGroupBoxLayout->addWidget(trackNameLabel, 0, 0);
  riffGroupBoxLayout->addWidget(m_trackNameComboBox, 0, 1);
  riffGroupBox->setLayout(riffGroupBoxLayout);
  tag3Layout->addWidget(riffGroupBox);
  tag3Layout->addStretch();

  QWidget* tag1AndTag2Page = new QWidget;
  QVBoxLayout* tag1AndTag2Layout = new QVBoxLayout(tag1AndTag2Page);
  QString tagFormatTitle(tr("&Tag Format"));
  m_tagFormatBox = new TagFormatBox(tagFormatTitle, tag1AndTag2Page);
  QGroupBox* ratingGroupBox = new QGroupBox(tr("Rating"), tag1AndTag2Page);
  QVBoxLayout* ratingLayout = new QVBoxLayout(ratingGroupBox);
  m_starRatingMappingsModel = new StarRatingMappingsModel(ratingGroupBox);
  TableModelEdit* ratingEdit = new TableModelEdit(m_starRatingMappingsModel);
  ratingLayout->addWidget(ratingEdit);
  tag1AndTag2Layout->addWidget(m_tagFormatBox);
  tag1AndTag2Layout->addWidget(ratingGroupBox);

  QTabWidget* tagsTabWidget = new QTabWidget;
  if (tagCfg.taggedFileFeatures() & TaggedFile::TF_ID3v11) {
    tagsTabWidget->addTab(tag1Page, tr("Tag &1"));
  }
  tagsTabWidget->addTab(tag2Page, tr("Tag &2"));
  tagsTabWidget->addTab(tag3Page, tr("Tag &3"));
  tagsTabWidget->addTab(tag1AndTag2Page, tr("All Ta&gs"));
  tagsTabWidget->setCurrentIndex(1);
  vlayout->addWidget(tagsTabWidget);
  return tagsPage;
}

/**
 * Create page with files settings.
 * @return files page.
 */
QWidget* ConfigDialogPages::createFilesPage()
{
  QWidget* filesPage = new QWidget;
  QVBoxLayout* vlayout = new QVBoxLayout(filesPage);
  QHBoxLayout* hlayout = new QHBoxLayout;
  QVBoxLayout* leftLayout = new QVBoxLayout;
  QVBoxLayout* rightLayout = new QVBoxLayout;

  QGroupBox* startupGroupBox = new QGroupBox(tr("Startup"), filesPage);
  m_loadLastOpenedFileCheckBox = new QCheckBox(tr("&Load last-opened files"),
                                               startupGroupBox);
  QVBoxLayout* startupLayout = new QVBoxLayout;
  startupLayout->addWidget(m_loadLastOpenedFileCheckBox);
  startupGroupBox->setLayout(startupLayout);
  leftLayout->addWidget(startupGroupBox);
  QGroupBox* saveGroupBox = new QGroupBox(tr("Save"), filesPage);
  m_preserveTimeCheckBox = new QCheckBox(tr("&Preserve file timestamp"), saveGroupBox);
  m_markChangesCheckBox = new QCheckBox(tr("&Mark changes"), saveGroupBox);
  m_coverFileNameLineEdit = new QLineEdit(saveGroupBox);
  m_fileTextEncodingComboBox = new QComboBox(saveGroupBox);
  m_fileTextEncodingComboBox->addItems(FileConfig::getTextCodecNames());
  m_fileTextEncodingComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                                        QSizePolicy::Minimum));
  QFormLayout* formLayout = new QFormLayout;
  formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  formLayout->addRow(m_preserveTimeCheckBox);
  formLayout->addRow(m_markChangesCheckBox);
  formLayout->addRow(tr("F&ilename for cover:"), m_coverFileNameLineEdit);
  formLayout->addRow(tr("Text &encoding (Export, Playlist):"),
                     m_fileTextEncodingComboBox);
  saveGroupBox->setLayout(formLayout);
  leftLayout->addWidget(saveGroupBox);

  QGroupBox* fileListGroupBox = new QGroupBox(tr("File List"), filesPage);
  QLabel* nameFilterLabel = new QLabel(tr("Filte&r:"), fileListGroupBox);
  m_nameFilterComboBox = new QComboBox(fileListGroupBox);
  m_nameFilterComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                                  QSizePolicy::Minimum));
  QList<QPair<QString, QString> > filters = FileProxyModel::createNameFilters();
  for (QList<QPair<QString, QString> >::const_iterator it = filters.constBegin();
       it != filters.constEnd();
       ++it) {
    QString nameFilter = m_platformTools->fileDialogNameFilter(
          QList<QPair<QString, QString> >() << *it);
#if QT_VERSION < 0x050000
    // Filters from the KDE 4 file dialog are not consistent, filter uses
    // something like "*.mp3|MP3 (*.mp3)", but the current filter returns only
    // something like "*.mp3".
    int pipeIndex = nameFilter.indexOf(QLatin1Char('|'));
    if (pipeIndex != -1) {
      nameFilter = nameFilter.left(pipeIndex);
    }
#endif
    m_nameFilterComboBox->addItem(it->first, nameFilter);
  }
  nameFilterLabel->setBuddy(m_nameFilterComboBox);
  QLabel* includeFoldersLabel = new QLabel(tr("Inclu&de folders:"),
                                           fileListGroupBox);
  m_includeFoldersLineEdit = new QLineEdit(fileListGroupBox);
  includeFoldersLabel->setBuddy(m_includeFoldersLineEdit);
  QLabel* excludeFoldersLabel = new QLabel(tr("E&xclude folders:"),
                                           fileListGroupBox);
  m_excludeFoldersLineEdit = new QLineEdit(fileListGroupBox);
  excludeFoldersLabel->setBuddy(m_excludeFoldersLineEdit);
  m_showHiddenFilesCheckBox = new QCheckBox(tr("&Show hidden files"),
                                            fileListGroupBox);
  QGridLayout* fileListGroupBoxLayout = new QGridLayout(fileListGroupBox);
  fileListGroupBoxLayout->addWidget(nameFilterLabel, 0, 0);
  fileListGroupBoxLayout->addWidget(m_nameFilterComboBox, 0, 1);
  fileListGroupBoxLayout->addWidget(includeFoldersLabel, 1, 0);
  fileListGroupBoxLayout->addWidget(m_includeFoldersLineEdit, 1, 1);
  fileListGroupBoxLayout->addWidget(excludeFoldersLabel, 2, 0);
  fileListGroupBoxLayout->addWidget(m_excludeFoldersLineEdit, 2, 1);
  fileListGroupBoxLayout->addWidget(m_showHiddenFilesCheckBox, 3, 0, 1, 2);
  rightLayout->addWidget(fileListGroupBox);
  rightLayout->addStretch();

  hlayout->addLayout(leftLayout);
  hlayout->addLayout(rightLayout);
  vlayout->addLayout(hlayout);

  QString fnFormatTitle(tr("&Filename Format"));
  m_fnFormatBox = new FilenameFormatBox(fnFormatTitle, filesPage);
  vlayout->addWidget(m_fnFormatBox);
  return filesPage;
}

/**
 * Create page with actions settings.
 * @return actions page.
 */
QWidget* ConfigDialogPages::createActionsPage()
{
  QWidget* actionsPage = new QWidget;
  QVBoxLayout* vlayout = new QVBoxLayout(actionsPage);
  QGroupBox* browserGroupBox = new QGroupBox(tr("Browser"), actionsPage);
  QLabel* browserLabel = new QLabel(tr("Web &browser:"), browserGroupBox);
  m_browserLineEdit = new QLineEdit(browserGroupBox);
  browserLabel->setBuddy(m_browserLineEdit);
  QHBoxLayout* hbox = new QHBoxLayout;
  hbox->addWidget(browserLabel);
  hbox->addWidget(m_browserLineEdit);
  browserGroupBox->setLayout(hbox);
  vlayout->addWidget(browserGroupBox);

  QGroupBox* commandsGroupBox = new QGroupBox(tr("Context &Menu Commands"), actionsPage);
  m_playOnDoubleClickCheckBox =
      new QCheckBox(tr("&Play on double click"), commandsGroupBox);
  m_commandsTableModel = new CommandsTableModel(commandsGroupBox);
  m_commandsTable = new ConfigTable(m_commandsTableModel, commandsGroupBox);
  m_commandsTable->setHorizontalResizeModes(
    m_commandsTableModel->getHorizontalResizeModes());
  QVBoxLayout* commandsLayout = new QVBoxLayout;
  commandsLayout->addWidget(m_playOnDoubleClickCheckBox);
  commandsLayout->addWidget(m_commandsTable);
  commandsGroupBox->setLayout(commandsLayout);
  vlayout->addWidget(commandsGroupBox);
  return actionsPage;
}

/**
 * Create page with network settings.
 * @return network page.
 */
QWidget* ConfigDialogPages::createNetworkPage()
{
  QWidget* networkPage = new QWidget;
  QVBoxLayout* vlayout = new QVBoxLayout(networkPage);
  QGroupBox* proxyGroupBox = new QGroupBox(tr("Proxy"), networkPage);
  m_proxyCheckBox = new QCheckBox(tr("&Proxy:"), proxyGroupBox);
  m_proxyLineEdit = new QLineEdit(proxyGroupBox);
  m_proxyAuthenticationCheckBox = new QCheckBox(tr("&Use authentication with proxy"), proxyGroupBox);
  QLabel* proxyUserNameLabel = new QLabel(tr("Proxy user &name:"), proxyGroupBox);
  m_proxyUserNameLineEdit = new QLineEdit(proxyGroupBox);
  proxyUserNameLabel->setBuddy(m_proxyUserNameLineEdit);
  QLabel* proxyPasswordLabel = new QLabel(tr("Proxy pass&word:"), proxyGroupBox);
  m_proxyPasswordLineEdit = new QLineEdit(proxyGroupBox);
  proxyPasswordLabel->setBuddy(m_proxyPasswordLineEdit);
  m_proxyPasswordLineEdit->setEchoMode(QLineEdit::Password);
  QVBoxLayout* vbox = new QVBoxLayout;
  QHBoxLayout* proxyHbox = new QHBoxLayout;
  proxyHbox->addWidget(m_proxyCheckBox);
  proxyHbox->addWidget(m_proxyLineEdit);
  vbox->addLayout(proxyHbox);
  vbox->addWidget(m_proxyAuthenticationCheckBox);
  QGridLayout* authLayout = new QGridLayout;
  authLayout->addWidget(proxyUserNameLabel, 0, 0);
  authLayout->addWidget(m_proxyUserNameLineEdit, 0, 1);
  authLayout->addWidget(proxyPasswordLabel, 1, 0);
  authLayout->addWidget(m_proxyPasswordLineEdit, 1, 1);
  vbox->addLayout(authLayout);
  proxyGroupBox->setLayout(vbox);
  vlayout->addWidget(proxyGroupBox);

  QSpacerItem* vspacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  vlayout->addItem(vspacer);
  return networkPage;
}

/**
 * Create page with plugins settings.
 * @return plugins page.
 */
QWidget* ConfigDialogPages::createPluginsPage()
{
  QWidget* pluginsPage = new QWidget;
  QVBoxLayout* vlayout = new QVBoxLayout(pluginsPage);
  QGroupBox* metadataGroupBox = new QGroupBox(
        tr("&Metadata Plugins && Priority"), pluginsPage);


  QVBoxLayout* metadataPluginsLayout = new QVBoxLayout(metadataGroupBox);
  m_enabledMetadataPluginsModel =
      new CheckableStringListModel(metadataGroupBox);
  StringListEdit* metadataEdit =
      new StringListEdit(m_enabledMetadataPluginsModel, metadataGroupBox);
  metadataEdit->setEditingDisabled(true);
  metadataPluginsLayout->addWidget(metadataEdit);
  vlayout->addWidget(metadataGroupBox);

  QGroupBox* pluginsGroupBox = new QGroupBox(tr("A&vailable Plugins"));
  QVBoxLayout* pluginsLayout = new QVBoxLayout(pluginsGroupBox);
  QListView* pluginsListView = new QListView;
  pluginsListView->setSelectionMode(QAbstractItemView::NoSelection);
  m_enabledPluginsModel = new CheckableStringListModel(pluginsGroupBox);
  pluginsListView->setModel(m_enabledPluginsModel);
  pluginsLayout->addWidget(pluginsListView);
  vlayout->addWidget(pluginsGroupBox);

  vlayout->addStretch();
  vlayout->addWidget(
        new QLabel(tr("Changes take only effect after a restart!")));
  return pluginsPage;
}

/**
 * Set values in pages from default configuration.
 */
void ConfigDialogPages::setDefaultConfig()
{
  FilenameFormatConfig fnCfg;
  TagFormatConfig id3Cfg;
  TagConfig tagCfg;
  tagCfg.setAvailablePlugins(TagConfig::instance().availablePlugins());
  tagCfg.setTaggedFileFeatures(TagConfig::instance().taggedFileFeatures());
  tagCfg.setDefaultPluginOrder();
  FileConfig fileCfg;
  UserActionsConfig userActionsCfg;
  userActionsCfg.setDefaultUserActions();
  GuiConfig guiCfg;
  NetworkConfig networkCfg;
  networkCfg.setDefaultBrowser();
  ImportConfig importCfg;
  importCfg.setAvailablePlugins(ImportConfig::instance().availablePlugins());
  setConfigs(fnCfg, id3Cfg, tagCfg, fileCfg, userActionsCfg, guiCfg, networkCfg,
            importCfg);
}

/**
 * Set values in dialog from current configuration.
 */
void ConfigDialogPages::setConfig()
{
  const FormatConfig& fnCfg = FilenameFormatConfig::instance();
  const FormatConfig& id3Cfg = TagFormatConfig::instance();
  const TagConfig& tagCfg = TagConfig::instance();
  const FileConfig& fileCfg = FileConfig::instance();
  const UserActionsConfig& userActionsCfg = UserActionsConfig::instance();
  const GuiConfig& guiCfg = GuiConfig::instance();
  const NetworkConfig& networkCfg = NetworkConfig::instance();
  const ImportConfig& importCfg = ImportConfig::instance();
  setConfigs(fnCfg, id3Cfg, tagCfg, fileCfg, userActionsCfg, guiCfg, networkCfg,
            importCfg);
}

/**
 * Set values in dialog from given configurations.
 */
void ConfigDialogPages::setConfigs(
    const FormatConfig& fnCfg, const FormatConfig& id3Cfg,
    const TagConfig& tagCfg, const FileConfig& fileCfg,
    const UserActionsConfig& userActionsCfg, const GuiConfig& guiCfg,
    const NetworkConfig& networkCfg, const ImportConfig& importCfg)
{
  m_fnFormatBox->fromFormatConfig(fnCfg);
  m_tagFormatBox->fromFormatConfig(id3Cfg);
  m_markTruncationsCheckBox->setChecked(tagCfg.markTruncations());
  m_totalNumTracksCheckBox->setChecked(tagCfg.enableTotalNumberOfTracks());
  m_loadLastOpenedFileCheckBox->setChecked(fileCfg.loadLastOpenedFile());
  m_preserveTimeCheckBox->setChecked(fileCfg.preserveTime());
  m_markChangesCheckBox->setChecked(fileCfg.markChanges());
  m_coverFileNameLineEdit->setText(fileCfg.defaultCoverFileName());
  m_nameFilterComboBox->setCurrentIndex(
        m_nameFilterComboBox->findData(fileCfg.nameFilter()));
  m_includeFoldersLineEdit->setText(
        folderPatternListToString(fileCfg.includeFolders(), true));
  m_excludeFoldersLineEdit->setText(
        folderPatternListToString(fileCfg.excludeFolders(), false));
  m_showHiddenFilesCheckBox->setChecked(fileCfg.showHiddenFiles());
  m_fileTextEncodingComboBox->setCurrentIndex(fileCfg.textEncodingIndex());
  m_onlyCustomGenresCheckBox->setChecked(tagCfg.onlyCustomGenres());
  m_genresEditModel->setStringList(tagCfg.customGenres());
  m_starRatingMappingsModel->setMappings(tagCfg.starRatingMappings());
  quint64 frameMask = tagCfg.quickAccessFrames();
  QList<int> frameTypes = tagCfg.quickAccessFrameOrder();
  if (frameTypes.size() != Frame::FT_LastFrame + 1) {
    frameTypes.clear();
    for (int i = Frame::FT_FirstFrame; i <= Frame::FT_LastFrame; ++i) {
      frameTypes.append(i);
    }
  }
  m_quickAccessTagsModel->clear();
  foreach (int frameType, frameTypes) {
    QStandardItem* item = new QStandardItem(
          Frame::ExtendedType(static_cast<Frame::Type>(frameType)).getTranslatedName());
    item->setData(frameType, Qt::UserRole);
    item->setCheckable(true);
    item->setCheckState((frameMask & (1ULL << frameType))
                        ? Qt::Checked : Qt::Unchecked);
    item->setDropEnabled(false);
    m_quickAccessTagsModel->appendRow(item);
  }
  m_commandsTableModel->setCommandList(userActionsCfg.contextMenuCommands());
  int idx = m_commentNameComboBox->findText(tagCfg.commentName());
  if (idx >= 0) {
    m_commentNameComboBox->setCurrentIndex(idx);
  } else {
    m_commentNameComboBox->addItem(tagCfg.commentName());
    m_commentNameComboBox->setCurrentIndex(m_commentNameComboBox->count() - 1);
  }
  m_pictureNameComboBox->setCurrentIndex(tagCfg.pictureNameIndex());
  m_genreNotNumericCheckBox->setChecked(tagCfg.genreNotNumeric());
  m_lowercaseId3ChunkCheckBox->setChecked(tagCfg.lowercaseId3RiffChunk());
  m_markStandardViolationsCheckBox->setChecked(tagCfg.markStandardViolations());
  m_textEncodingV1ComboBox->setCurrentIndex(tagCfg.textEncodingV1Index());
  m_textEncodingComboBox->setCurrentIndex(tagCfg.textEncoding());
  m_id3v2VersionComboBox->setCurrentIndex(
        m_id3v2VersionComboBox->findData(tagCfg.id3v2Version()));
  m_trackNumberDigitsSpinBox->setValue(tagCfg.trackNumberDigits());
  m_markOversizedPicturesCheckBox->setChecked(tagCfg.markOversizedPictures());
  m_maximumPictureSizeSpinBox->setValue(tagCfg.maximumPictureSize());
  idx = m_trackNameComboBox->findText(tagCfg.riffTrackName());
  if (idx >= 0) {
    m_trackNameComboBox->setCurrentIndex(idx);
  } else {
    m_trackNameComboBox->addItem(tagCfg.riffTrackName());
    m_trackNameComboBox->setCurrentIndex(m_trackNameComboBox->count() - 1);
  }
  m_browserLineEdit->setText(networkCfg.browser());
  m_playOnDoubleClickCheckBox->setChecked(guiCfg.playOnDoubleClick());
  m_proxyCheckBox->setChecked(networkCfg.useProxy());
  m_proxyLineEdit->setText(networkCfg.proxy());
  m_proxyAuthenticationCheckBox->setChecked(networkCfg.useProxyAuthentication());
  m_proxyUserNameLineEdit->setText(networkCfg.proxyUserName());
  m_proxyPasswordLineEdit->setText(networkCfg.proxyPassword());

  QStringList metadataPlugins;
  QStringList pluginOrder = tagCfg.pluginOrder();
  if (!pluginOrder.isEmpty()) {
    for (int i = 0; i < pluginOrder.size(); ++i) {
      metadataPlugins.append(QString());
    }
    foreach (const QString& pluginName, tagCfg.availablePlugins()) {
      int pluginIdx = pluginOrder.indexOf(pluginName);
      if (pluginIdx >= 0) {
        metadataPlugins[pluginIdx] = pluginName;
      } else {
        metadataPlugins.append(pluginName);
      }
    }
    metadataPlugins.removeAll(QString());
  } else {
    metadataPlugins = tagCfg.availablePlugins();
  }
  quint64 metadataPluginsMask = 0;
  quint64 mask = 1;
  QStringList disabledTagPlugins = tagCfg.disabledPlugins();
  for (int i = 0; i < metadataPlugins.size(); ++i, mask <<= 1) {
    if (!disabledTagPlugins.contains(metadataPlugins.at(i))) {
      metadataPluginsMask |= mask;
    }
  }
  m_enabledMetadataPluginsModel->setStringList(metadataPlugins);
  m_enabledMetadataPluginsModel->setBitMask(metadataPluginsMask);

  QStringList importPlugins = importCfg.availablePlugins();
  quint64 importPluginsMask = 0;
  mask = 1;
  QStringList disabledPlugins = importCfg.disabledPlugins();
  for (int i = 0; i < importPlugins.size(); ++i, mask <<= 1) {
    if (!disabledPlugins.contains(importPlugins.at(i))) {
      importPluginsMask |= mask;
    }
  }
  m_enabledPluginsModel->setStringList(importPlugins);
  m_enabledPluginsModel->setBitMask(importPluginsMask);

  if (!guiCfg.configWindowGeometry().isEmpty()) {
    if (QWidget* configDialog = qobject_cast<QWidget*>(parent())) {
      configDialog->restoreGeometry(guiCfg.configWindowGeometry());
    }
  }
}

/**
 * Get values from dialog and store them in the current configuration.
 */
void ConfigDialogPages::getConfig() const
{
  FormatConfig& fnCfg = FilenameFormatConfig::instance();
  FormatConfig& id3Cfg = TagFormatConfig::instance();
  TagConfig& tagCfg = TagConfig::instance();
  FileConfig& fileCfg = FileConfig::instance();
  UserActionsConfig& userActionsCfg = UserActionsConfig::instance();
  GuiConfig& guiCfg = GuiConfig::instance();
  NetworkConfig& networkCfg = NetworkConfig::instance();
  ImportConfig& importCfg = ImportConfig::instance();

  m_fnFormatBox->toFormatConfig(fnCfg);
  m_tagFormatBox->toFormatConfig(id3Cfg);
  tagCfg.setMarkTruncations(m_markTruncationsCheckBox->isChecked());
  tagCfg.setEnableTotalNumberOfTracks(m_totalNumTracksCheckBox->isChecked());
  fileCfg.setLoadLastOpenedFile(m_loadLastOpenedFileCheckBox->isChecked());
  fileCfg.setPreserveTime(m_preserveTimeCheckBox->isChecked());
  fileCfg.setMarkChanges(m_markChangesCheckBox->isChecked());
  fileCfg.setDefaultCoverFileName(m_coverFileNameLineEdit->text());
#if QT_VERSION >= 0x050200
  fileCfg.setNameFilter(m_nameFilterComboBox->currentData().toString());
#else
  fileCfg.setNameFilter(m_nameFilterComboBox->itemData(
                          m_nameFilterComboBox->currentIndex()).toString());
#endif
  fileCfg.setIncludeFolders(
        folderPatternListFromString(m_includeFoldersLineEdit->text(), true));
  fileCfg.setExcludeFolders(
        folderPatternListFromString(m_excludeFoldersLineEdit->text(), false));
  fileCfg.setShowHiddenFiles(m_showHiddenFilesCheckBox->isChecked());
  fileCfg.setTextEncodingIndex(m_fileTextEncodingComboBox->currentIndex());
  tagCfg.setOnlyCustomGenres(m_onlyCustomGenresCheckBox->isChecked());
  tagCfg.setCustomGenres(m_genresEditModel->stringList());
  tagCfg.setStarRatingMappings(m_starRatingMappingsModel->getMappings());
  QList<int> frameTypes;
  bool isStandardFrameOrder = true;
  quint64 frameMask = 0;
  for (int row = 0; row < m_quickAccessTagsModel->rowCount(); ++row) {
    QModelIndex index = m_quickAccessTagsModel->index(row, 0);
    int frameType = index.data(Qt::UserRole).toInt();
    if (frameType != row) {
      isStandardFrameOrder = false;
    }
    frameTypes.append(frameType);
    if (m_quickAccessTagsModel->data(
          index, Qt::CheckStateRole).toInt() == Qt::Checked) {
      frameMask |= 1ULL << frameType;
    }
  }
  if (isStandardFrameOrder) {
    frameTypes.clear();
  }
  tagCfg.setQuickAccessFrames(frameMask);
  tagCfg.setQuickAccessFrameOrder(frameTypes);
  userActionsCfg.setContextMenuCommands(m_commandsTableModel->getCommandList());
  tagCfg.setCommentName(m_commentNameComboBox->currentText());
  tagCfg.setPictureNameIndex(m_pictureNameComboBox->currentIndex());
  tagCfg.setGenreNotNumeric(m_genreNotNumericCheckBox->isChecked());
  tagCfg.setLowercaseId3RiffChunk(m_lowercaseId3ChunkCheckBox->isChecked());
  tagCfg.setMarkStandardViolations(m_markStandardViolationsCheckBox->isChecked());
  tagCfg.setTextEncodingV1Index(m_textEncodingV1ComboBox->currentIndex());
  tagCfg.setTextEncoding(m_textEncodingComboBox->currentIndex());
  tagCfg.setId3v2Version(m_id3v2VersionComboBox->itemData(
        m_id3v2VersionComboBox->currentIndex()).toInt());
  tagCfg.setTrackNumberDigits(m_trackNumberDigitsSpinBox->value());
  tagCfg.setMarkOversizedPictures(m_markOversizedPicturesCheckBox->isChecked());
  tagCfg.setMaximumPictureSize(m_maximumPictureSizeSpinBox->value());
  tagCfg.setRiffTrackName(m_trackNameComboBox->currentText());
  networkCfg.setBrowser(m_browserLineEdit->text());
  guiCfg.setPlayOnDoubleClick(m_playOnDoubleClickCheckBox->isChecked());
  networkCfg.setUseProxy(m_proxyCheckBox->isChecked());
  networkCfg.setProxy(m_proxyLineEdit->text());
  networkCfg.setUseProxyAuthentication(m_proxyAuthenticationCheckBox->isChecked());
  networkCfg.setProxyUserName(m_proxyUserNameLineEdit->text());
  networkCfg.setProxyPassword(m_proxyPasswordLineEdit->text());

  QStringList pluginOrder, disabledPlugins;
  for (int row = 0; row < m_enabledMetadataPluginsModel->rowCount(); ++row) {
    QString pluginName =
        m_enabledMetadataPluginsModel->index(row).data().toString();
    pluginOrder.append(pluginName);
    if (m_enabledMetadataPluginsModel->index(row).data(Qt::CheckStateRole).
        toInt() != Qt::Checked) {
      disabledPlugins.append(pluginName);
    }
  }
  tagCfg.setPluginOrder(pluginOrder);
  tagCfg.setDisabledPlugins(disabledPlugins);

  disabledPlugins.clear();
  for (int row = 0; row < m_enabledPluginsModel->rowCount(); ++row) {
    if (m_enabledPluginsModel->index(row).data(Qt::CheckStateRole).
        toInt() != Qt::Checked) {
      disabledPlugins.append(
            m_enabledPluginsModel->index(row).data().toString());
    }
  }
  importCfg.setDisabledPlugins(disabledPlugins);

  if (QWidget* configDialog = qobject_cast<QWidget*>(parent())) {
    guiCfg.setConfigWindowGeometry(configDialog->saveGeometry());
  }
}
