﻿#include "weatherplugin.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>

WeatherPlugin::WeatherPlugin(QObject *parent)
    : QObject(parent),
      m_tipsLabel(new QLabel),
      m_refershTimer(new QTimer(this)),
      m_settings("deepin", "dde-dock-HTYWeather")
{
    m_tipsLabel->setObjectName("HTYWeather");
    m_tipsLabel->setStyleSheet("color:white; padding:0px 3px;");

    forcastApplet = new ForcastWidget(m_settings.value("theme", "hty").toString(), nullptr);
    m_centralWidget = new WeatherWidget(forcastApplet, nullptr);
    forcastApplet->setObjectName("forcast");
    forcastApplet->setVisible(false);
    connect(m_centralWidget, &WeatherWidget::requestContextMenu, [this] { m_proxyInter->requestContextMenu(this, QString()); });
    connect(m_centralWidget, &WeatherWidget::requestUpdateGeometry, [this] { m_proxyInter->itemUpdate(this, QString()); });
    connect(forcastApplet, SIGNAL(weatherNow(QString,QString,QString,QPixmap)), this, SLOT(weatherNow(QString,QString,QString,QPixmap)));
    forcastApplet->updateWeather();

    m_refershTimer->setInterval(3600000);
    m_refershTimer->start();
    connect(m_refershTimer, &QTimer::timeout, forcastApplet, &ForcastWidget::updateWeather);

}

const QString WeatherPlugin::pluginName() const
{
    return "HTYWeather";
}

const QString WeatherPlugin::pluginDisplayName() const
{
    return "HTYWeather";
}

void WeatherPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
    if (m_centralWidget->enabled())
        m_proxyInter->itemAdded(this, QString());
}

void WeatherPlugin::pluginStateSwitched()
{
    m_centralWidget->setEnabled(!m_centralWidget->enabled());
    if (m_centralWidget->enabled())
        m_proxyInter->itemAdded(this, QString());
    else
        m_proxyInter->itemRemoved(this, QString());
}

bool WeatherPlugin::pluginIsDisable()
{
    return !m_centralWidget->enabled();
}

int WeatherPlugin::itemSortKey(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    const QString key = QString("pos_%1").arg(displayMode());
    return m_settings.value(key, 0).toInt();
}

void WeatherPlugin::setSortKey(const QString &itemKey, const int order)
{
    Q_UNUSED(itemKey);

    const QString key = QString("pos_%1").arg(displayMode());
    m_settings.setValue(key, order);
}

QWidget *WeatherPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_centralWidget;
}

QWidget *WeatherPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_tipsLabel;
}

QWidget *WeatherPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return forcastApplet;
}

const QString WeatherPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    QList<QVariant> items;
    items.reserve(0);

    QMap<QString, QVariant> about;
    about["itemId"] = "about";
    about["itemText"] = "About";
    about["isActive"] = true;
    items.push_back(about);

    QMap<QString, QVariant> set;
    set["itemId"] = "set";
    set["itemText"] = "Settings";
    set["isActive"] = true;
    items.push_back(set);

    QMap<QString, QVariant> themeItem;
    themeItem["itemId"] = "theme";
    themeItem["itemText"] = "Themes";
    themeItem["isActive"] = true;
    items.push_back(themeItem);

    QMap<QString, QVariant> refresh;
    refresh["itemId"] = "refresh";
    refresh["itemText"] = "refresh";
    refresh["isActive"] = true;
    items.push_back(refresh);

    QMap<QString, QVariant> satalite;
    satalite["itemId"] = "map";
    satalite["itemText"] = "Clouds Map";
    satalite["isActive"] = true;
    items.push_back(satalite);

    QMap<QString, QVariant> log;
    log["itemId"] = "log";
    log["itemText"] = "Log";
    log["isActive"] = true;
    items.push_back(log);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;
    return QJsonDocument::fromVariant(menu).toJson();
}

void WeatherPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey);
    Q_UNUSED(checked);

    if(menuId=="about"){
        MBAbout();
    }else if(menuId=="set"){
        set();
    }else if(menuId=="refresh"){
        forcastApplet->updateWeather();
        m_refershTimer->start();
    }else if(menuId=="map"){
        showMap();
    }else if(menuId=="log"){
        showLog();
    }
    else if(menuId=="theme"){
        changeTheme();
    }

}

const QStringList WeatherPlugin::themeSet({"hty", "hty_resized", "gray"});
void WeatherPlugin::changeTheme() {
    QDialog *dialog = new QDialog;
    dialog->setWindowTitle("Change Theme");
    QVBoxLayout *vbox = new QVBoxLayout;
    QHBoxLayout *hbox = new QHBoxLayout;

    QLabel *themeHint = new QLabel("Select Theme: ");
    hbox->addWidget(themeHint);
    QComboBox *themeSelect = new QComboBox();
    themeSelect->addItems(themeSet);
    themeSelect->setCurrentText(forcastApplet->theme());
    hbox->addWidget(themeSelect);
    vbox->addLayout(hbox);

    QDialogButtonBox *buttons = new QDialogButtonBox(
                QDialogButtonBox::Ok|QDialogButtonBox::Cancel, dialog);
    connect(buttons, SIGNAL(rejected()), dialog, SLOT(reject()));
    connect(buttons, SIGNAL(accepted()), dialog, SLOT(accept()));
    vbox->addWidget(buttons);

    dialog->setLayout(vbox);
    if(dialog->exec() == QDialog::Accepted){
        QString thm = themeSelect->currentText();
        // qDebug() << "accept theme" << thm;
        m_settings.setValue("theme", thm);
        forcastApplet->setTheme(thm);
        forcastApplet->updateWeather(); // TODO: seperate plotting and updating
    }
    dialog->close();
}

void WeatherPlugin::MBAbout()
{
    QMessageBox aboutMB(QMessageBox::NoIcon, "HTYWeather 5.1", "About\n\n"
                        "Deepin Linux Dock Weather Plugin.\n"
                        "Maintainer: CareF\nE-mail: me@mail.caref.xyz\n"
                        "Original Author: 黄颖\nE-mail: sonichy@163.com\n"
                        "Source: https://github.com/sonichy/WEATHER_DDE_DOCK\n"
                        "API: https://openweathermap.org/forecast5");
    aboutMB.setIconPixmap(QPixmap(":/hty/icon/01d.png"));
    aboutMB.exec();
}

void WeatherPlugin::weatherNow(QString weather, QString temp, QString stip, QPixmap pixmap)
{
    m_centralWidget->sw = weather;
    m_centralWidget->temp = temp;
    m_centralWidget->pixmap = pixmap;
    m_tipsLabel->setText(stip);
}

void WeatherPlugin::showMap()
{
    QLabel *label = new QLabel;
    label->setWindowTitle("Clouds Map");
    label->setWindowFlags(Qt::Tool);
    QString appid = "8f3c852b69f0417fac76cd52c894ba63";
    QString surl = "https://tile.openweathermap.org/map/clouds_new/10/" + m_settings.value("lat","").toString() + "/" + m_settings.value("lon","").toString()+ ".png?appid=" + appid;

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString log = currentDateTime.toString("yyyy/MM/dd HH:mm:ss") + " : " + surl;
    QString path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first() + "/HTYWeather.log";
    QFile file(path);
    if (file.open(QFile::WriteOnly | QFile::Append)) {
        file.write(log.toUtf8());
        file.close();
    }

    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply;
    reply = manager.get(QNetworkRequest(QUrl(surl)));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QByteArray BA = reply->readAll();
    QPixmap pixmap;
    pixmap.loadFromData(BA);
    label->resize(pixmap.size());
    label->move((QApplication::desktop()->width() - label->width())/2, (QApplication::desktop()->height() - label->height())/2);
    label->setPixmap(pixmap);
    label->show();
}

