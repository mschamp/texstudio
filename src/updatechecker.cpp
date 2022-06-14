#include "updatechecker.h"
#include "smallUsefulFunctions.h"
#include "utilsVersion.h"
#include "configmanager.h"
#include <QNetworkProxyFactory>
#include <QMutex>

UpdateChecker *UpdateChecker::m_Instance = nullptr;
int comboBoxUpdateLevel;

UpdateChecker::UpdateChecker() :
    QObject(nullptr), silent(true)
{
	QNetworkProxyFactory::setUseSystemConfiguration(true);
	//networkManager = new QNetworkAccessManager();
    // activate networkManager only when directly needed as it causes network delays on mac (Bug 1717/1738)
    networkManager=nullptr;
}

UpdateChecker::~UpdateChecker()
{
    m_Instance = nullptr;
}

QString UpdateChecker::lastCheckAsString()
{
	QDateTime lastCheck = ConfigManager::getInstance()->getOption("Update/LastCheck").toDateTime();
	return lastCheck.isValid() ? lastCheck.toString("d.M.yyyy hh:mm") : tr("Never", "last update");
}

void UpdateChecker::autoCheck()
{
	ConfigManagerInterface *cfg  = ConfigManager::getInstance();
	bool autoCheck = cfg->getOption("Update/AutoCheck").toBool();
	if (autoCheck) {
		QDateTime lastCheck = cfg->getOption("Update/LastCheck").toDateTime();
		int checkInterval = cfg->getOption("Update/AutoCheckInvervalDays").toInt();
		if (!lastCheck.isValid() || lastCheck.addDays(checkInterval) < QDateTime::currentDateTime()) {
			check();
		}
	}

}

void UpdateChecker::check(bool silent, int currentComboBoxUpdateLevel)
{
	// catch value if possible, s. comment at start of checkForNewVersion
	comboBoxUpdateLevel = currentComboBoxUpdateLevel;

	this->silent = silent;
    networkManager = new QNetworkAccessManager();
    QNetworkRequest request = QNetworkRequest(QUrl("https://api.github.com/repos/texstudio-org/texstudio/git/refs/tags"));
	request.setRawHeader("User-Agent", "TeXstudio Update Checker");
	QNetworkReply *reply = networkManager->get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(onRequestCompleted()));
	if (!silent)
#if QT_VERSION_MAJOR<6
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)));
#else
        connect(reply, &QNetworkReply::errorOccurred,this, &UpdateChecker::onRequestError);
#endif
}

void UpdateChecker::onRequestError(QNetworkReply::NetworkError )
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (!reply) return;

	UtilsUi::txsCritical(tr("Update check failed with error:\n") + reply->errorString());

    networkManager->deleteLater();
    networkManager=nullptr;
}

void UpdateChecker::onRequestCompleted()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (!reply || reply->error() != QNetworkReply::NoError) return;

	QByteArray ba = reply->readAll();

	parseData(ba);
	checkForNewVersion();

    networkManager->deleteLater();
    networkManager=nullptr;
}

void UpdateChecker::parseData(const QByteArray &data)
{
    // simple,dirty parsing of github api result (tags)
    QString result=QString(data);
    QStringList lines=result.split(",");
    QStringList tags;
    foreach(const QString& line,lines){
        int pos;
        if((pos=line.indexOf("\"ref\""))>=0){
            QString zw=line.mid(pos+6).simplified();
            zw.remove("\"");
            zw.remove("refs/tags/");
            tags<<zw;
        }
    }
	bool rcFound = false;
	bool devFound = false;
    for(int j=tags.length()-1;j>=0;j--){
        QString tag=tags.value(j);
        QRegExp rx("^((\\d+\\.)+(\\d+))([a-zA-Z]+)?(\\d*)?$");
        if(rx.indexIn(tag)==0){
            QString ver=rx.cap(1);
            QString type=rx.cap(4);
            qDebug()<<ver<<type;
            if (!rcFound && type.toLower() == "rc"){
                rcFound = true;
                Version v;
                v.versionNumber = ver;
                v.type = "release candidate";
                v.revision = rx.cap(5).toInt();
                latestReleaseCandidateVersion = v;
            }
            if (!devFound && type.toLower() == "beta"){
                devFound = true;
                Version v;
                v.versionNumber = ver;
                v.type = "beta";
                v.revision = rx.cap(5).toInt();
                latestDevVersion = v;
            }
            if(type.isEmpty()){
                Version v;
                v.versionNumber = ver;
                v.type = "stable";
                v.revision = 0; // unused in stable
                latestStableVersion = v;
                if(!latestDevVersion.isValid())
                    latestDevVersion=v;
                if(!latestReleaseCandidateVersion.isValid())
                    latestReleaseCandidateVersion=v;
                break; // all other versions are older, so abort after first release
            }
        }
    }
}

