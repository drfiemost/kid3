/**
 * \file framelist.cpp
 * List of ID3v2.3 frames.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 9 Jan 2003
 */

#include "config.h"
#ifdef CONFIG_USE_KDE
#include <klocale.h>
#include <kdialogbase.h>
#include <kfiledialog.h>
#else
#include <qdialog.h>
#include <qfiledialog.h>
#define i18n(s) tr(s)
#define I18N_NOOP(s) QT_TR_NOOP(s)
#endif

#include <qfile.h>
#include <qdatastream.h>
#include <qlistbox.h>
#include <qimage.h>
#include <qpainter.h>
#include <qinputdialog.h>
#include <qcombobox.h>
#if defined WIN32 && defined _DEBUG
#include <id3.h> /* ID3TagIterator_Delete() */
#endif

#include "mp3file.h"
#include "framelist.h"

/**
 * Constructor.
 *
 * @param parent parent widget
 * @param name   internal name or 0
 */

LabeledTextEdit::LabeledTextEdit(QWidget *parent, const char *name) :
    QWidget(parent, name)
{
	layout = new QVBoxLayout(this);
	label = new QLabel(this);
	edit = new QTextEdit(this);
	if (layout && label && edit) {
		edit->setTextFormat(Qt::PlainText);
		layout->addWidget(label);
		layout->addWidget(edit);
	}
}

/**
 * Constructor.
 *
 * @param parent parent widget
 * @param name   internal name or 0
 */

LabeledLineEdit::LabeledLineEdit(QWidget *parent, const char *name) :
    QWidget(parent, name)
{
	layout = new QVBoxLayout(this);
	label = new QLabel(this);
	edit = new QLineEdit(this);
	if (layout && label && edit) {
		layout->addWidget(label);
		layout->addWidget(edit);
	}
}

/**
 * Constructor.
 *
 * @param parent parent widget
 * @param name   internal name or 0
 * @param strlst list with ComboBox items, terminated by NULL
 */

LabeledComboBox::LabeledComboBox(QWidget *parent, const char *name,
				 const char **strlst) : QWidget(parent, name)
{
	layout = new QVBoxLayout(this);
	label = new QLabel(this);
	combo = new QComboBox(this);
	if (layout && label && combo) {
//		combo->insertStrList(strlst);
		while (*strlst) {
			combo->insertItem(i18n(*strlst++));
		}
		layout->addWidget(label);
		layout->addWidget(combo);
	}
}

/**
 * Constructor.
 *
 * @param parent parent widget
 * @param name   internal name or 0
 */

LabeledSpinBox::LabeledSpinBox(QWidget *parent, const char *name) :
    QWidget(parent, name)
{
	layout = new QVBoxLayout(this);
	label = new QLabel(this);
	spinbox = new QSpinBox(this);
	if (layout && label && spinbox) {
		layout->addWidget(label);
		layout->addWidget(spinbox);
	}
}

/**
 * Constructor.
 *
 * @param parent parent widget
 * @param name   internal name or 0
 * @param fld    ID3_Field containing binary data
 */

BinaryOpenSave::BinaryOpenSave(QWidget *parent, const char *name,
			       ID3_Field *fld) :
	QWidget(parent, name), field(fld), loadfilename("")
{
	layout = new QHBoxLayout(this);
	label = new QLabel(this);
	openButton = new QPushButton(i18n("Import"), this);
	saveButton = new QPushButton(i18n("Export"), this);
	viewButton = new QPushButton(i18n("View"), this);
	if (layout && label && openButton && saveButton && viewButton) {
		layout->addWidget(label);
		layout->addWidget(openButton);
		layout->addWidget(saveButton);
		layout->addWidget(viewButton);
		connect(openButton, SIGNAL(clicked()), this, SLOT(loadData()));
		connect(saveButton, SIGNAL(clicked()), this, SLOT(saveData()));
		connect(viewButton, SIGNAL(clicked()), this, SLOT(viewData()));
	}
}

/**
 * Request name of file to import binary data from.
 * The data is imported later when Ok is pressed in the parent dialog.
 */

