/**
 * \file httpclient.cpp
 * Client to connect to HTTP server.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 30 Dec 2008
 *
 * Copyright (C) 2008-2009  Urs Fleisch
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

#include "httpclient.h"
#include "kid3.h"

/** Only defined for generation of KDE3 translation files */
#define FOR_KDE3_PO_1 I18N_NOOP("Data received: %1")

/**
 * Constructor.
 */
HttpClient::HttpClient() :
	m_rcvIdx(0), m_rcvBodyIdx(0), m_rcvBodyLen(0)
{
#if QT_VERSION >= 0x040000
	m_sock = new QTcpSocket();
	connect(m_sock, SIGNAL(disconnected()),
			this, SLOT(slotConnectionClosed()));
	connect(m_sock, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(slotError(QAbstractSocket::SocketError)));
#else
	m_sock = new QSocket();
	connect(m_sock, SIGNAL(connectionClosed()),
			this, SLOT(slotConnectionClosed()));
	connect(m_sock, SIGNAL(error(int)),
			this, SLOT(slotError(int)));
#endif
	connect(m_sock, SIGNAL(hostFound()),
			this, SLOT(slotHostFound()));
	connect(m_sock, SIGNAL(connected()),
			this, SLOT(slotConnected()));
	connect(m_sock, SIGNAL(readyRead()),
			this, SLOT(slotReadyRead()));
}

/**
 * Destructor.
 */
HttpClient::~HttpClient()
{
	m_sock->close();
	m_sock->disconnect();
	delete m_sock;
}

/**
 * Emit a progress signal with step/total steps.
 *
 * @param text       state text
 * @param step       current step
 * @param totalSteps total number of steps
 */
void HttpClient::emitProgress(const QString& text, int step, int totalSteps)
{
	emit progress(text, step, totalSteps);
}

/**
 * Emit a progress signal with bytes received/total bytes.
 *
 * @param text state text
 */
void HttpClient::emitProgress(const QString& text)
{
	emit progress(text, m_rcvIdx - m_rcvBodyIdx, m_rcvBodyLen);
}

/**
 * Get string with proxy or destination and port.
 * If a proxy is set, the proxy is returned, else the real destination.
 *
 * @param dst real destination
 *
 * @return "destinationname:port".
 */
QString HttpClient::getProxyOrDest(const QString& dst)
{
	QString dest;
	if (Kid3App::s_miscCfg.m_useProxy) {
		dest = Kid3App::s_miscCfg.m_proxy;
	}
	if (dest.isEmpty()) {
		dest = dst;
	}
	return dest;
}

/**
 * Extract name and port from string.
 *
 * @param namePort input string with "name:port"
 * @param name     output string with "name"
 * @param port     output integer with port
 */
void HttpClient::splitNamePort(const QString& namePort,
																 QString& name, int& port)
{
	int colPos = namePort.QCM_lastIndexOf(':');
	if (colPos >= 0) {
		bool ok;
		port = namePort.mid(colPos + 1).toInt(&ok);
		if (!ok) port = 80;
		name = namePort.left(colPos);
	} else {
		name = namePort;
		port = 80;
	}
}

/**
 * Display status if host is found.
 */
void HttpClient::slotHostFound()
{
	emitProgress(i18n("Host found..."), CS_HostFound, CS_EstimatedBytes);
}

/**
 * Display status if connection is established.
 */
void HttpClient::slotConnected()
{
	m_sock->QCM_writeBlock(m_request.QCM_latin1(), m_request.length());
	emitProgress(i18n("Request sent..."), CS_RequestSent, CS_EstimatedBytes);
}

/**
 * Read the available bytes.
 */
