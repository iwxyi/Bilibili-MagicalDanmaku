#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSettings>
#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include <QNetworkCookie>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCodec>
#include <stdio.h>
#include <iostream>
#include <QtWebSockets/QWebSocket>
#include <QAuthenticator>
#include <QtConcurrent/QtConcurrent>
#include <QSystemTrayIcon>
#include <QDesktopServices>
#if defined(ENABLE_HTTP_SERVER)
#include <qhttpserver.h>
#include <qhttprequest.h>
#include <qhttpresponse.h>
#endif
#include <QWebSocketServer>
#include "runtimeinfo.h"
#include "usersettings.h"
#include "accountinfo.h"
#include "platforminfo.h"
#include "coderunner.h"
#include "netutil.h"
#include "entities.h"
#include "livedanmaku.h"
#include "livedanmakuwindow.h"
#include "taskwidget.h"
#include "replywidget.h"
#include "eventwidget.h"
#include "orderplayerwindow.h"
#include "textinputdialog.h"
#include "luckydrawwindow.h"
#include "livevideoplayer.h"
#include "externalblockdialog.h"
#include "picturebrowser.h"
#include "netinterface.h"
#include "waterfloatbutton.h"
#include "custompaintwidget.h"
#include "appendbutton.h"
#include "waterzoombutton.h"
#include "tipbox.h"
#include "bili_liveopenservice.h"
#include "singleentrance.h"
#include "sqlservice.h"
#include "dbbrowser.h"
#include "m3u8downloader.h"
#include "bili_liveservice.h"
#include "web_server/webserver.h"
#include "voice_service/voiceservice.h"
#include "chat_service/chatservice.h"
#include "emailutil.h"
#include "fansarchivesservice.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define CALC_DEB if (0) qDebug() // 输出计算相关的信息
#define SERVER_DEB if (0) qDebug() // 输出服务器功能相关信息

#define CONNECT_SERVER_INTERVAL 1800000

#define SERVER_PORT 0
#define DANMAKU_SERVER_PORT 1
#define MUSIC_SERVER_PORT 2

#define PAGE_ROOM 0
#define PAGE_DANMAKU 1
#define PAGE_THANK 2
#define PAGE_MUSIC 3
#define PAGE_EXTENSION 4
#define PAGE_DATA 5
#define PAGE_PREFENCE 6

#define UPDATE_TOOL_NAME "UpUpTool.exe"

class MainWindow;

typedef void(MainWindow::*VoidFunc)();
typedef ListItemInterface*(MainWindow::*InsertItemFunc)(MyJson);

class MainWindow : public QMainWindow, public NetInterface
{
    Q_OBJECT
    Q_PROPERTY(double paletteProg READ getPaletteBgProg WRITE setPaletteBgProg)
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    struct Diange
    {
        QString nickname;
        UIDT uid;
        QString name;
        QDateTime time;
    };

    const QSettings *getSettings() const;

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent (QEvent * event) override;
    void paintEvent(QPaintEvent * event) override;

signals:
    void signalRoomChanged(QString roomId);
    void signalLiveVideoChanged(QString roomId);
    void signalNewDanmaku(const LiveDanmaku& danmaku);
    void signalRemoveDanmaku(LiveDanmaku danmaku);
    void signalCmdEvent(QString cmd, LiveDanmaku danmaku);

public slots:
    void removeTimeoutDanmaku();
    void slotRoomInfoChanged();
    void slotTryBlockDanmaku(const LiveDanmaku& danmaku);
    void slotNewGiftReceived(const LiveDanmaku& danmaku);
    void slotSendUserWelcome(const LiveDanmaku& danmaku);
    void slotSendAttentionThank(const LiveDanmaku& danmaku);
    void slotNewGuardBuy(const LiveDanmaku& danmaku);

    void processRemoteCmd(QString msg, bool response = true);
    bool execFunc(QString msg, LiveDanmaku &danmaku, CmdResponse& res, int& resVal);