void BinaryOpenSave::loadData(void)
{
#ifdef CONFIG_USE_KDE
	loadfilename = KFileDialog::getOpenFileName();
#else
	loadfilename = QFileDialog::getOpenFileName();
#endif
}

/**
 * Request name of file and export binary data.
 */

void BinaryOpenSave::saveData(void)
{
#ifdef CONFIG_USE_KDE
	QString fn = KFileDialog::getSaveFileName();
#else
	QString fn = QFileDialog::getSaveFileName();
#endif
	if (!fn.isEmpty()) {
		QFile file(fn);
		if (file.open(IO_WriteOnly)) {
			QDataStream stream(&file);
			stream.writeRawBytes(
			    (const char *)field->GetRawBinary(),
			    (unsigned int)field->Size());
			file.close();
		}
	}
}

/**
 * Create image from binary data and display it in window.
 */

void BinaryOpenSave::viewData(void)
{
	QImage image;
	if (image.loadFromData((const uchar *)field->GetRawBinary(),
			       (uint)field->Size())) {
		ImageViewer iv(this, 0, &image);
		iv.exec();
	}
}

/**
 * Constructor.
 *
 * @param parent parent widget
 * @param name   internal name or 0
 * @param img    image to display in window
 */

ImageViewer::ImageViewer(QWidget *parent, const char *name, QImage *img) :
    QDialog(parent, name, TRUE), image(img)
{
	setFixedSize(image->width(), image->height());
	setCaption(i18n("View Picture"));
}

/**
 * Paint image, called when window has to be drawn.
 */

void ImageViewer::paintEvent(QPaintEvent *)
{
	QPainter paint(this);
	paint.drawImage(0, 0, *image, 0, 0, image->width(), image->height());
}

/**
 * Get description for ID3_Field.
 *
 * @param id ID of field
 * @return description or NULL if id unknown.
 */
const char *FieldControl::getFieldIDString(ID3_FieldID id) const
{
	static const struct id_str_s { ID3_FieldID id; const char *str; }
	id_str[] = {
		{ID3FN_TEXTENC,        I18N_NOOP("Text Encoding")},
		{ID3FN_TEXT,           I18N_NOOP("Text")},
		{ID3FN_URL,            I18N_NOOP("URL")},
		{ID3FN_DATA,           I18N_NOOP("Data")},
		{ID3FN_DESCRIPTION,    I18N_NOOP("Description")},
		{ID3FN_OWNER,          I18N_NOOP("Owner")},
		{ID3FN_EMAIL,          I18N_NOOP("Email")},
		{ID3FN_RATING,         I18N_NOOP("Rating")},
		{ID3FN_FILENAME,       I18N_NOOP("Filename")},
		{ID3FN_LANGUAGE,       I18N_NOOP("Language")},
		{ID3FN_PICTURETYPE,    I18N_NOOP("Picture Type")},
		{ID3FN_IMAGEFORMAT,    I18N_NOOP("Image format")},
		{ID3FN_MIMETYPE,       I18N_NOOP("Mimetype")},
		{ID3FN_COUNTER,        I18N_NOOP("Counter")},
		{ID3FN_ID,             I18N_NOOP("Identifier")},
		{ID3FN_VOLUMEADJ,      I18N_NOOP("Volume Adjustment")},
		{ID3FN_NUMBITS,        I18N_NOOP("Number of Bits")},
		{ID3FN_VOLCHGRIGHT,    I18N_NOOP("Volume Change Right")},
		{ID3FN_VOLCHGLEFT,     I18N_NOOP("Volume Change Left")},
		{ID3FN_PEAKVOLRIGHT,   I18N_NOOP("Peak Volume Right")},
		{ID3FN_PEAKVOLLEFT,    I18N_NOOP("Peak Volume Left")},
		{ID3FN_TIMESTAMPFORMAT,I18N_NOOP("Timestamp Format")},
		{ID3FN_CONTENTTYPE,    I18N_NOOP("Content Type")},
		{ID3FN_LASTFIELDID,    NULL}
	};

	const struct id_str_s *is = &id_str[0];
	while (is->str) {
		if (is->id == id) {
			break;
		}
		++is;
	}
	return is->str;
}

/**
 * Update field with data from dialog.
 */