void HttpClient::readBytesAvailable()
{
	unsigned long len = m_sock->bytesAvailable();
	if (len > 0) {
		m_rcvBuf.resize(m_rcvIdx + len);
		unsigned long bytesRead = m_sock->QCM_readBlock(m_rcvBuf.data() + m_rcvIdx,
		                                                len);
		if (bytesRead > 0) {
			m_rcvIdx += bytesRead;
			if (bytesRead < len) {
				m_rcvBuf.resize(m_rcvIdx);
			}
			if (m_rcvBodyIdx == 0 && m_rcvBodyLen == 0) {
#if QT_VERSION >= 0x040000
				QByteArray& str(m_rcvBuf);
#else
				QCString str(m_rcvBuf.data(), m_rcvIdx + 1);
#endif
				int contentLengthPos = str.QCM_indexOf("Content-Length:");
				if (contentLengthPos != -1) {
					contentLengthPos += 15;
					while (str[contentLengthPos] == ' ' ||
					       str[contentLengthPos] == '\t') {
						++contentLengthPos;
					}
					int contentLengthLen = str.QCM_indexOf("\r\n", contentLengthPos);
					if (contentLengthLen > contentLengthPos) {
						contentLengthLen -= contentLengthPos;
					}
					m_rcvBodyLen = str.mid(contentLengthPos, contentLengthLen).toULong();
				}
				int contentTypePos = str.QCM_indexOf("Content-Type:");
				if (contentTypePos != -1) {
					contentTypePos += 13;
					char ch;
					while (contentTypePos < static_cast<int>(m_rcvIdx) &&
					       ((ch = str[contentTypePos]) == ' ' || ch == '\t')) {
						++contentTypePos;
					}
					int contentTypeEnd = contentTypePos;
					while (contentTypeEnd < static_cast<int>(m_rcvIdx) &&
					       (ch = str[contentTypeEnd]) != ' ' && ch != '\t' &&
					       ch != '\r' && ch != '\n' && ch != ';') {
						++contentTypeEnd;
					}
					m_rcvBodyType = str.mid(contentTypePos,
					                        contentTypeEnd - contentTypePos);
				}
				int bodyPos = str.QCM_indexOf("\r\n\r\n");
				if (bodyPos != -1) {
					m_rcvBodyIdx = bodyPos + 4;
				}
			}
		}
	}
}

/**
 * Read received data when the server has closed the connection.
 * A bytesReceived() signal is emitted.
 */
void HttpClient::slotConnectionClosed()
{
	readBytesAvailable();
#if QT_VERSION >= 0x040000
	QByteArray body(m_rcvBuf.data() + m_rcvBodyIdx,
	                m_rcvBuf.size() - m_rcvBodyIdx);
#else
	QByteArray body;
	body.duplicate(m_rcvBuf.data() + m_rcvBodyIdx,
	               m_rcvBuf.size() - m_rcvBodyIdx);
	body.resize(m_rcvBuf.size() - m_rcvBodyIdx + 1);
	body[m_rcvBuf.size() - m_rcvBodyIdx] = '\0';
#endif
	emit bytesReceived(body);
	m_sock->close();
	emitProgress(i18n("Ready."));
}

/**
 * Display information about read progress.
 */
void HttpClient::slotReadyRead()
{
	readBytesAvailable();
	emitProgress(KCM_i18n1("Data received: %1", m_rcvIdx));
}

/**
 * Display information about socket error.
 */
#if QT_VERSION >= 0x040000
void HttpClient::slotError(QAbstractSocket::SocketError err)
{
	if (err == QAbstractSocket::RemoteHostClosedError)
		return;
	QString msg(i18n("Socket error: "));
	switch (err) {
		case QAbstractSocket::ConnectionRefusedError:
			msg += i18n("Connection refused");
			break;
		case QAbstractSocket::HostNotFoundError:
			msg += i18n("Host not found");
			break;
		case QAbstractSocket::SocketAccessError:
			msg += i18n("Read failed");
			break;
		default:
			msg += m_sock->errorString();
	}
	emitProgress(msg);
}
#else
void HttpClient::slotError(int err)
{
	QString msg(i18n("Socket error: "));
	switch (err) {
		case QSocket::ErrConnectionRefused:
			msg += i18n("Connection refused");
			break;
		case QSocket::ErrHostNotFound:
			msg += i18n("Host not found");
			break;
		case QSocket::ErrSocketRead:
			msg += i18n("Read failed");
			break;
		default:
			msg += QString::number(err);
	}
	emitProgress(msg);
}
#endif

/**
 * Send a HTTP GET request.
 *
 * @param server host name
 * @param path   path of the URL
 */
void HttpClient::sendRequest(const QString& server, const QString& path)
{
	QString dest;
	int destPort;
	QString destNamePort(getProxyOrDest(server));
	splitNamePort(destNamePort, dest, destPort);
	QString serverName;
	int serverPort;
	splitNamePort(server, serverName, serverPort);
	m_request = "GET ";
	if (dest != serverName) {
		m_request += "http://";
		m_request += serverName;
		if (serverPort != 80) {
			m_request += ':';
			m_request += QString::number(serverPort);
		}
	}
	m_request += path;
	m_request += " HTTP/1.1\r\nUser-Agent: Kid3/" VERSION "\r\nHost: ";
	m_request += serverName;
	m_request += "\r\nConnection: close\r\n\r\n";

	m_rcvBuf.resize(0);
	m_rcvIdx = 0;
	m_rcvBodyIdx = 0;
	m_rcvBodyLen = 0;
	m_rcvBodyType = "";

	m_sock->connectToHost(dest, destPort);
	emitProgress(i18n("Connecting..."), CS_Connecting, CS_EstimatedBytes);
}
