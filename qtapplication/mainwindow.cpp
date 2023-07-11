#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow),connectionState(QString("disconnected"))
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
    wsserver->listen(QHostAddress("ws://localhost"),7777);
    connect(wsserver,&QWebSocketServer::newConnection,this,&MainWindow::handleNewSocketConnection);
    ui->passwordInput->setPlaceholderText("Password:");
    ui->adminPanelButton->setEnabled(false);
    QHeaderView* headerView = ui->userCredentialsTable->horizontalHeader();
    headerView->setSectionResizeMode(QHeaderView::Stretch);
    ui->regPassword->setPlaceholderText("Password:");
    ui->regUsername->setPlaceholderText("Username:");
    ui->usernameInput->setPlaceholderText("Username:");
    ui->passwordInput->setEchoMode(QLineEdit::Password);
    ui->regPassword->setEchoMode(QLineEdit::Password);
    ui->stackedWidget->setCurrentIndex(4);
    ui->portInput->setPlaceholderText("Input PORT:");
    ui->imagePreview->setContentsMargins(0,0,0,0);
    ui->stateLabel->setAlignment(Qt::AlignCenter);
    ui->registerLabel->setAlignment(Qt::AlignCenter);
    ui->loginLabel->setAlignment(Qt::AlignCenter);
    ui->userCredentialsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->userCredentialsTable->verticalHeader()->setVisible(false);
    ui->portInput->setMaxLength(5);
    setWindowTitle("Storage Manager");
}


MainWindow::~MainWindow()
{
    delete ui;
}

QString encryptPassword(QString password)
{
    QByteArray utfPass = password.toUtf8();
    QByteArray hashedPass = QCryptographicHash::hash(utfPass,QCryptographicHash::Md5);
    QString encryptedPassword = QString(hashedPass.toHex());
    return encryptedPassword;
}

void MainWindow::updateStorage()
{
    QNetworkRequest request;
    MainWindow::textList = {};
    MainWindow::imagesList = {};
    QUrl targetUrl = QUrl(QStringLiteral("http://localhost:%1/user").arg(PORT,token));
    QUrlQuery userQuery;
    userQuery.addQueryItem("username",currentUser.username);
    userQuery.addQueryItem("api",token);
    targetUrl.setQuery(userQuery);
    request.setUrl(targetUrl);
    ui->adminPanelButton->setEnabled(currentUser.isAdmin);

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

void MainWindow::handleNewSocketConnection()
{
    QWebSocket* socket = wsserver->nextPendingConnection();
    connect(socket,&QWebSocket::textMessageReceived,this,[this](QString message) -> void{
        qDebug() << message;
    });
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

    for(auto& file : MainWindow::textList)
    {
        QStandardItem* item = new QStandardItem(file);
        item->setEditable(false);
        Texts->appendRow(item);
    }

    for(auto& image: MainWindow::imagesList)
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
    QStandardItemModel* modelPtr = qobject_cast<QStandardItemModel*>(ui->fileViewer->model());
    QStandardItem* selectedItem = modelPtr->itemFromIndex(index);
    QString itemString = selectedItem->text();
    if(textList.contains(itemString))
    {
        QUrl targetUrl = QUrl(QStringLiteral("%1/texts").arg(baseUrl.toString()));
        QUrlQuery targetQuery;
        targetQuery.addQueryItem("file",itemString);
        targetQuery.addQueryItem("username",currentUser.username);
        targetQuery.addQueryItem("api",token);
        targetUrl.setQuery(targetQuery);
        selectedFile = itemString;
        selectedFileType = "texts";
        sendGetRequest(targetUrl);
    }
    else if(imagesList.contains(itemString))
    {
        QUrl targetUrl = QUrl(QStringLiteral("%1/images").arg(baseUrl.toString()));
        QUrlQuery targetQuery;
        targetQuery.addQueryItem("file",itemString);
        targetQuery.addQueryItem("username",currentUser.username);
        targetQuery.addQueryItem("api",token);
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
    payloadPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"file\"")); // tendria q usar qstringliteral y en vez d file usar selectefile en el post tmb
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
        QUrl targetUrl = QUrl(QStringLiteral("%1/texts").arg(baseUrl.toString()));
        QUrlQuery targetQuery;
        targetQuery.addQueryItem("file",selectedFile);
        targetQuery.addQueryItem("username",currentUser.username);
        targetQuery.addQueryItem("api",token);
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
    payloadPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"file\"")); // NAME ES REQ.FILE.ORIGINALNAME Y FILENAME ES EL REQ.'FILE' :$.
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
    if(connectionState == "disconnected")
    {
        QMessageBox::warning(this,"Disconnected","You are currently not connected to the cloud.");
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this,"Open File");
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
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

    QString fileUrl = QStringLiteral("%1/%2?file=%3&username=%4&api=%5").arg(baseUrl.toString(),storagePath,fileName,currentUser.username,token);
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
    if(connectionState == "disconnected")
    {
        QMessageBox::warning(this,"Disconnected","You are currently not connected to the cloud.");
        return;
    }

    if(selectedFile != "")
    {
        QString targetFile = selectedFile;
        QUrl url = QStringLiteral("%1/%2?file=%3&username=%4&api=%5").arg(baseUrl.toString(),selectedFileType,targetFile,currentUser.username,token);
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
    if(PORT.isEmpty())
    {
        QMessageBox::warning(this,"Missing Port","No port specified for connecting to the cloud.");
        return;
    }

    if(connectionState == "disconnected")
    {
        QMessageBox::warning(this,"Disconnected","You are currently not connected to the cloud.");
        return;
    }

    updateStorage();
}