void TextFieldControl::updateTag(void)
{
	// get encoding from selection
	ID3_TextEnc enc = frmlst->getSelectedEncoding();
	if (enc != ID3TE_NONE) {
		field->SetEncoding(enc);
	}
	Mp3File::setString(field, edit->text());
}

/**
 * Create widget for dialog.
 *
 * @param parent parent widget
 * @return widget to edit field.
 */

QWidget *TextFieldControl::createWidget(QWidget *parent)
{
	edit = new LabeledTextEdit(parent);
	if (edit == NULL)
		return NULL;

	edit->setLabel(i18n(getFieldIDString(field_id)));
	edit->setText(Mp3File::getString(field));
	return edit;
}

/**
 * Update field with data from dialog.
 */

void LineFieldControl::updateTag(void)
{
	field->Set(edit->text());
}

/**
 * Create widget for dialog.
 *
 * @param parent parent widget
 * @return widget to edit field.
 */

QWidget *LineFieldControl::createWidget(QWidget *parent)
{
	edit = new LabeledLineEdit(parent);
	if (edit) {
		edit->setLabel(i18n(getFieldIDString(field_id)));
		edit->setText(field->GetRawText());
	}
	return edit;
}

/**
 * Update field with data from dialog.
 */

void IntFieldControl::updateTag(void)
{
	field->Set(numinp->value());
}

/**
 * Create widget for dialog.
 *
 * @param parent parent widget
 * @return widget to edit field.
 */

QWidget *IntFieldControl::createWidget(QWidget *parent)
{
	numinp = new LabeledSpinBox(parent);
	if (numinp) {
		numinp->setLabel(i18n(getFieldIDString(field_id)));
		numinp->setValue(field->Get());
	}
	return numinp;
}

/**
 * Update field with data from dialog.
 */

void IntComboBoxControl::updateTag(void)
{
	field->Set(ptinp->currentItem());
	/* If this is the selected encoding, store it to be used by text fields */
	if (field->GetID() == ID3FN_TEXTENC) {
		frmlst->setSelectedEncoding((ID3_TextEnc)ptinp->currentItem());
	}
}

/**
 * Create widget for dialog.
 *
 * @param parent parent widget
 * @return widget to edit field.
 */

QWidget *IntComboBoxControl::createWidget(QWidget *parent)
{
	ptinp = new LabeledComboBox(parent, 0, strlst);
	if (ptinp) {
		ptinp->setLabel(i18n(getFieldIDString(field_id)));
		ptinp->setCurrentItem(field->Get());
	}
	return ptinp;
}

/**
 * Update field with data from dialog.
 */

void BinFieldControl::updateTag(void)
{
	if (bos && !bos->getFilename().isEmpty()) {
		QFile file(bos->getFilename());
		if (file.open(IO_ReadOnly)) {
			size_t size = file.size();
			uchar *data = new uchar[size];
			if (data) {
				QDataStream stream(&file);
				stream.readRawBytes((char *)data,
						    (unsigned int)size);
				field->Set(data, size);
				delete [] data;
			}
			file.close();
		}
	}
}

/**
 * Create widget for dialog.
 *
 * @param parent parent widget
 * @return widget to edit field.
 */

QWidget *BinFieldControl::createWidget(QWidget *parent)
{
	bos = new BinaryOpenSave(parent, 0, field);
	if (bos) {
		bos->setLabel(i18n(getFieldIDString(field_id)));
	}
	return bos;
}

#ifdef CONFIG_USE_KDE
/** Field edit dialog */
class EditFrameDialog : public KDialogBase { /* KDE */
public:
	EditFrameDialog(QWidget *parent, QString &caption,
			QPtrList<FieldControl> &ctls);
};

/**
 * Constructor.
 *
 * @param parent  parent widget
 * @param caption window title
 * @param ctls    list with controls to edit fields
 */