    void slotSubAccountChanged(const QString& cookie, const SubAccount& subAccount);

private slots:
    void on_DiangeAutoCopyCheck_stateChanged(int);

    void on_testDanmakuButton_clicked();

    void on_removeDanmakuIntervalSpin_valueChanged(int arg1);

    void on_roomIdEdit_editingFinished();

    void on_languageAutoTranslateCheck_stateChanged(int);

    void onExtensionTabWidgetBarClicked(int index);

    void on_SendMsgButton_clicked();

    void on_AIReplyCheck_stateChanged(int);

    void on_testDanmakuEdit_returnPressed();

    void on_SendMsgEdit_returnPressed();

    void on_taskListWidget_customContextMenuRequested(const QPoint &);

    void on_replyListWidget_customContextMenuRequested(const QPoint &);

    void on_eventListWidget_customContextMenuRequested(const QPoint &);

    void addListItemOnCurrentPage();

    void slotDiange(const LiveDanmaku &danmaku);

    void slotComboSend();

    void slotSocketError(QAbstractSocket::SocketError error);

    void on_autoSendWelcomeCheck_stateChanged(int arg1);

    void on_autoSendGiftCheck_stateChanged(int arg1);

    void on_autoWelcomeWordsEdit_textChanged();

    void on_autoThankWordsEdit_textChanged();

    void on_startLiveWordsEdit_editingFinished();

    void on_endLiveWordsEdit_editingFinished();

    void on_startLiveSendCheck_stateChanged(int arg1);

    void on_autoSendAttentionCheck_stateChanged(int arg1);

    void on_autoAttentionWordsEdit_textChanged();

    void on_sendWelcomeCDSpin_valueChanged(int arg1);

    void on_sendGiftCDSpin_valueChanged(int arg1);

    void on_sendAttentionCDSpin_valueChanged(int arg1);

    void showDiangeHistory();

    void eternalBlockUser(UIDT uid, QString uname, QString msg);
    void cancelEternalBlockUser(UIDT uid);
    void cancelEternalBlockUser(UIDT uid, qint64 roomId);
    void cancelEternalBlockUserAndUnblock(UIDT uid);
    void cancelEternalBlockUserAndUnblock(UIDT uid, qint64 roomId);
    void saveEternalBlockUsers();
    void detectEternalBlockUsers();

    void on_enableBlockCheck_clicked();

    void on_newbieTipCheck_clicked();

    void on_autoBlockNewbieCheck_clicked();

    void on_autoBlockNewbieKeysEdit_textChanged();

    void on_autoBlockNewbieNotifyCheck_clicked();

    void on_autoBlockNewbieNotifyWordsEdit_textChanged();

    void on_saveDanmakuToFileCheck_clicked();

    void on_promptBlockNewbieCheck_clicked();

    void on_promptBlockNewbieKeysEdit_textChanged();

    void on_timerConnectServerCheck_clicked();

    void on_startLiveHourSpin_valueChanged(int arg1);

    void on_endLiveHourSpin_valueChanged(int arg1);

    void on_calculateDailyDataCheck_clicked();

    void on_dailyStatisticButton_clicked();

    void on_calculateCurrentLiveDataCheck_clicked();

    void on_currentLiveStatisticButton_clicked();

    void on_removeDanmakuTipIntervalSpin_valueChanged(int arg1);

    void on_doveCheck_clicked();

    void on_notOnlyNewbieCheck_clicked();

    void on_pkAutoMelonCheck_clicked();

    void on_pkMaxGoldButton_clicked();

    void on_pkJudgeEarlyButton_clicked();

    void on_roomIdEdit_returnPressed();

    void on_actionData_Path_triggered();

    void on_actionShow_Live_Danmaku_triggered();

    void on_actionSet_Cookie_triggered();

    void on_actionSet_Danmaku_Data_Format_triggered();

    void on_actionCookie_Help_triggered();

    void on_actionCreate_Video_LRC_triggered();

    void on_actionShow_Order_Player_Window_triggered();

    void on_diangeReplyCheck_clicked();

    void on_actionAbout_triggered();

