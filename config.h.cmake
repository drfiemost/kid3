/* config.h.  Generated by cmake from config.h.cmake */
#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "@CPACK_PACKAGE_VERSION@"
#define RELEASE_YEAR "@RELEASE_YEAR@"

/* Define if you have QtDBus installed */
#cmakedefine HAVE_QTDBUS 1

/* Define if QML is enabled */
#cmakedefine HAVE_QML 1

/* Define if you have Phonon installed */
#cmakedefine HAVE_PHONON 1

/* Define if mntent.h is available */
#cmakedefine HAVE_MNTENT_H 1

#cmakedefine CFG_DATAROOTDIR "@CFG_DATAROOTDIR@"
#cmakedefine CFG_DOCDIR "@CFG_DOCDIR@"
#cmakedefine CFG_TRANSLATIONSDIR "@CFG_TRANSLATIONSDIR@"
#cmakedefine HAVE_TRANSLATIONSDIR_IN_QRC 1
#cmakedefine CFG_QMLDIR "@CFG_QMLDIR@"
#cmakedefine HAVE_QMLDIR_IN_QRC 1
#cmakedefine CFG_QMLSRCDIR "@CFG_QMLSRCDIR@"
/* Path to plugins directory relative from directory containing executable */
#cmakedefine CFG_PLUGINSDIR "@CFG_PLUGINSDIR@"

#endif