EditFrameDialog::EditFrameDialog(QWidget *parent, QString &caption,
 QPtrList<FieldControl> &ctls) :
	KDialogBase(parent, "edit_frame", true, caption, Ok|Cancel, Ok)
{
	QWidget *page = new QWidget(this);
	if (page) {
		setMainWidget(page);
		QVBoxLayout *vb = new QVBoxLayout(page);
		if (vb) {
			vb->setSpacing(6);
			vb->setMargin(6);
			FieldControl *fld_ctl = ctls.first();
			while (fld_ctl != NULL) {
				vb->addWidget(fld_ctl->createWidget(page));
				fld_ctl = ctls.next();
			}
		}
	}
	resize(fontMetrics().maxWidth() * 30, -1);
}
#else
/** Field edit dialog */
class EditFrameDialog : public QDialog {
public:
	EditFrameDialog(QWidget *parent, QString &caption,
			QPtrList<FieldControl> &ctls);
protected:
	QVBoxLayout *vlayout;
	QHBoxLayout *hlayout;
	QSpacerItem *hspacer;
	QPushButton *okButton;
	QPushButton *cancelButton;
};

/**
 * Constructor.
 *
 * @param parent  parent widget
 * @param caption window title
 * @param ctls    list with controls to edit fields
 */

EditFrameDialog::EditFrameDialog(QWidget *parent, QString &caption,
 QPtrList<FieldControl> &ctls) :
	QDialog(parent, "edit_frame", true)
{
	setCaption(caption);
	vlayout = new QVBoxLayout(this);
	if (vlayout) {
		vlayout->setSpacing(6);
		vlayout->setMargin(6);
		FieldControl *fld_ctl = ctls.first();
		while (fld_ctl != NULL) {
			vlayout->addWidget(fld_ctl->createWidget(this));
			fld_ctl = ctls.next();
		}
	}
	hlayout = new QHBoxLayout(vlayout);
	QSpacerItem *hspacer = new QSpacerItem(16, 0, QSizePolicy::Expanding,
					   QSizePolicy::Minimum);
	okButton = new QPushButton(i18n("OK"), this);
	cancelButton = new QPushButton(i18n("Cancel"), this);
	if (hlayout && okButton && cancelButton) {
		hlayout->addItem(hspacer);
		hlayout->addWidget(okButton);
		hlayout->addWidget(cancelButton);
		okButton->setDefault(TRUE);
		connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	}
	resize(fontMetrics().maxWidth() * 30, -1);
}
#endif

/**
 * Constructor.
 */

FrameList::FrameList() : listbox(0), tags(0), file(0), selected_enc(ID3TE_NONE)
{
	fieldcontrols.setAutoDelete(TRUE);
}

/**
 * Clear listbox and file reference.
 */

void FrameList::clear(void)
{
	listbox->clear();
	file = 0;
}

/**
 * Fill listbox with frame descriptions.
 * Before using this method, the listbox and file have to be set.
 * @see setListBox(), setTags()
 */

void FrameList::readTags(void)
{
	listbox->clear();
	if (tags) {
		ID3_Tag::Iterator* iter = tags->CreateIterator();
		ID3_Frame* frame;
		while ((frame = iter->GetNext()) != NULL) {
			const char *idstr = getIdString(frame->GetID());
			listbox->insertItem(idstr ? i18n(idstr) : QString(frame->GetTextID()));
		}
#if defined WIN32 && defined _DEBUG
		/* just to avoid user breakpoint in VC++ */
		ID3TagIterator_Delete(reinterpret_cast<ID3TagIterator*>(iter));
#else
		delete iter;
#endif
	}
}

/**
 * Set file and fill the list box with its frames.
 * The listbox has to be set with setListBox() before calling this
 * function.
 *
 * @param mp3file file
 */

void FrameList::setTags(Mp3File *mp3file)
{
	file = mp3file;
	tags = mp3file->tagV2;
	readTags();
}

/**
 * Get frame with index.
 *
 * @param index index in listbox
 * @return frame with index.
 */

ID3_Frame *FrameList::getFrame(int index) const
{
	ID3_Frame *frame = NULL;
	if (tags) {
		int i;
		ID3_Tag::Iterator* iter = tags->CreateIterator();
		for (i = 0;
		     i <= index && ((frame = iter->GetNext()) != NULL);
		     i++);
#if defined WIN32 && defined _DEBUG
		/* just to avoid user breakpoint in VC++ */
		ID3TagIterator_Delete(reinterpret_cast<ID3TagIterator*>(iter));
#else
		delete iter;
#endif
	}
	return frame;
}