    void on_actionGitHub_triggered();

    void on_actionCustom_Variant_triggered();

    void on_actionVariant_Translation_triggered();

    void on_actionSend_Long_Text_triggered();

    void on_actionShow_Lucky_Draw_triggered();

    void on_actionGet_Play_Url_triggered();

    void on_actionShow_Live_Video_triggered();

    void on_actionShow_PK_Video_triggered();

    void on_pkChuanmenCheck_clicked();

    void on_pkMsgSyncCheck_clicked();

    void on_actionMany_Robots_triggered();

    void on_judgeRobotCheck_clicked();

    void trayAction(QSystemTrayIcon::ActivationReason reason);

    void on_actionAdd_Room_To_List_triggered();

    void on_recordCheck_clicked();

    void on_recordSplitSpin_valueChanged(int arg1);

    void on_sendWelcomeTextCheck_clicked();

    void on_sendWelcomeVoiceCheck_clicked();

    void on_sendGiftTextCheck_clicked();

    void on_sendGiftVoiceCheck_clicked();

    void on_sendAttentionTextCheck_clicked();

    void on_sendAttentionVoiceCheck_clicked();

    void on_enableScreenDanmakuCheck_clicked();

    void on_enableScreenMsgCheck_clicked();

    void on_screenDanmakuWithNameCheck_clicked();

    void on_screenDanmakuLeftSpin_valueChanged(int arg1);

    void on_screenDanmakuRightSpin_valueChanged(int arg1);

    void on_screenDanmakuTopSpin_valueChanged(int arg1);

    void on_screenDanmakuBottomSpin_valueChanged(int arg1);

    void on_screenDanmakuSpeedSpin_valueChanged(int arg1);

    void on_screenDanmakuFontButton_clicked();

    void on_screenDanmakuColorButton_clicked();

    void on_autoSpeekDanmakuCheck_clicked();

    void on_speakUsernameCheck_clicked();

    void on_diangeFormatEdit_textEdited(const QString &text);

    void on_diangeNeedMedalCheck_clicked();

    void on_showOrderPlayerButton_clicked();

    void on_diangeShuaCheck_clicked();

    void on_orderSongShuaSpin_editingFinished();

    void on_pkMelonValButton_clicked();

    void slotStartWork();

    void on_autoSwitchMedalCheck_clicked();

    void on_sendAutoOnlyLiveCheck_clicked();

    void on_autoDoSignCheck_clicked();

    void on_actionRoom_Status_triggered();

    void on_autoLOTCheck_clicked();

    void on_blockNotOnlyNewbieCheck_clicked();

    void on_autoBlockTimeSpin_editingFinished();

    void triggerCmdEvent(QString cmd, LiveDanmaku danmaku, bool debug = false);

    void on_voiceLocalRadio_toggled(bool checked);

    void on_voiceXfyRadio_toggled(bool checked);

    void on_voiceMSRadio_toggled(bool checked);

    void on_voiceCustomRadio_toggled(bool checked);

    void on_voiceNameEdit_editingFinished();

    void on_voiceNameSelectButton_clicked();

    void on_voicePitchSlider_valueChanged(int value);

    void on_voiceSpeedSlider_valueChanged(int value);

    void on_voiceVolumeSlider_valueChanged(int value);

    void on_voicePreviewButton_clicked();

    void on_voiceLocalRadio_clicked();

    void on_voiceXfyRadio_clicked();

    void on_voiceMSRadio_clicked();

    void on_voiceCustomRadio_clicked();

    void on_label_10_linkActivated(const QString &link);

    void on_xfyAppIdEdit_textEdited(const QString &text);

    void on_xfyApiSecretEdit_textEdited(const QString &text);

    void on_xfyApiKeyEdit_textEdited(const QString &text);

    void on_voiceCustomUrlEdit_editingFinished();

    void on_eternalBlockListButton_clicked();

    void on_AIReplyMsgCheck_clicked();

    void on_AIReplySelfCheck_clicked();

    void slotAIReplyed(QString reply, LiveDanmaku danmaku);

