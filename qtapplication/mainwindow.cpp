#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow),baseUrl("http://localhost:8080")
{
    {
        QApplication::setStyle("fusion");
        QColor baseColor(QStringLiteral("#32383D"));
        QColor complementColor(QStringLiteral("#202529"));
        QColor disabledColor(QStringLiteral("#4C555C"));
        QColor highlightColor(QStringLiteral("#009BFF"));
        QColor linkColor(QStringLiteral("#009BFF"));
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Active, QPalette::Button, disabledColor.darker());
        darkPalette.setColor(QPalette::AlternateBase, complementColor);
        darkPalette.setColor(QPalette::Base, baseColor);
        darkPalette.setColor(QPalette::Button, complementColor);
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
        darkPalette.setColor(QPalette::Disabled, QPalette::Light, complementColor);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
        darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledColor);
        darkPalette.setColor(QPalette::Highlight, highlightColor);
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        darkPalette.setColor(QPalette::Link, linkColor);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::ToolTipBase, highlightColor);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Window, complementColor);
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        QApplication::setPalette(darkPalette);
    }
    ui->setupUi(this);
    setWindowTitle("Storage Manager");
    updateStorage();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::updateStorage()
{
    QNetworkRequest request;
    MainWindow::textList = {};
    MainWindow::imagesList = {};
    request.setUrl(QUrl("http://localhost:8080"));
    QNetworkReply *reply = netManager->get(request);

    QEventLoop eventLoop;
    connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    QByteArray JsonData = reply->readAll();
    QJsonDocument responseJSON = QJsonDocument::fromJson(JsonData);
    QJsonObject responseObject = responseJSON.object();
    QJsonValue images = responseObject.value("images");
    QJsonValue texts = responseObject.value("texts");
    QJsonArray textArray = texts.toArray();
    QJsonArray imagesArray = images.toArray();

    for(const QJsonValue &file : textArray)
    {
        MainWindow::textList.append(file.toString());
    }

    for(const QJsonValue &file : imagesArray)
    {
        MainWindow::imagesList.append(file.toString());
    }

    updateFileViewer();
}


void MainWindow::sendGetRequest(const QUrl &url)
{
    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply* reply = netManager->get(request);
    connect(reply,&QNetworkReply::finished,this,[this,reply]() -> void{
        handleGetResponse(reply);
    });
}

void MainWindow::handleGetResponse(QNetworkReply* reply)
{
    QByteArray responseData = reply->readAll();
    QByteArray fileType = reply->rawHeader("File-Type");
    QByteArray contentType = reply->rawHeader("Content-Type");

    if(fileType == "text")
    {
        if(contentType == "application/json; charset=utf-8")
        {
            QJsonDocument responseJSON = QJsonDocument::fromJson(responseData);
            ui->responseDisplay->setPlainText(QString::fromUtf8(responseJSON.toJson()));
        }
        else
        {
            ui->responseDisplay->setPlainText(responseData);
        }
        setImagePreview(nullptr);
    }
    else if(fileType == "image")
    {
        setImagePreview(responseData);
        ui->responseDisplay->setPlainText(nullptr);
    }
    else
    {
        return;
    }
    connect(reply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(handleError()));
    connect(reply,&QNetworkReply::finished,this,&MainWindow::cleanUpReply);
}

void MainWindow::updateFileViewer()
{
    QStandardItemModel *model = new QStandardItemModel(this);
    QStandardItem* Images = new QStandardItem("Image Files");
    Images->setEditable(false);
    QStandardItem* Texts = new QStandardItem("Text Files");
    Texts->setEditable(false);
    model->appendRow(Texts);
    model->appendRow(Images);

    for(const QString &file : MainWindow::textList)
    {
        QStandardItem* item = new QStandardItem(file);
        item->setEditable(false);
        Texts->appendRow(item);
    }

    for(const QString& image: MainWindow::imagesList)
    {
        QStandardItem* item = new QStandardItem(image);
        item->setEditable(false);
        Images->appendRow(item);
    }
    model->setHeaderData(0,Qt::Horizontal,"File Viewer");
    ui->fileViewer->setModel(model);
}