void MainWindow::on_undoButton_clicked()
{
    if(connectionState == "disconnected")
    {
        QMessageBox::warning(this,"Disconnected","You are currently not connected to the cloud.");
        return;
    }

    if(deletedFiles.size() > 0)
    {
        tempFile recoveredFile = deletedFiles.at(0);
        QUrl recoveredFileUrl = QStringLiteral("%1/%2?file=%3&username=%4&api=%5").arg(baseUrl.toString(),recoveredFile.fileType,recoveredFile.fileName,currentUser.username,token);
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
    if(connectionState == "disconnected")
    {
        QMessageBox::warning(this,"Disconnected","You are currently not connected to the cloud.");
        return;
    }

    if(selectedFile != "")
    {
        QString filename = selectedFile;
        QNetworkRequest request;
        QUrl targetUrl = QStringLiteral("%1/%2?file=%3&username=%4&api=%5").arg(baseUrl.toString(),selectedFileType,selectedFile,currentUser.username,token);
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



void MainWindow::on_validateButton_clicked()
{
    QRegularExpression regEx("^[0-9]*$");
    QString portString = ui->portInput->text();
    QRegularExpressionMatch portMatch = regEx.match(portString);
    bool isNumber = portMatch.hasMatch();

    if(!isNumber)
    {
        QMessageBox::warning(this,"Invalid Port","The port should be a number, please don't use letters or special characters.");
        return;
    }

    if(portString.toInt() > 65535)
    {
        QMessageBox::warning(this,"Invalid Port","65535 is the highest value possible for a port, please select a valid port.");
        return;
    }
    else if(portString.toInt() <= 1024)
    {
        QMessageBox::warning(this,"Invalid Port","Ports from 0 to 1024 are reserved for specific services, choose from 1025 to 65535.");
        return;
    }

    PORT = portString;
    baseUrl = QUrl(QStringLiteral("http://localhost:%1").arg(PORT));
    QNetworkRequest request(baseUrl);
    QNetworkReply* reply = netManager->get(request);

    connect(reply,&QNetworkReply::finished,this,[this,reply]() ->void{
        validateConnection(reply);
    });
}

void MainWindow::validateConnection(QNetworkReply* reply)
{
    QString stateHeader = reply->rawHeader("Connection-state");
    if(stateHeader.isEmpty())
    {
        QMessageBox::warning(this,"Unable to connect","Unable to connect to the cloud, please check if the PORT is correct and if the server is on.");
        setImagePreview(nullptr);
        ui->responseDisplay->setPlainText(nullptr);
        selectedFile = "";
        selectedFileType = "";
        connectionState = "disconnected";
        ui->stateLabel->setText("Disconnected");
        ui->stateLabel->setStyleSheet(
                "background-color:rgb(36, 31, 49);"
                "border-style:outset;"
                "border-width:2px;"
                "border-color:rgb(34, 22, 37);"
                "color:rgb(165, 29, 45);"
        );
        return;
    }
    else
    {
        connectionState = stateHeader;
        ui->stateLabel->setText("Connected");
        ui->stateLabel->setStyleSheet(
              "background-color:rgb(36, 31, 49);"
              "border-style:outset;"
              "border-width:2px;"
              "border-color:rgb(34, 22, 37);"
              "color: rgb(38, 162, 105);"
        );
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if(selectedFileType == "images")
    {
        setImagePreview(nullptr);
        QUrl targetUrl = QUrl(QStringLiteral("%1/images").arg(baseUrl.toString()));
        QUrlQuery targetQuery;
        targetQuery.addQueryItem("file",selectedFile);
        targetUrl.setQuery(targetQuery);
        sendGetRequest(targetUrl);
    }
}

bool validateCredentials(QString& username, QString& password)
{
    QRegularExpression loginRegex("^[A-Za-z0-9]+$");
    QRegularExpressionMatch usernameMatch = loginRegex.match(username);
    QRegularExpressionMatch passwordMatch = loginRegex.match(password);
    return (usernameMatch.hasMatch() && passwordMatch.hasMatch());
}

void MainWindow::on_submitLogin_clicked()
{
    if(connectionState == "connected")
    {
        QUrl loginRoute = QUrl(QStringLiteral("http://localhost:%1/login").arg(PORT));
        QString username = ui->usernameInput->text();
        QString password = ui->passwordInput->text();
        bool validCredentials = validateCredentials(username,password);

        if(!validCredentials)
        {
            QMessageBox::warning(this,"Invalid credentials","Please don't use special characters neither leave empty fields on the form.");
            return;
        }

        QJsonObject credentialJSON;
        credentialJSON["username"] = username;
        credentialJSON["password"] = encryptPassword(password);
        QJsonDocument credentialDocument(credentialJSON);
        QByteArray credentialsData = credentialDocument.toJson();
        handleLogin(credentialsData,loginRoute);
        return;
    }

    QMessageBox::warning(this,"Disconnected","Please select the correct port to connect to the server and then log in.");
}

void MainWindow::handleLogin(QByteArray& data, QUrl& route)
{
    QNetworkRequest request(route);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    QNetworkReply* reply = netManager->post(request,data);
    connect(reply,&QNetworkReply::finished,this,[this,reply]() -> void {
        QByteArray JSONResponse = reply->readAll();
        QJsonDocument responseDocument = QJsonDocument::fromJson(JSONResponse);
        QString statusMessage = responseDocument["statusMessage"].toString();
        qint16 statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(statusCode == 200)
        {
            ui->stackedWidget->setCurrentIndex(0);
            QMessageBox::information(this,"Success",statusMessage);
            currentUser.isAdmin = responseDocument["isAdmin"].toBool();
            currentUser.userId = responseDocument["userId"].toInt();
            currentUser.username = responseDocument["username"].toString();
            token = responseDocument["token"].toString();
            updateStorage();
            return;
        }
        QMessageBox::warning(this,"Failed",statusMessage);
    });
}

void MainWindow::setupCredentialsTable()
{
    QNetworkRequest request(QUrl(QStringLiteral("http://localhost:%1/admin").arg(PORT)));
    QNetworkReply* reply = netManager->get(request);
    QEventLoop tableAwait;
    connect(reply,&QNetworkReply::finished,&tableAwait,&QEventLoop::quit);
    tableAwait.exec();
    QByteArray replyData = reply->readAll();
    QJsonDocument credentialsDocument = QJsonDocument::fromJson(replyData);
    QJsonArray credentialsArray = credentialsDocument.array();
    for(const QJsonValue& user : credentialsArray)
    {
        QJsonObject userObject = user.toObject();
        QTableWidgetItem* username = new QTableWidgetItem(userObject["username"].toString());
        QTableWidgetItem* password = new QTableWidgetItem(userObject["password"].toString());
        QTableWidgetItem* isAdmin = new QTableWidgetItem(((userObject["adminPrivileges"].toInt() > 0) ? "True" : "False"));
        QTableWidgetItem* id = new QTableWidgetItem(QString::number(userObject["id"].toInt()));
        ui->userCredentialsTable->insertRow(ui->userCredentialsTable->rowCount());
        ui->userCredentialsTable->setItem((ui->userCredentialsTable->rowCount() - 1),0,id);
        ui->userCredentialsTable->setItem((ui->userCredentialsTable->rowCount() - 1),1,username);
        ui->userCredentialsTable->setItem((ui->userCredentialsTable->rowCount() - 1),2,password);
        ui->userCredentialsTable->setItem((ui->userCredentialsTable->rowCount() - 1),3,isAdmin);
    }
}

void MainWindow::on_registerButton_clicked()
{
    if(connectionState == "connected")
    {
        QString username = ui->regUsername->text();
        QString password = ui->regPassword->text();
        bool validCredentials = validateCredentials(username,password);

        if(!validCredentials)
        {
            QMessageBox::warning(this,"Invalid credentials","Please don't use special characters neither leave empty fields on the form.");
            return;
        }
        QJsonObject credentialsJSON;
        credentialsJSON["username"] = username;
        credentialsJSON["password"] = encryptPassword(password);
        QJsonDocument credentialsDocument(credentialsJSON);
        QByteArray credentialsData = credentialsDocument.toJson();
        QNetworkRequest request(QUrl(QStringLiteral("http://localhost:%1/register").arg(PORT)));
        request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
        QNetworkReply* reply = netManager->post(request,credentialsData);
        connect(reply,&QNetworkReply::finished,this,&MainWindow::handleRegister);
        return;
    }

    QMessageBox::warning(this,"Disconnected","You are currently disconnected, please go back to the main panel and input the proper PORT.");
}

void MainWindow::handleRegister()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray replyData = reply->readAll();
    QString replyOutput = QString::fromUtf8(replyData);
    QMessageBox::information(this,"",replyOutput);
}

void MainWindow::on_adminPanelButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
    setupCredentialsTable();
    setupGlobalStorage();
}

void MainWindow::setupGlobalStorage()
{
   QNetworkRequest request(QUrl(QStringLiteral("http://localhost:%1/global").arg(PORT)));
   QNetworkReply* reply = netManager->get(request);
   QEventLoop replyAwait;
   connect(reply,&QNetworkReply::finished,&replyAwait,&QEventLoop::quit);
   replyAwait.exec();

   QStandardItemModel* itemModel = new QStandardItemModel(this);
   QStandardItem* users = new QStandardItem("Users");

   QByteArray replyData = reply->readAll();
   QJsonDocument globalStorageDocument = QJsonDocument::fromJson(replyData);
   QJsonObject globalStorageObject = globalStorageDocument.object();
   QJsonValue usersValue = globalStorageObject.value("users");
   QJsonArray usersArray = usersValue.toArray();
   for(const QJsonValue& userVal : usersArray)
   {
       QJsonObject user = userVal.toObject();
       QString username = user.value("user").toString();
       QJsonValue images = user.value("images");
       QJsonValue texts = user.value("texts");
       QJsonArray imageArray = images.toArray();
       QJsonArray textArray = texts.toArray();
       QStandardItem* userItem = new QStandardItem(username);
       QStandardItem* imagesItem = new QStandardItem("Images");
       QStandardItem* textsItem = new QStandardItem("Texts");
       for(const QJsonValue& image : imageArray)
       {
           QString fileName = image.toString();
           QStandardItem* fileItem = new QStandardItem(fileName);
           imagesItem->appendRow(fileItem);
       }
       for(const QJsonValue& text : textArray)
       {
           QString fileName = text.toString();
           QStandardItem* fileItem = new QStandardItem(fileName);
           textsItem->appendRow(fileItem);
       }
       userItem->appendRow(imagesItem);
       userItem->appendRow(textsItem);
       users->appendRow(userItem);
   }
    itemModel->appendRow(users);
    itemModel->setHeaderData(0,Qt::Horizontal,"Global File Viewer");
    ui->globalStorage->setModel(itemModel);
}

void MainWindow::on_mainPanelButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void MainWindow::on_loginReturn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}


void MainWindow::on_registerAccount_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
}


void MainWindow::on_logoutButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->passwordInput->setText("");
    ui->usernameInput->setText("");
    handleLogout();
}

void MainWindow::handleLogout()
{
    QNetworkRequest request(QUrl(QStringLiteral("http://localhost:%1/logout").arg(PORT,token)));
    QJsonObject logoutData;
    logoutData["token"] = token;
    logoutData["username"] = currentUser.username;
    QJsonDocument logoutDocument(logoutData);
    QByteArray logoutPayload = logoutDocument.toJson();
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    netManager->post(request,logoutPayload);
}