    void on_danmuLongestSpin_editingFinished();

    void on_startupAnimationCheck_clicked();
#if defined (ENABLE_HTTP_SERVER)
    void serverHandle(QHttpRequest *req, QHttpResponse *resp);

    void requestHandle(QHttpRequest *req, QHttpResponse *resp);

    void serverHandleUrl(const QString &urlPath, QHash<QString, QString> &params, QHttpRequest *req, QHttpResponse *resp);
#endif
    void on_serverCheck_clicked();

    void on_serverPortSpin_editingFinished();

    void on_autoPauseOuterMusicCheck_clicked();

    void on_outerMusicKeyEdit_textEdited(const QString &arg1);

    void on_acquireHeartCheck_clicked();

    void on_sendExpireGiftCheck_clicked();

    void on_actionPicture_Browser_triggered();

    void on_orderSongsToFileCheck_clicked();

    void on_orderSongsToFileFormatEdit_textEdited(const QString &arg1);

    void on_orderSongsToFileMaxSpin_editingFinished();

    void on_playingSongToFileCheck_clicked();

    void on_playingSongToFileFormatEdit_textEdited(const QString &arg1);

    void on_actionCatch_You_Online_triggered();

    void on_pkBlankButton_clicked();

    void on_actionUpdate_New_Version_triggered();

    void on_startOnRebootCheck_clicked();

    void on_domainEdit_editingFinished();

    void prepareQuit();

    void on_giftComboSendCheck_clicked();

    void on_giftComboDelaySpin_editingFinished();

    void on_retryFailedDanmuCheck_clicked();

    void on_songLyricsToFileCheck_clicked();

    void on_songLyricsToFileMaxSpin_editingFinished();

    void on_allowWebControlCheck_clicked();

    void on_saveEveryGuardCheck_clicked();

    void on_saveMonthGuardCheck_clicked();

    void on_saveEveryGiftCheck_clicked();

    void on_exportDailyButton_clicked();

    void on_exportCurrentLiveButton_clicked();

    void on_closeTransMouseButton_clicked();

    void on_pkAutoMaxGoldCheck_clicked();

    void on_allowRemoteControlCheck_clicked();

    void on_actionJoin_Battle_triggered();

    void on_actionQRCode_Login_triggered();

    void on_allowAdminControlCheck_clicked();

    void on_actionSponsor_triggered();

    void on_actionPaste_Code_triggered();

    void on_actionGenerate_Default_Code_triggered();

    void on_actionRead_Default_Code_triggered();

    void on_giftComboTopCheck_clicked();

    void on_giftComboMergeCheck_clicked();

    void on_listenMedalUpgradeCheck_clicked();

    void on_pkChuanmenCheck_stateChanged(int arg1);

    void on_actionLast_Candidate_triggered();

    void on_actionLocal_Mode_triggered();

    void on_actionDebug_Mode_triggered();

    void on_actionGuard_Online_triggered();

    void on_actionOfficial_Website_triggered();

    void on_actionAnchor_Case_triggered();

    void on_robotNameButton_clicked();

    void on_thankWelcomeTabButton_clicked();

    void on_thankGiftTabButton_clicked();

    void on_thankAttentionTabButton_clicked();

    void on_blockTabButton_clicked();

    void on_sendMsgMoreButton_clicked();

    void on_showLiveDanmakuWindowButton_clicked();

    void on_liveStatusButton_clicked();

    void on_tenCardLabel1_linkActivated(const QString &link);

    void on_tenCardLabel2_linkActivated(const QString &link);

    void on_tenCardLabel3_linkActivated(const QString &link);

    void on_tenCardLabel4_linkActivated(const QString &link);

    void on_musicBlackListButton_clicked();

    void on_enableFilterCheck_clicked();

    void on_actionReplace_Variant_triggered();

    void on_autoClearComeIntervalSpin_editingFinished();

    void on_roomDescriptionBrowser_anchorClicked(const QUrl &arg1);

    void on_adjustDanmakuLongestCheck_clicked();