void UpdateChecker::checkForNewVersion()
{
	// updateLevel values from comboBoxUpdateLevel indices:
	// 0: stable, 1: release candidate, 2: development
	// config dialog (check button) passes correct current index from dialog, so user can check with different settings without closing dialog
	// auto check uses -1, since we do not have the current gui value. in this case we can stay with config value.
	int updateLevel;
	if (comboBoxUpdateLevel > -1)		
		updateLevel = comboBoxUpdateLevel;
	else
		updateLevel = ConfigManager::getInstance()->getOption("Update/UpdateLevel").toInt();

	bool checkReleaseCandidate = updateLevel >= 1;
	bool checkDevVersions = updateLevel >= 2;

	Version currentVersion = Version::current();
    QString downloadAddress = "https://texstudio.org";
    QString downloadAddressBeta = "https://github.com/texstudio-org/texstudio/releases";

	if (!latestStableVersion.isValid()) {
		if (!silent) UtilsUi::txsWarning(tr("Update check failed (invalid update file format)."));
		return;
	}
	while (true) {  // single control loop, used be able to break the control flow when some new version is detected
		if (checkReleaseCandidate && !latestReleaseCandidateVersion.isEmpty()) {
			if (!latestReleaseCandidateVersion.isValid()) {
				if (!silent) UtilsUi::txsWarning(tr("Update check for release candidate failed (invalid update file format)."));
			}
			if (latestReleaseCandidateVersion > currentVersion && latestReleaseCandidateVersion > latestStableVersion) {
				notify(QString(tr(
				                   "A new release candidate of TeXstudio is available.<br>"
				                   "<table><tr><td>Current version:</td><td>%1 (%2)</td></tr>"
				                   "<tr><td>Latest stable version:</td><td>%3 (%4)</td></tr>"
                                   "<tr><td>Release candidate:</td><td>%5 (rc%6)</td></tr>"
				                   "</table><br><br>"
				                   "You can download it from the <a href='%7'>TeXstudio website</a>."
				               )).arg(currentVersion.versionNumber).arg(currentVersion.revision)
				       .arg(latestStableVersion.versionNumber).arg(latestStableVersion.revision)
				       .arg(latestReleaseCandidateVersion.versionNumber).arg(latestReleaseCandidateVersion.revision)
                       .arg(downloadAddressBeta)
				      );
				break;
			}
		}
		if (checkDevVersions && !latestDevVersion.isEmpty()) {
			if (!latestDevVersion.isValid()) {
				if (!silent) UtilsUi::txsWarning(tr("Update check for development version failed (invalid update file format)."));
			}
			if (latestDevVersion > currentVersion && latestDevVersion > latestStableVersion) {
				notify(QString(tr(
				                   "A new development version of TeXstudio is available.<br>"
				                   "<table><tr><td>Current version:</td><td>%1 (%2)</td></tr>"
				                   "<tr><td>Latest stable version:</td><td>%3 (%4)</td></tr>"
                                   "<tr><td>Latest development version:</td><td>%5 (beta%6)</td></tr>"
				                   "</table><br><br>"
				                   "You can download it from the <a href='%7'>TeXstudio website</a>."
				               )).arg(currentVersion.versionNumber).arg(currentVersion.revision)
				       .arg(latestStableVersion.versionNumber).arg(latestStableVersion.revision)
				       .arg(latestDevVersion.versionNumber).arg(latestDevVersion.revision)
                       .arg(downloadAddressBeta)
				      );
				break;
			}
		}
		if (latestStableVersion > currentVersion) {
			notify(QString(tr(
			                   "A new version of TeXstudio is available.<br>"
			                   "<table><tr><td>Current version:</td><td>%1</td></tr>"
			                   "<tr><td>Latest stable version:</td><td>%2</td></tr>"
			                   "</table><br><br>"
			                   "You can download it from the <a href='%3'>TeXstudio website</a>."
                           )).arg(currentVersion.versionNumber,latestStableVersion.versionNumber,downloadAddress)
			      );
		} else {
			if (!silent) UtilsUi::txsInformation(tr("TeXstudio is up-to-date."));
		}
		break;
	}

	ConfigManager::getInstance()->setOption("Update/LastCheck", QDateTime::currentDateTime());
	emit checkCompleted();
}

void UpdateChecker::notify(QString message)
{
	QMessageBox msgBox;
	msgBox.setWindowTitle(tr("TeXstudio Update"));
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setText(message);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
	msgBox.exec();
}

UpdateChecker *UpdateChecker::instance()
{
	static QMutex mutex;
	mutex.lock();
	if (!m_Instance)
		m_Instance = new UpdateChecker();
	mutex.unlock();
	return m_Instance;
}
