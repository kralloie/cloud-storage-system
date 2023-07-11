#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <openssl/aes.h>
#include <QtNetwork/QNetworkReply>
#include <QCryptographicHash>
#include <QRegularExpressionMatch>
#include <QTextStream>
#include <QAbstractTableModel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QTableView>
#include <QHeaderView>
#include <QRegularExpression>
#include <QIODevice>
#include <QNetworkRequest>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QFile>
#include <QImage>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QMimeDatabase>
#include <QMimeData>
#include <QMimeType>
#include "QJsonDocument"
#include "QJsonObject"
#include "QJsonArray"
#include "QJsonValue"
#include "QDebug"
#include "QStandardItem"
#include "QStandardItemModel"
#include "QUrl"
#include "QUrlQuery"
#include "QTemporaryFile"
#include "QPixmap"
#include "QHttpMultiPart"
#include "QHttpPart"
#include "QFileDialog"
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/qwebsocketserver.h>
#include <QPair>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class tempFile
{
    public:
        QByteArray data;
        QString fileName;
        QString fileType;
};

class user
{
    public:
        QString username;
        bool isAdmin;
        qint32 userId;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void sendGetRequest(const QUrl &url);
    void cleanUpReply();
    void handleError();
    void handleGetResponse(QNetworkReply* reply);
    void handlePatchRequest(QNetworkRequest& request, QHttpMultiPart* payload);
    void sendPatchRequest(const QUrl &url,const QByteArray &payloadData);
    void updateFileViewer();
    void handleDownload(QNetworkReply* reply, const QString& filename, const QString& path);
    void sendDeleteRequest(const QUrl& url);
    void uploadFile(QByteArray& fileBuffer, QUrl& url);
    void handlePostRequest(QHttpMultiPart* payload, QNetworkRequest& request);
    void updateStorage();
    void setImagePreview(const QByteArray& pixmapParam);
    void on_fileViewer_clicked(const QModelIndex &index);
    void handleTempStorage(QNetworkReply* reply,const QUrl& url);
    void setupCredentialsTable();
    void on_updateTextFile_clicked();
    void on_uploadButton_clicked();
    void on_deleteButton_clicked();
    void on_reloadButton_clicked();
    void on_undoButton_clicked();
    void on_downloadButton_clicked();
    void on_validateButton_clicked();
    void on_submitLogin_clicked();
    void handleLogin(QByteArray& data, QUrl& route);
    void validateConnection(QNetworkReply* reply);
    void on_adminPanelButton_clicked();
    void on_mainPanelButton_clicked();
    void on_loginReturn_clicked();
    void on_registerButton_clicked();
    void on_registerAccount_clicked();
    void handleNewSocketConnection();
    void handleRegister();
    void setupGlobalStorage();
    void handleLogout();
    void on_logoutButton_clicked();

private:
    QMap<QString,QList<QString>*> fileViewerMap;
    QVector<tempFile> deletedFiles;
    Ui::MainWindow *ui;
    user currentUser;
    QString connectionState;
    QNetworkAccessManager* netManager = new QNetworkAccessManager(this);
    QString PORT;
    QString token;
    QMimeDatabase mimeDatabase;
    QWebSocketServer* wsserver = new QWebSocketServer("wserver",QWebSocketServer::NonSecureMode,this);
    QString selectedFile;
    QString selectedFileType;
    QUrl baseUrl;
    QList<QString> imagesList;
    QList<QString> textList;
    QList<QString> imageExtensions = {"png","jpg","jpeg","webp","gif","raw"};
};
#endif // MAINWINDOW_H