    void on_actionBuy_VIP_triggered();

    void on_droplight_clicked();

    void on_vipExtensionButton_clicked();

    void on_vipDatabaseButton_clicked();

    void on_enableTrayCheck_clicked();

    void on_toutaGiftCheck_clicked();

    void on_toutaGiftCountsEdit_textEdited(const QString &arg1);

    void on_toutaGiftListButton_clicked();

    void on_timerConnectIntervalSpin_editingFinished();

    void on_heartTimeSpin_editingFinished();

    void on_syncShieldKeywordCheck_clicked();

    void on_roomCoverSpacingLabel_customContextMenuRequested(const QPoint &);

    void on_upHeaderLabel_customContextMenuRequested(const QPoint &);

    void on_saveEveryGiftButton_clicked();

    void on_saveEveryGuardButton_clicked();

    void on_saveMonthGuardButton_clicked();

    void on_musicConfigButton_clicked();

    void on_musicConfigStack_currentChanged(int arg1);

    void on_addMusicToLiveButton_clicked();

    void on_roomNameLabel_customContextMenuRequested(const QPoint &);

    void on_upNameLabel_customContextMenuRequested(const QPoint &);

    void on_roomAreaLabel_customContextMenuRequested(const QPoint &);

    void on_tagsButtonGroup_customContextMenuRequested(const QPoint &);

    void on_roomDescriptionBrowser_customContextMenuRequested(const QPoint &);

    void on_upLevelLabel_customContextMenuRequested(const QPoint &);

    void on_refreshExtensionListButton_clicked();

    void on_droplight_customContextMenuRequested(const QPoint &);

    void on_autoUpdateCheck_clicked();

    void on_showChangelogCheck_clicked();

    void on_updateBetaCheck_clicked();

    void on_dontSpeakOnPlayingSongCheck_clicked();

    void on_shieldKeywordListButton_clicked();

    void on_saveEveryGuardButton_customContextMenuRequested(const QPoint &pos);

    void exportAllGuardsByMonth(QString exportPath);

    void on_setCustomVoiceButton_clicked();

    void on_MSAreaCodeEdit_editingFinished();

    void on_MSSubscriptionKeyEdit_editingFinished();

    void on_MS_TTS__SSML_Btn_clicked();

    void on_receivePrivateMsgCheck_clicked();

    void on_receivePrivateMsgCheck_stateChanged(int arg1);

    void on_processUnreadMsgCheck_clicked();

    void on_TXSecretIdEdit_editingFinished();

    void on_TXSecretKeyEdit_editingFinished();

    void on_saveDanmakuToFileButton_clicked();

    void on_calculateDailyDataButton_clicked();

    void on_calculateCurrentLiveDataButton_clicked();

    void on_syntacticSugarCheck_clicked();

    void on_forumButton_clicked();

    void on_complexCalcCheck_clicked();

    void on_positiveVoteCheck_clicked();

    void on_saveLogCheck_clicked();

    void on_stringSimilarCheck_clicked();

    void on_onlineRankListWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_actionShow_Gift_List_triggered();

    void on_liveOpenCheck_clicked();

    void on_identityCodeEdit_editingFinished();

    void on_recordDataButton_clicked();

    void on_actionLogout_triggered();

    void on_saveToSqliteCheck_clicked();

    void on_databaseQueryButton_clicked();

    void on_actionQueryDatabase_triggered();

    void on_saveCmdToSqliteCheck_clicked();

    void on_recordFormatCheck_clicked();

    void on_ffmpegButton_clicked();

    void on_closeGuiCheck_clicked();

    void on_chatGPTKeyEdit_textEdited(const QString &arg1);

    void on_chatGPTMaxTokenCountSpin_valueChanged(int arg1);

    void on_chatGPTMaxContextCountSpin_valueChanged(int arg1);

    void on_chatGPTModelNameCombo_activated(const QString &arg1);

    void on_chatGPTKeyButton_clicked();

    void on_chatGPTRadio_clicked();

    void on_chatTxRadio_clicked();

    void on_chatGPTPromptButton_clicked();

