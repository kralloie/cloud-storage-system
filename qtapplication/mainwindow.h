#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QCryptographicHash>
#include <QRegularExpressionMatch>
#include <QTextStream>
#include <QRegularExpression>
#include <QIODevice>
#include <QNetworkRequest>
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


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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
    void on_updateTextFile_clicked();
    void on_uploadButton_clicked();
    void on_deleteButton_clicked();
    void on_reloadButton_clicked();
    void on_undoButton_clicked();
    void on_downloadButton_clicked();

    void on_validateButton_clicked();

private:
    QMap<QString,QList<QString>*> fileViewerMap;
    QVector<tempFile> deletedFiles;
    Ui::MainWindow *ui;
    QString connectionState;
    QNetworkAccessManager* netManager = new QNetworkAccessManager(this);
    QString PORT;
    QMimeDatabase mimeDatabase;
    QString selectedFile;
    QString selectedFileType;
    QUrl baseUrl;
    QMap<QString,QList<QString>*> typeMap;
    QList<QString> imagesList;
    QList<QString> textList;
    QList<QString> imageExtensions = {"png","jpg","jpeg","webp","gif","raw"};
};
#endif // MAINWINDOW_H