void MainWindow::on_fileViewer_clicked(const QModelIndex &index)
{
    selectedFile = "";
    QStandardItemModel* modelPtr = dynamic_cast<QStandardItemModel*>(ui->fileViewer->model());
    QStandardItem* selectedItem = modelPtr->itemFromIndex(index);
    QString itemString = selectedItem->text();
    if(textList.contains(itemString))
    {
        QUrl targetUrl = QUrl("http://localhost:8080/texts");
        QUrlQuery targetQuery;
        targetQuery.addQueryItem("file",itemString);
        targetUrl.setQuery(targetQuery);
        selectedFile = itemString;
        selectedFileType = "texts";
        sendGetRequest(targetUrl);
    }
    else if(imagesList.contains(itemString))
    {
        QUrl targetUrl = QUrl("http://localhost:8080/images");
        QUrlQuery targetQuery;
        targetQuery.addQueryItem("file",itemString);
        targetUrl.setQuery(targetQuery);
        selectedFile = itemString;
        selectedFileType = "images";
        sendGetRequest(targetUrl);
    }
    else
    {
        return;
    }
}

void MainWindow::setImagePreview(const QByteArray& pixmapParam)
{
    QPixmap pixmap;
    pixmap.loadFromData(pixmapParam);
    QSize imageSize = ui->imagePreview->size();
    QPixmap scaledPixmap = pixmap.scaled(imageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->imagePreview->setPixmap(scaledPixmap);
}


void MainWindow::sendPatchRequest(const QUrl& url,const QByteArray& payloadData)
{
    QNetworkRequest request(url);
    QHttpMultiPart* payload = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart payloadPart;
    payloadPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"file\""));
    payloadPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    payloadPart.setBody(payloadData);
    payload->append(payloadPart);
    QByteArray boundary = payload->boundary();
    QByteArray contentType = QStringLiteral("multipart/form-data; boundary=\"%1\"").arg(boundary).toUtf8();
    request.setHeader(QNetworkRequest::ContentTypeHeader,contentType);
    handlePatchRequest(request,payload);
}

void MainWindow::handlePatchRequest(QNetworkRequest& request, QHttpMultiPart* payload)
{
    QNetworkReply* reply = netManager->sendCustomRequest(request,"PATCH",payload);
    connect(reply,&QNetworkReply::finished,this,&MainWindow::cleanUpReply);
    connect(reply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(handleError()));
}

void MainWindow::handleError()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QMessageBox::critical(this,"Server Error",reply->errorString());
}

void MainWindow::cleanUpReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if(reply)
    {
        reply->deleteLater();
    }
}

void MainWindow::on_updateTextFile_clicked()
{
    if(selectedFileType == "texts")
    {
        QUrl targetUrl = QUrl("http://localhost:8080/texts");
        QUrlQuery targetQuery;
        targetQuery.addQueryItem("file",selectedFile);
        targetUrl.setQuery(targetQuery);
        QString data = ui->responseDisplay->toPlainText();
        QByteArray payloadData = data.toUtf8();
        sendPatchRequest(targetUrl,payloadData);
    }
    else if(selectedFileType == "images")
    {
        QMessageBox::warning(this,"Invalid file","Cannot edit image file.");
    }
    else
    {
        QMessageBox::warning(this,"No file selected","Please select a text file for editing the content.");
    }
}

void MainWindow::uploadFile(QByteArray& fileBuffer, QUrl& url)
{
    QNetworkRequest request(url);
    QHttpMultiPart* payload = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart payloadPart;
    payloadPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"file\""));
    payloadPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    payloadPart.setBody(fileBuffer);
    payload->append(payloadPart);
    QByteArray boundary = payload->boundary();
    QByteArray contentType = QStringLiteral("multipart/form-data; boundary=\"%1\"").arg(boundary).toUtf8();
    request.setHeader(QNetworkRequest::ContentTypeHeader,contentType);
    handlePostRequest(payload,request);
}

void MainWindow::handlePostRequest(QHttpMultiPart* payload, QNetworkRequest& request)
{
    QNetworkReply* reply = netManager->post(request,payload);
    connect(reply,&QNetworkReply::finished,this,&MainWindow::cleanUpReply);
    connect(reply,&QNetworkReply::finished,this,&MainWindow::updateStorage);
    connect(reply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(handleError()));
}