    void on_GPTAnalysisPromptButton_clicked();

    void on_GPTAnalysisCheck_clicked();

    void on_GPTAnalysisFormatButton_clicked();

    void on_removeLongerRandomDanmakuCheck_clicked();

    void on_GPTAnalysisEventButton_clicked();

    void on_emailDriverCombo_activated(const QString &arg1);

    void on_emailHostEdit_editingFinished();

    void on_emailPortSpin_editingFinished();

    void on_emailFromEdit_editingFinished();

    void on_emailPasswordEdit_editingFinished();

    void on_proxyTypeCombo_activated(const QString &arg1);

    void on_proxyHostEdit_editingFinished();

    void on_proxyPortSpin_editingFinished();

    void on_proxyUsernameEdit_editingFinished();

    void on_proxyPasswordEdit_editingFinished();

    void on_chatGPTEndpointEdit_textEdited(const QString &arg1);

    void on_fansArchivesCheck_clicked();

    void on_fansArchivesTabButton_clicked();

    void on_databaseTabButton_clicked();

    void on_refreshFansArchivesButton_clicked();

    void on_fansArchivesByRoomCheck_clicked();

    void on_clearFansArchivesButton_clicked();

    void on_screenMonitorCombo_activated(int index);

    void on_chatGPTModelNameCombo_editTextChanged(const QString &arg1);

    void on_UAButton_clicked();

    void on_addSubAccountButton_clicked();

    void on_refreshSubAccountButton_clicked();

    void on_subAccountDescButton_clicked();

    void on_subAccountTableWidget_customContextMenuRequested(const QPoint &pos);

    void on_proxyTestIPButton_clicked();

    void on_platformButton_clicked();

    void on_actionWebViewLogin_triggered();

private:
    void initView();
    void initStyle();
    void initObject();
    void initPath();
    void initLiveService();
    void adjustWidgetsByPlatform();
    void readConfig();
    void readConfig2();
    void initDanmakuWindow();
    void initEvent();
    void initCodeRunner();
    void initWebServer();
    void initVoiceService();
    void initChatService();
    void initDynamicConfigs();
    void adjustPageSize(int page);
    void switchPageAnimation(int page);

    void oldLiveDanmakuRemoved(const LiveDanmaku &danmaku);
    void localNotify(const QString &text);
    void localNotify(const QString &text, UIDT uid);

    TaskWidget *addTimerTask(bool enable, int second, QString text, int index = -1);
    TaskWidget *addTimerTask(const MyJson &json);
    void connectTimerTaskEvent(TaskWidget *tw, QListWidgetItem* item);
    void saveTaskList();
    void restoreTaskList();

    ReplyWidget *addAutoReply(bool enable, QString key, QString reply, int index= -1);
    ReplyWidget *addAutoReply(const MyJson &json);
    void connectAutoReplyEvent(ReplyWidget* rw, QListWidgetItem* item);
    void saveReplyList();
    void restoreReplyList();
    bool hasReply(const QString& text);
    bool gotoReply(const QString& text);

    EventWidget *addEventAction(bool enable, QString cmd, QString action, int index= -1);
    EventWidget *addEventAction(const MyJson &json);
    void connectEventActionEvent(EventWidget* tw, QListWidgetItem* item);
    void saveEventList();
    void restoreEventList();
    bool hasEvent(const QString &cmd) const;
    bool gotoEvent(const QString& text);

    template<class T>
    void showListMenu(QListWidget* listWidget, QString listKey, VoidFunc saveFunc);
    void addCodeSnippets(const QJsonDocument &doc);

    void autoSetCookie(const QString &s);
    QVariant getCookies() const;
    void saveSubAccount();
    void restoreSubAccount();
    void updateSubAccount();
    void refreshUndetectedSubAccount();
    QString getDomainPort() const;
    void startConnectIdentityCode();
    void startConnectRoom();

    void updatePermission();
    int hasPermission();