/**
 * Get frame which is selected in listbox.
 *
 * @return selected frame.
 */

ID3_Frame *FrameList::getSelectedFrame(void) const
{
	ID3_Frame *frame = NULL;
	if (tags) {
		int i;
		ID3_Tag::Iterator* iter = tags->CreateIterator();
		for (i = 0;
		     i < (int)listbox->count() &&
			 ((frame = iter->GetNext()) != NULL);
		     i++) {
			if (listbox->isSelected(i)) {
				break;
			}
			else {
				frame = NULL;
			}
		}
#if defined WIN32 && defined _DEBUG
		/* just to avoid user breakpoint in VC++ */
		ID3TagIterator_Delete(reinterpret_cast<ID3TagIterator*>(iter));
#else
		delete iter;
#endif
	}
	return frame;
}

/**
 * Create dialog to edit the selected frame and update the fields if Ok is
 * returned.
 *
 * @return TRUE if Ok selected in dialog.
 */

bool FrameList::editFrame(void)
{
	bool result = FALSE;
	ID3_Frame *frame = getSelectedFrame();
	if (frame) {
		ID3_Frame::Iterator* iter = frame->CreateIterator();
		ID3_Field *field;
		while ((field = iter->GetNext()) != NULL) {
			ID3_FieldID id = field->GetID();
			ID3_FieldType type = field->GetType();
			if (type == ID3FTY_INTEGER) {
				if (id == ID3FN_TEXTENC) {
					static const char *strlst[] = {
						I18N_NOOP("ISO-8859-1"),
						I18N_NOOP("Unicode"),
						I18N_NOOP("UTF16BE"),
						I18N_NOOP("UTF8"),
						NULL
					};
					IntComboBoxControl *cbox =
						new IntComboBoxControl(this, id, field, strlst);
					if (cbox) {
						fieldcontrols.append(cbox);
					}
				}
				else if (id == ID3FN_PICTURETYPE) {
					static const char *strlst[] = {
						I18N_NOOP("Other"),
						I18N_NOOP("32x32 pixels PNG file icon"),
						I18N_NOOP("Other file icon"),
						I18N_NOOP("Cover (front)"),
						I18N_NOOP("Cover (back)"),
						I18N_NOOP("Leaflet page"),
						I18N_NOOP("Media"),
						I18N_NOOP("Lead artist/lead performer/soloist"),
						I18N_NOOP("Artist/performer"),
						I18N_NOOP("Conductor"),
						I18N_NOOP("Band/Orchestra"),
						I18N_NOOP("Composer"),
						I18N_NOOP("Lyricist/text writer"),
						I18N_NOOP("Recording Location"),
						I18N_NOOP("During recording"),
						I18N_NOOP("During performance"),
						I18N_NOOP("Movie/video screen capture"),
						I18N_NOOP("A bright coloured fish"),
						I18N_NOOP("Illustration"),
						I18N_NOOP("Band/artist logotype"),
						I18N_NOOP("Publisher/Studio logotype"),
						NULL
					};
					IntComboBoxControl *cbox =
						new IntComboBoxControl(this, id, field, strlst);
					if (cbox) {
						fieldcontrols.append(cbox);
					}
				}
				else if (id == ID3FN_TIMESTAMPFORMAT) {
					static const char *strlst[] = {
						I18N_NOOP("Other"),
						I18N_NOOP("MPEG frames as unit"),
						I18N_NOOP("Milliseconds as unit"),
						NULL
					};
					IntComboBoxControl *cbox =
						new IntComboBoxControl(this, id, field, strlst);
					if (cbox) {
						fieldcontrols.append(cbox);
					}
				}
				else if (id == ID3FN_CONTENTTYPE) {
					static const char *strlst[] = {
						I18N_NOOP("Other"),
						I18N_NOOP("Lyrics"),
						I18N_NOOP("Text transcription"),
						I18N_NOOP("Movement/part name"),
						I18N_NOOP("Events"),
						I18N_NOOP("Chord"),
						I18N_NOOP("Trivia/pop up"),
						NULL
					};
					IntComboBoxControl *cbox =
						new IntComboBoxControl(this, id, field, strlst);
					if (cbox) {
						fieldcontrols.append(cbox);
					}
				}
				else {
					IntFieldControl *intctl =
						new IntFieldControl(this, id, field);
					if (intctl) {
						fieldcontrols.append(intctl);
					}
				}
			}
			else if (type == ID3FTY_BINARY) {
				BinFieldControl *binctl =
					new BinFieldControl(this, id, field);
				if (binctl) {
					fieldcontrols.append(binctl);
				}
			}
			else if (type == ID3FTY_TEXTSTRING) {
				ID3_TextEnc enc = field->GetEncoding();
				if (id == ID3FN_TEXT ||
					// (ID3TE_IS_DOUBLE_BYTE_ENC(enc))
					enc == ID3TE_UTF16 || enc == ID3TE_UTF16BE) {
					// Large textedit for text fields
					TextFieldControl *textctl =
						new TextFieldControl(this, id, field);
					if (textctl) {
						fieldcontrols.append(textctl);
					}
				}
				else {
					LineFieldControl *textctl =
						new LineFieldControl(this, id, field);
					if (textctl) {
						fieldcontrols.append(textctl);
					}
				}
			}
		}
#if defined WIN32 && defined _DEBUG
		/* just to avoid user breakpoint in VC++ */
		ID3TagIterator_Delete(reinterpret_cast<ID3TagIterator*>(iter));
#else
		delete iter;
#endif
		const char *idstr = getIdString(frame->GetID());
		QString caption = idstr ? i18n(idstr) : QString(frame->GetTextID());
		EditFrameDialog *dialog =
			new EditFrameDialog(NULL, caption, fieldcontrols);
		if (dialog && dialog->exec() == QDialog::Accepted) {
			FieldControl *fld_ctl = fieldcontrols.first();
			// will be set if there is an encoding selector
			setSelectedEncoding(ID3TE_NONE);
			while (fld_ctl != NULL) {
				fld_ctl->updateTag();
				fld_ctl = fieldcontrols.next();
			}
			if (file) {
				file->changedV2 = TRUE;
			}
			result = TRUE;
		}
		fieldcontrols.clear();
	}
	return result;
}