void WeatherPlugin::showLog()
{
    QString surl = "file://" + QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first() + "/HTYWeather.log";
    QDesktopServices::openUrl(QUrl(surl));
}

void WeatherPlugin::set()
{
    QDialog *dialog = new QDialog;
    dialog->setWindowTitle("Set");
    QVBoxLayout *vbox = new QVBoxLayout;
    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *label = new QLabel("City");
    hbox->addWidget(label);
    QLineEdit *lineEdit = new QLineEdit;
    lineEdit->setPlaceholderText("English Only");
    QRegExp regExp("[a-zA-Z]+$");
    QValidator *validator = new QRegExpValidator(regExp,lineEdit);
    lineEdit->setValidator(validator);
    lineEdit->setText(m_settings.value("city","").toString());
    hbox->addWidget(lineEdit);
    label = new QLabel("Country");
    hbox->addWidget(label);
    QComboBox *comboBox = new QComboBox;
    QString country_codes = "AF,AX,AL,DZ,AS,AD,AO,AI,AQ,AG,AR,AM,AW,AU,AT,AZ,BS,BH,BD,BB,BY,BE,BZ,BJ,BM,BT,BO,BQ,BA,BW,BV,BR,IO,BN,BG,BF,BI,KH,CM,CA,CV,KY,CF,TD,CL,CN,CX,CC,CO,KM,CD,CG,CK,CR,CI,HR,CU,CW,CY,CZ,DK,DJ,DM,DO,EC,EG,SV,GQ,ER,EE,ET,FK,FO,FJ,FI,FR,GF,PF,TF,GA,GM,GE,DE,GH,GI,GR,GL,GD,GP,GU,GT,GG,GW,GN,GY,HT,HM,VA,HN,HK,HU,IS,IN,ID,IR,IQ,IE,IM,IL,IT,JM,JP,JE,JO,KZ,KE,KI,KP,KR,KW,KG,LA,LV,LB,LS,LR,LY,LI,LT,LU,MO,MK,MG,MW,MY,MV,ML,MT,MH,MQ,MR,MU,YT,MX,FM,MD,MC,MN,ME,MS,MA,MZ,MM,NA,NR,NP,NL,NC,NZ,NI,NG,NE,NU,NF,MP,NO,OM,PK,PW,PS,PA,PG,PY,PE,PH,PN,PL,PT,PR,QA,RE,RO,RU,RW,BL,SH,KN,LC,MF,PM,VC,WS,SM,ST,SA,SN,RS,SC,SL,SG,SX,SK,SI,SB,SO,ZA,GS,SS,ES,LK,SD,SR,SJ,SZ,SE,CH,SY,TW,TJ,TZ,TH,TL,TG,TK,TO,TT,TN,TR,TM,TC,TV,UG,UA,AE,GB,UM,US,UY,UZ,VU,VE,VN,VG,VI,WF,EH,YE,ZM,ZW";
    QStringList SL;
    SL = country_codes.split(",");
    SL.sort();
    comboBox->addItems(SL);
    comboBox->setCurrentText(m_settings.value("country","").toString());
    hbox->addWidget(comboBox);
    vbox->addLayout(hbox);
    hbox = new QHBoxLayout;
    label = new QLabel("Search your city and country in openweathermap.org");
    hbox->addWidget(label);
    vbox->addLayout(hbox);
    QPushButton *pushButton_confirm = new QPushButton("Confirm");
    QPushButton *pushButton_cancel = new QPushButton("Cancel");
    connect(pushButton_confirm, SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(pushButton_cancel, SIGNAL(clicked()), dialog, SLOT(reject()));
    hbox = new QHBoxLayout;
    hbox->addWidget(pushButton_confirm);
    hbox->addWidget(pushButton_cancel);
    vbox->addLayout(hbox);
    dialog->setLayout(vbox);
    if(dialog->exec() == QDialog::Accepted){
        m_settings.setValue("city", lineEdit->text());
        m_settings.setValue("country", comboBox->currentText());
        forcastApplet->updateWeather();
    }
    dialog->close();
}