    void setRoomCover(const QPixmap &pixmap);
    void setRoomThemeByCover(double val);
    void adjustCoverSizeByRoomCover(QPixmap pixmap);
    void adjustRoomIdWidgetPos();
    void showRoomIdWidget();
    void hideRoomIdWidget();
    void userComeEvent(LiveDanmaku& danmaku);
    void updateOnlineRankGUI();
    void setRoomDescription(QString roomDescription);

    double getPaletteBgProg() const;
    void setPaletteBgProg(double x);

    void sendWelcomeIfNotRobot(LiveDanmaku danmaku);
    void sendAttentionThankIfNotRobot(LiveDanmaku danmaku);
    void sendWelcome(LiveDanmaku danmaku);
    void sendAttentionThans(LiveDanmaku danmaku);
    void judgeRobotAndMark(LiveDanmaku danmaku);
    void showScreenDanmaku(const LiveDanmaku &danmaku);

    void restoreToutaGifts(QString text);
    void initLiveRecord();
    void startLiveRecord();
    void startRecordUrl(QString url);
    void finishLiveRecord();

    void restoreCustomVariant(QString text);
    QString saveCustomVariant();
    void restoreVariantTranslation();

    void restoreReplaceVariant(QString text);
    QString saveReplaceVariant();

    void savePlayingSong();
    void saveOrderSongs(const SongList& songs);
    void saveSongLyrics();

    void saveMonthGuard();
    void saveEveryGuard(LiveDanmaku danmaku);
    void saveEveryGift(LiveDanmaku danmaku);
    void appendFileLine(QString filePath, QString format, LiveDanmaku danmaku);

    void releaseLiveData(bool prepare = false);
    QRect getScreenRect();
    void loadScreenMonitors();

    void getPositiveVote();

    void startSplash();
    void loadWebExtensionList();
    void shakeWidget(QWidget* widget);

    void saveGameNumbers(int channel);
    void restoreGameNumbers();
    void saveGameTexts(int channel);
    void restoreGameTexts();

    virtual void setUrlCookie(const QString &url, QNetworkRequest *request) override;
    void openLink(QString link);
    void addGuiGiftList(const LiveDanmaku& danmaku);

    void initServerData();
    void openServer(int port = 0);
    void openSocketServer();
    void processSocketTextMsg(QWebSocket* clientSocket, const QString& message);
    void closeServer();
    void sendDanmakuToSockets(QString cmd, LiveDanmaku danmaku);
    void sendJsonToSockets(QString cmd, QJsonValue data, QWebSocket* socket = nullptr);
    void processServerVariant(QByteArray& doc);
#if defined(ENABLE_HTTP_SERVER)
    QByteArray processApiRequest(QString url, QHash<QString, QString> params, QString *contentType, QHttpRequest *req, QHttpResponse *resp);
#endif
    void sendTextToSockets(QString cmd, QByteArray data, QWebSocket* socket = nullptr);
    void sendMusicList(const SongList& songs, QWebSocket* socket = nullptr);
    void sendLyricList(QWebSocket* socket = nullptr);
    QString webCache(QString name) const;

    void syncMagicalRooms();
    void pullRoomShieldKeyword();
    void addCloudShieldKeyword(QString keyword);
    void downloadNewPackage(QString version, QString packageUrl);

    QString GetFileVertion(QString fullName);
    void upgradeVersionToLastest(QString oldVersion);
    void upgradeOneVersionData(QString beforeVersion);
    static bool hasInstallVC2015();

    void generateDefaultCode(QString path = "");
    void readDefaultCode(QString path = "");
    void showError(QString title, QString s) const;
    void showError(QString s) const;
    void showNotify(QString title, QString s) const;
    void showNotify(QString s) const;

    void initDbService();
    void showSqlQueryResult(QString sql);

    void sendEmail(const QString& to, const QString& subject, const QString& body);

    void setNetworkProxy();

    void initFansArchivesService();
    void updateFansArchivesListView();
    void loadFansArchives(QString uid);

private:
    // 应用信息
    Ui::MainWindow *ui;