/**
 * Delete selected frame.
 *
 * @return FALSE if frame not found.
 */

bool FrameList::deleteFrame(void)
{
	ID3_Frame *frame = getSelectedFrame();
	if (frame) {
		if (tags) {
			tags->RemoveFrame(frame);
			readTags(); // refresh listbox
		}
		if (file) {
			file->changedV2 = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * Add a new frame.
 *
 * @param id ID of frame to add
 * @return TRUE if frame added.
 */

bool FrameList::addFrame(ID3_FrameID id)
{
	if (id == ID3FID_METACOMPRESSION || id == ID3FID_METACRYPTO) {
		// these two do not seem to work
		return FALSE;
	}
	ID3_Frame *frame = new ID3_Frame(id);
	if (frame) {
		if (tags) {
			tags->AttachFrame(frame);
			readTags(); // refresh listbox
		}
		if (file) {
			file->changedV2 = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

/** Alphabetically sorted list of frame descriptions */
const char *FrameList::frameid_str[num_frameid] = {
	I18N_NOOP("AENC - Audio encryption"),
	I18N_NOOP("APIC - Attached picture"),
	I18N_NOOP("CDM  - Compressed data meta frame"),
	I18N_NOOP("COMM - Comments"),
	I18N_NOOP("COMR - Commercial"),
	I18N_NOOP("CRM  - Encrypted meta frame"),
	I18N_NOOP("ENCR - Encryption method registration"),
	I18N_NOOP("EQUA - Equalization"),
	I18N_NOOP("ETCO - Event timing codes"),
	I18N_NOOP("GEOB - General encapsulated object"),
	I18N_NOOP("GRID - Group identification registration"),
	I18N_NOOP("IPLS - Involved people list"),
	I18N_NOOP("LINK - Linked information"),
	I18N_NOOP("MCDI - Music CD identifier"),
	I18N_NOOP("MLLT - MPEG location lookup table"),
	I18N_NOOP("OWNE - Ownership frame"),
	I18N_NOOP("PRIV - Private frame"),
	I18N_NOOP("PCNT - Play counter"),
	I18N_NOOP("POPM - Popularimeter"),
	I18N_NOOP("POSS - Position synchronisation frame"),
	I18N_NOOP("RBUF - Recommended buffer size"),
	I18N_NOOP("RVAD - Relative volume adjustment"),
	I18N_NOOP("RVRB - Reverb"),
	I18N_NOOP("SYLT - Synchronized lyric/text"),
	I18N_NOOP("SYTC - Synchronized tempo codes"),
	I18N_NOOP("TALB - Album/Movie/Show title"),
	I18N_NOOP("TBPM - BPM (beats per minute)"),
	I18N_NOOP("TCOM - Composer"),
	I18N_NOOP("TCON - Content type"),
	I18N_NOOP("TCOP - Copyright message"),
	I18N_NOOP("TDAT - Date"),
	I18N_NOOP("TDLY - Playlist delay"),
	I18N_NOOP("TENC - Encoded by"),
	I18N_NOOP("TEXT - Lyricist/Text writer"),
	I18N_NOOP("TFLT - File type"),
	I18N_NOOP("TIME - Time"),
	I18N_NOOP("TIT1 - Content group description"),
	I18N_NOOP("TIT2 - Title/songname/content description"),
	I18N_NOOP("TIT3 - Subtitle/Description refinement"),
	I18N_NOOP("TKEY - Initial key"),
	I18N_NOOP("TLAN - Language(s)"),
	I18N_NOOP("TLEN - Length"),
	I18N_NOOP("TMED - Media type"),
	I18N_NOOP("TOAL - Original album/movie/show title"),
	I18N_NOOP("TOFN - Original filename"),
	I18N_NOOP("TOLY - Original lyricist(s)/text writer(s)"),
	I18N_NOOP("TOPE - Original artist(s)/performer(s)"),
	I18N_NOOP("TORY - Original release year"),
	I18N_NOOP("TOWN - File owner/licensee"),
	I18N_NOOP("TPE1 - Lead performer(s)/Soloist(s)"),
	I18N_NOOP("TPE2 - Band/orchestra/accompaniment"),
	I18N_NOOP("TPE3 - Conductor/performer refinement"),
	I18N_NOOP("TPE4 - Interpreted, remixed, or otherwise modified by"),
	I18N_NOOP("TPOS - Part of a set"),
	I18N_NOOP("TPUB - Publisher"),
	I18N_NOOP("TRCK - Track number/Position in set"),
	I18N_NOOP("TRDA - Recording dates"),
	I18N_NOOP("TRSN - Internet radio station name"),
	I18N_NOOP("TRSO - Internet radio station owner"),
	I18N_NOOP("TSIZ - Size"),
	I18N_NOOP("TSRC - ISRC (international standard recording code)"),
	I18N_NOOP("TSSE - Software/Hardware and settings used for encoding"),
	I18N_NOOP("TXXX - User defined text information"),
	I18N_NOOP("TYER - Year"),
	I18N_NOOP("UFID - Unique file identifier"),
	I18N_NOOP("USER - Terms of use"),
	I18N_NOOP("USLT - Unsynchronized lyric/text transcription"),
	I18N_NOOP("WCOM - Commercial information"),
	I18N_NOOP("WCOP - Copyright/Legal information"),
	I18N_NOOP("WOAF - Official audio file webpage"),
	I18N_NOOP("WOAR - Official artist/performer webpage"),
	I18N_NOOP("WOAS - Official audio source webpage"),
	I18N_NOOP("WORS - Official internet radio station homepage"),
	I18N_NOOP("WPAY - Payment"),
	I18N_NOOP("WPUB - Official publisher webpage"),
	I18N_NOOP("WXXX - User defined URL link")
};

/** Frame IDs corresponding to frameid_str[] */
const ID3_FrameID FrameList::frameid_code[num_frameid] = {
	ID3FID_AUDIOCRYPTO,
	ID3FID_PICTURE,
	ID3FID_METACOMPRESSION,
	ID3FID_COMMENT,
	ID3FID_COMMERCIAL,
	ID3FID_METACRYPTO,
	ID3FID_CRYPTOREG,
	ID3FID_EQUALIZATION,
	ID3FID_EVENTTIMING,
	ID3FID_GENERALOBJECT,
	ID3FID_GROUPINGREG,
	ID3FID_INVOLVEDPEOPLE,
	ID3FID_LINKEDINFO,
	ID3FID_CDID,
	ID3FID_MPEGLOOKUP,
	ID3FID_OWNERSHIP,
	ID3FID_PRIVATE,
	ID3FID_PLAYCOUNTER,
	ID3FID_POPULARIMETER,
	ID3FID_POSITIONSYNC,
	ID3FID_BUFFERSIZE,
	ID3FID_VOLUMEADJ,
	ID3FID_REVERB,
	ID3FID_SYNCEDLYRICS,
	ID3FID_SYNCEDTEMPO,
	ID3FID_ALBUM,
	ID3FID_BPM,
	ID3FID_COMPOSER,
	ID3FID_CONTENTTYPE,
	ID3FID_COPYRIGHT,
	ID3FID_DATE,
	ID3FID_PLAYLISTDELAY,
	ID3FID_ENCODEDBY,
	ID3FID_LYRICIST,
	ID3FID_FILETYPE,
	ID3FID_TIME,
	ID3FID_CONTENTGROUP,
	ID3FID_TITLE,
	ID3FID_SUBTITLE,
	ID3FID_INITIALKEY,
	ID3FID_LANGUAGE,
	ID3FID_SONGLEN,
	ID3FID_MEDIATYPE,
	ID3FID_ORIGALBUM,
	ID3FID_ORIGFILENAME,
	ID3FID_ORIGLYRICIST,
	ID3FID_ORIGARTIST,
	ID3FID_ORIGYEAR,
	ID3FID_FILEOWNER,
	ID3FID_LEADARTIST,
	ID3FID_BAND,
	ID3FID_CONDUCTOR,
	ID3FID_MIXARTIST,
	ID3FID_PARTINSET,
	ID3FID_PUBLISHER,
	ID3FID_TRACKNUM,
	ID3FID_RECORDINGDATES,
	ID3FID_NETRADIOSTATION,
	ID3FID_NETRADIOOWNER,
	ID3FID_SIZE,
	ID3FID_ISRC,
	ID3FID_ENCODERSETTINGS,
	ID3FID_USERTEXT,
	ID3FID_YEAR,
	ID3FID_UNIQUEFILEID,
	ID3FID_TERMSOFUSE,
	ID3FID_UNSYNCEDLYRICS,
	ID3FID_WWWCOMMERCIALINFO,
	ID3FID_WWWCOPYRIGHT,
	ID3FID_WWWAUDIOFILE,
	ID3FID_WWWARTIST,
	ID3FID_WWWAUDIOSOURCE,
	ID3FID_WWWRADIOPAGE,
	ID3FID_WWWPAYMENT,
	ID3FID_WWWPUBLISHER,
	ID3FID_WWWUSER
};

/**
 * Get description of frame.
 *
 * @param id ID of frame
 * @return description or NULL if id not found.
 */

const char *FrameList::getIdString(ID3_FrameID id) const
{
	int i;
	for (i = 0; i < num_frameid; i++) {
		if (frameid_code[i] == id) {
			return frameid_str[i];
		}
	}
	return NULL;
}

/**
 * Display a dialog to select a frame type.
 *
 * @return ID of selected frame,
 *         ID3FID_NOFRAME if no frame selected.
 */

ID3_FrameID FrameList::selectFrameId(void)
{
	int i;
	QStringList lst;
	bool ok = FALSE;
	for (i = 0; i < num_frameid; i++) {
		lst.append(i18n(frameid_str[i]));
	}
	QString res = QInputDialog::getItem(
		i18n("Add Frame"),
		i18n("Select the frame ID"), lst, 0, FALSE, &ok);
	if (ok) {
		int idx = lst.findIndex(res);
		if (idx >= 0 && idx < num_frameid) {
			return frameid_code[idx];
		}
	}
	return ID3FID_NOFRAME;
}