void MainWindow::on_uploadButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,"Open File");
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    QString queryParam = QStringLiteral("?file=%1").arg(fileName);
    QString fileExtension = fileInfo.suffix();
    QString storagePath;

    if(imageExtensions.contains(fileExtension))
    {
        storagePath = "images";
    }
    else
    {
        storagePath = "texts";
    }

    QString fileUrl = QStringLiteral("%1/%2/%3").arg(baseUrl.toString(),storagePath,queryParam);
    QUrl url = QUrl(fileUrl);
    QFile targetFile(filePath);
    if(targetFile.open(QIODevice::ReadOnly))
    {
        QByteArray fileBuffer = targetFile.readAll();
        uploadFile(fileBuffer,url);
    };
}


void MainWindow::sendDeleteRequest(const QUrl& url)
{
    QNetworkRequest request(url);
    QNetworkReply* reply = netManager->sendCustomRequest(request,"DELETE");
    connect(reply,&QNetworkReply::finished,this,&MainWindow::cleanUpReply);
    connect(reply,&QNetworkReply::finished,this,&MainWindow::updateStorage);
    connect(reply,SIGNAL(errorOccurred(QNetworkReply::NetworkError)),this,SLOT(handleError()));
    selectedFile = nullptr;
    selectedFileType = nullptr;
}


void MainWindow::on_deleteButton_clicked()
{
    if(selectedFile != "")
    {
        QString targetFile = selectedFile;
        QUrl url = QStringLiteral("http://localhost:8080/%1?file=%2").arg(selectedFileType,targetFile);
        QNetworkRequest tempRequest(url);
        QNetworkReply* tempReply = netManager->get(tempRequest);
        connect(tempReply,&QNetworkReply::finished,[this,url,tempReply]() -> void {
            handleTempStorage(tempReply,url);
        });
    }
    else
    {
        QMessageBox::warning(this,"No file selected","No file selected to delete, please select the file that you want to delete.");
    }
}

void MainWindow::handleTempStorage(QNetworkReply* reply,const QUrl& url)
{
    if(selectedFile != nullptr)
    {
        QByteArray tempData = reply->readAll();
        tempFile file;
        file.data = tempData;
        file.fileName = selectedFile;
        file.fileType = selectedFileType;
        deletedFiles.push_front(file);
        sendDeleteRequest(url);
    }
}


void MainWindow::on_reloadButton_clicked()
{
    updateStorage();
}


void MainWindow::on_undoButton_clicked()
{
    if(deletedFiles.size() > 0)
    {
        tempFile recoveredFile = deletedFiles.at(0);
        QUrl recoveredFileUrl = QStringLiteral("http://localhost:8080/%1?file=%2").arg(recoveredFile.fileType,recoveredFile.fileName);
        uploadFile(recoveredFile.data,recoveredFileUrl);
        deletedFiles.remove(0);
    }
    else
    {
        QMessageBox::warning(this,"Empty temp storage","No files in the temporary storage to recover.");
    }
}


void MainWindow::on_downloadButton_clicked()
{
    if(selectedFile != "")
    {
        QString filename = selectedFile;
        QNetworkRequest request;
        QUrl targetUrl = QStringLiteral("http://localhost:8080/%1?file=%2").arg(selectedFileType,selectedFile);
        request.setUrl(targetUrl);
        QString targetPath = QFileDialog::getExistingDirectory(this, "Select directory to save the file", QDir::homePath());

        if(targetPath.isEmpty())
        {
            return;
        }

        QNetworkReply* reply = netManager->get(request);
        connect(reply,&QNetworkReply::finished,this,[this,reply,filename,targetPath]() -> void{
            handleDownload(reply,filename,targetPath);
        });
    }
    else
    {
        QMessageBox::warning(this,"No file selected","Please select the file that you want to download.");
    }
}

void MainWindow::handleDownload(QNetworkReply* reply, const QString& filename, const QString& path)
{
    QString absolutePath = QStringLiteral("%1/%2").arg(path,filename);
    QFile file(absolutePath);
    QByteArray data = reply->readAll();
    if (file.open(QIODevice::WriteOnly))
    {
        if(selectedFileType == "texts")
        {
            QTextStream stream(&file);
            stream << QString::fromUtf8(data);
        }
        else
        {
            file.write(data);
        }
    }
    file.close();
}