    // 控件
    QList<WaterZoomButton*> sideButtonList;
    QLabel* roomCoverLabel;
    QWidget* roomIdBgWidget;
    InteractiveButtonBase* roomSelectorBtn;
    QList<WaterFloatButton*> thankTabButtons;
    QList<WaterFloatButton*> dataCenterTabButtons;
    InteractiveButtonBase* customVarsButton;
    InteractiveButtonBase* extensionButton;
    CustomPaintWidget* musicTitleDecorateWidget;
    AppendButton* appendListItemButton;
    QLabel* statusLabel;
    TipBox* tip_box;
    InteractiveButtonBase* droplight;
    SingleEntrance* fakeEntrance = nullptr;
    
    // 直播数据
    LiveServiceBase* liveService = nullptr;

    // 启动与定时
    QTimer* syncTimer = nullptr;
    QTimer* liveTimeTimer = nullptr;

    // 动画
    double paletteProg = 0;
    BFSColor prevPa;
    BFSColor currentPa;

    // 颜色
    QColor themeBg = Qt::white;
    QColor themeFg = Qt::black;
    QColor themeSbg = Qt::white;
    QColor themeSfg = Qt::blue;
    QColor themeGradient = Qt::white;

    // 弹幕信息
    LiveDanmakuWindow* danmakuWindow = nullptr;
    QTimer* removeTimer;

    // 点歌
    bool diangeAutoCopy = false;
    QList<Diange> diangeHistory;
    QString diangeFormatString;
    OrderPlayerWindow* musicWindow = nullptr;

    // 服务信息
    QString SERVER_DOMAIN = LOCAL_MODE ? "http://localhost:8102" : "http://iwxyi.com:8102";
    QString serverPath = SERVER_DOMAIN + "/server/";
    int permissionLevel = 0;
    bool permissionType[20] = {};
    QTimer* permissionTimer = nullptr;
    QString permissionText = "捐赠版";
    qint64 permissionDeadline = 0;

    // 录播
    qint64 startRecordTime = 0;
    QString recordUrl;
    QEventLoop* recordLoop = nullptr;
    QTimer* recordTimer = nullptr;
    QProcess* recordConvertProcess = nullptr;
    QString recordLastPath;
    M3u8Downloader m3u8Downloader;

    // 抽奖机
    LuckyDrawWindow* luckyDrawWindow = nullptr;

    // 视频
    LiveVideoPlayer* videoPlayer = nullptr;

    // 点歌
    QStringList orderSongBlackList;

    // 托盘
    QSystemTrayIcon *tray;//托盘图标添加成员

    // 文字转语音
    VoiceService* voiceService = nullptr;

    // 聊天
    ChatService* chatService = nullptr;

    // 全屏弹幕
    QFont screenDanmakuFont;
    QColor screenDanmakuColor;
    QList<QLabel*> screenLabels;
    int screenDanmakuIndex = -1;

    // 服务端
    WebServer* webServer;

    // 截图管理
    PictureBrowser* pictureBrowser = nullptr;

    // 彩蛋
    QString warmWish;

    // 数据库
    SqlService sqlService;

    // 邮件
    EmailUtil* emailService = nullptr;

    // 粉丝档案
    FansArchivesService* fansArchivesService = nullptr;

    // 子账号
    bool _flag_detectingAllSubAccount = false;
};

class RequestBodyHelper : public QObject
{
    Q_OBJECT
#if defined(ENABLE_HTTP_SERVER)
public:
    RequestBodyHelper(QHttpRequest* req, QHttpResponse* resp)
        : req(req), resp(resp)
    {
    }

    void waitBody()
    {
        req->storeBody();
        connect(req, SIGNAL(end()), this, SLOT(end()));
    }

public slots:
    void end()
    {
        // qInfo() << "httprequest.end.body: " << req->body();
        emit finished(req, resp);
        this->deleteLater();
    }

signals:
    void finished(QHttpRequest* req, QHttpResponse* resp);

private:
    QHttpRequest* req;
    QHttpResponse* resp;
#endif
};

#endif // MAINWINDOW_H
