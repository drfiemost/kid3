/* config.h.  Generated by cmake from config.h.cmake */
#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "@CPACK_PACKAGE_VERSION@"
#define RELEASE_YEAR "@RELEASE_YEAR@"

/* Define if you have id3lib installed */
#cmakedefine HAVE_ID3LIB 1
#cmakedefine HAVE_NO_ID3LIB_VBR

/* Define if you have TagLib installed */
#cmakedefine HAVE_TAGLIB 1
#cmakedefine HAVE_TAGLIB_ID3V23_SUPPORT 1
#cmakedefine HAVE_TAGLIB_XM_SUPPORT 1

/* Define if you have mp4v2 installed */
#cmakedefine HAVE_MP4V2 1

/* Define if you have mp4v2/mp4v2.h installed */
#cmakedefine HAVE_MP4V2_MP4V2_H 1

/* Define to build with MP4GetMetadataByIndex char** argument */
#cmakedefine HAVE_MP4V2_MP4GETMETADATABYINDEX_CHARPP_ARG 1

/* Define if you have ogg/vorbis installed */
#cmakedefine HAVE_VORBIS 1

/* Define if you have FLAC++ installed */
#cmakedefine HAVE_FLAC 1
#cmakedefine HAVE_NO_FLAC_STREAMMETADATA_OPERATOR
#cmakedefine HAVE_FLAC_PICTURE 1

/* Define if you have QtDBus installed */
#cmakedefine HAVE_QTDBUS 1

/* Define if you have Phonon installed */
#cmakedefine HAVE_PHONON 1

/* Define if mntent.h is available */
#cmakedefine HAVE_MNTENT_H 1

#cmakedefine CFG_DATAROOTDIR "@CFG_DATAROOTDIR@"
#cmakedefine CFG_DOCDIR "@CFG_DOCDIR@"
#cmakedefine CFG_TRANSLATIONSDIR "@CFG_TRANSLATIONSDIR@"
/* Path to plugins directory relative from directory containing executable */
#cmakedefine CFG_PLUGINSDIR "@CFG_PLUGINSDIR@"

#endif
