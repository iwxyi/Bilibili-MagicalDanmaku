#include <zlib.h>
#include <QListView>
#include <QMovie>
#include <QClipboard>
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QDataStream>
#include <QTableWidget>
#include <QSqlQuery>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "videolyricscreator.h"
#include "roomstatusdialog.h"
#include "RoundedAnimationLabel.h"
#include "catchyouwidget.h"
#include "qrcodelogindialog.h"
#include "fileutil.h"
#include "stringutil.h"
#include "escape_dialog/escapedialog.h"
#include "guardonlinedialog.h"
#include "watercirclebutton.h"
#include "variantviewer.h"
#include "csvviewer.h"
#include "buyvipdialog.h"
#include "warmwishutil.h"
#include "httpuploader.h"
#include "third_party/qss_editor/qsseditdialog.h"
#ifdef Q_OS_WIN
#include "widgets/windowshwnd.h"
#endif
#include "tx_nlp.h"
#include "order_player/roundedpixmaplabel.h"
#include "string_distance_util.h"
#include "bili_api_util.h"
#include "pixmaputil.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      NetInterface(this),
      ui(new Ui::MainWindow),
      sqlService(this)
{
    ui->setupUi(this);
    resize(800, 600);
    rt->mainwindow = this;
    cr->setMainUI(ui);

    initView();
    initStyle();

    initPath();
    initObject();
    initLiveService();
    initEvent();
    initLiveOpenService();
    initDbService();
    initCodeRunner();
    initWebServer();
    initVoiceService();
    initChatService();
    readConfig();

#ifdef Q_OS_ANDROID
    ui->sideBarWidget->hide();
    ui->verticalLayout->addWidget(ui->robotInfoWidget);
    ui->droplight->hide();
    ui->guardInfoWidget->hide();
    ui->roomPage->layout()->setMargin(0);
    ui->danmakuPage->layout()->setMargin(0);
    ui->thankPage->layout()->setMargin(0);
    ui->musicPage->layout()->setMargin(0);
    ui->extensionPage->layout()->setMargin(0);
    ui->preferencePage->layout()->setMargin(0);
#endif

    // 彩蛋
    warmWish = WarmWishUtil::getWarmWish(":/documents/warm_wish");
    if (!warmWish.isEmpty())
    {
        ui->roomNameLabel->setText(warmWish);

        QTimer::singleShot(500 * warmWish.length(), [=]{
            ui->roomNameLabel->setText(ac->roomTitle);
        });
    }

    QTimer::singleShot(0, [=]{
        triggerCmdEvent("START_UP", LiveDanmaku(), true);
    });

#ifdef Q_OS_WIN
    // 检测VC2015
    QTimer::singleShot(1000, [=]{
        if (hasInstallVC2015())
            return ;

        // 提示下载安装
        auto notify = new NotificationEntry("", "下载VC运行库", "系统缺少必须的VC2015");
        connect(notify, &NotificationEntry::signalCardClicked, this, [=]{
            QDesktopServices::openUrl(QUrl("https://aka.ms/vs/15/release/vc_redist.x64.exe"));
        });
        tip_box->createTipCard(notify);
    });
#endif

    // 检查更新
    syncTimer->start((qrand() % 10 + 10) * 1000);

    // ========== 测试 ==========
}

void MainWindow::initView()
{
    QApplication::setQuitOnLastWindowClosed(false);
    connect(qApp, &QApplication::paletteChanged, this, [=](const QPalette& pa){
        ui->tabWidget->setPalette(pa);
    });
    ui->menubar->setStyleSheet("QMenuBar:item{background:transparent;}QMenuBar{background:transparent;}");

    sideButtonList = { ui->roomPageButton,
                  ui->danmakuPageButton,
                  ui->thankPageButton,
                  ui->musicPageButton,
                  ui->extensionPageButton,
                  ui->preferencePageButton,
                  ui->forumButton
                };
    for (int i = 0; i < sideButtonList.size(); i++)
    {
        auto button = sideButtonList.at(i);
        button->setSquareSize();
        button->setFixedForePos();
        button->setFixedSize(QSize(rt->widgetSizeL, rt->widgetSizeL));
        button->setRadius(rt->fluentRadius, rt->fluentRadius);
        button->setIconPaddingProper(0.23);
//        button->setChokingProp(0.08);

        if (i == sideButtonList.size() - 1) // 最后面的不是切换页面，不设置
            continue;

        connect(button, &InteractiveButtonBase::clicked, this, [=]{
            int prevIndex = ui->stackedWidget->currentIndex();
            if (prevIndex == i) // 同一个索引，不用重复切换
                return ;

            ui->stackedWidget->setCurrentIndex(sideButtonList.indexOf(button));

            adjustPageSize(i);
            switchPageAnimation(i);

            // 当前项
            us->setValue("mainwindow/stackIndex", i);
            foreach (auto btn, sideButtonList)
            {
                btn->setNormalColor(Qt::transparent);
            }
            sideButtonList.at(i)->setNormalColor(themeSbg);
        });
    }

    roomCoverLabel = new QLabel(ui->roomInfoMainWidget);

    // 房号位置
    roomIdBgWidget = new QWidget(this->centralWidget());
    roomSelectorBtn = new WaterCircleButton(QIcon(":/icons/account_link"), ui->roomIdEdit);
    roomIdBgWidget->setObjectName("roomIdBgWidget");
    ui->roomIdEdit->setFixedSize(ui->roomIdEdit->size());
    roomSelectorBtn->setGeometry(ui->roomIdEdit->width() - ui->roomIdEdit->height(), 0, ui->roomIdEdit->height(), ui->roomIdEdit->height());
    ui->roomIdSpacingWidget->setMinimumWidth(ui->roomIdEdit->width() + ui->roomIdEdit->height());
    QHBoxLayout* ril = new QHBoxLayout(roomIdBgWidget);
    ril->addWidget(ui->roomIdEdit);
    ril->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->roomIdSpacingWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->upLevelLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->robotNameButton->setRadius(rt->fluentRadius);
    ui->robotNameButton->setTextDynamicSize();
    ui->robotNameButton->setFontSize(12);
    ui->robotNameButton->setFixedForePos();
    ui->robotNameButton->setPaddings(6);
    ui->liveStatusButton->setTextDynamicSize(true);
    ui->liveStatusButton->setText("");
    ui->liveStatusButton->setRadius(rt->fluentRadius);

    // 隐藏用不到的工具
    ui->pushNextCmdButton->hide();
    ui->timerPushCmdCheck->hide();
    ui->timerPushCmdSpin->hide();
    ui->closeTransMouseButton->hide();
    ui->pkMelonValButton->hide();
    ui->menubar->hide();
    ui->statusbar->hide();

    // 设置属性
    ui->roomDescriptionBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    QFontMetrics fm(ui->roomDescriptionBrowser->font());
    ui->roomDescriptionBrowser->setMaximumHeight(fm.lineSpacing() * 5);
    ui->guardCountCard->setMinimumSize(ui->guardCountCard->sizeHint());
    ui->hotCountCard->setMinimumSize(ui->hotCountCard->sizeHint());
    int upHeaderSize = ui->upNameLabel->sizeHint().height() + ui->upLevelLabel->sizeHint().height();
    ui->upHeaderLabel->setFixedSize(upHeaderSize * 2, upHeaderSize * 2);
    ui->robotHeaderLabel->setMinimumSize(ui->upHeaderLabel->size());
    ui->robotHeaderLabel->setFixedHeight(ui->upHeaderLabel->height());
    roomSelectorBtn->setSquareSize();
    roomSelectorBtn->setRadius(rt->fluentRadius);
    roomSelectorBtn->setFocusPolicy(Qt::FocusPolicy::ClickFocus);
    ui->tagsButtonGroup->setSelecteable(false);
    ui->robotInfoWidget->setMinimumWidth(upHeaderSize * 2);

    // 避免压缩
    ui->roomInfoMainWidget->setMinimumSize(ui->roomInfoMainWidget->sizeHint());

    // 限制
    ui->roomIdEdit->setValidator(new QRegExpValidator(QRegExp("^\\w+$")));

    // 切换房间
    roomSelectorBtn->show();
    roomSelectorBtn->setCursor(Qt::PointingHandCursor);
    connect(roomSelectorBtn, &InteractiveButtonBase::clicked, this, [=]{
        newFacileMenu;

        menu->addAction(ui->actionAdd_Room_To_List);
        menu->split();

        QStringList list = us->value("custom/rooms", "").toString().split(";", QString::SkipEmptyParts);
        for (int i = 0; i < list.size(); i++)
        {
            QStringList texts = list.at(i).split(",", QString::SkipEmptyParts);
            if (texts.size() < 1)
                continue ;
            QString id = texts.first();
            QString name = texts.size() >= 2 ? texts.at(1) : id;
            menu->addAction(name, [=]{
                ui->roomIdEdit->setText(id);
                on_roomIdEdit_editingFinished();
            });
        }

        menu->exec();
    });

    connect(ui->robotHeaderLabel, &ClickableLabel::clicked, this, [=]{
        liveService->openUserSpacePage(ac->cookieUid);
    });

    connect(ui->upHeaderLabel, &ClickableLabel::clicked, this, [=]{
        liveService->openLiveRoomPage(ac->roomId);
    });

    connect(ui->upNameLabel, &ClickableLabel::clicked, this, [=]{
        liveService->openUserSpacePage(ac->upUid);
    });

    connect(ui->roomAreaLabel, &ClickableLabel::clicked, this, [=]{
        if (ac->cookieUid == ac->upUid)
        {
            liveService->myLiveSelectArea(true);
        }
        else
        {
            liveService->openAreaRankPage(ac->areaId, ac->parentAreaId);
        }
    });

    connect(ui->roomNameLabel, &ClickableLabel::clicked, this, [=]{
        if (ac->upUid.isEmpty() || ac->upUid != ac->cookieUid)
            return ;

       liveService->myLiveSetTitle();
    });

    connect(ui->battleRankIconLabel, &ClickableLabel::clicked, this, [=]{
        liveService->showPkMenu();
    });

    connect(ui->battleRankNameLabel, &ClickableLabel::clicked, this, [=]{
        liveService->showPkMenu();
    });

    connect(ui->winningStreakLabel, &ClickableLabel::clicked, this, [=]{
        liveService->showPkMenu();
    });

    // 吊灯
    ui->droplight->setPaddings(12, 12, 2, 2);
    ui->droplight->adjustMinimumSize();
    ui->droplight->setRadius(rt->fluentRadius);

    // 礼物列表
    const int giftImgSize = ui->upHeaderLabel->width(); // 礼物小图片的高度
    int listHeight = giftImgSize + ui->guardCountLabel->height() + ui->guardCountTextLabel->height();
    ui->giftListWidget->setFixedHeight(listHeight);

    // 弹幕设置瀑布流
    ui->showLiveDanmakuWindowButton->setAutoTextColor(false);
    ui->showLiveDanmakuWindowButton->setFontSize(12);
    ui->showLiveDanmakuWindowButton->setPaddings(16, 16, 4, 4);
    ui->showLiveDanmakuWindowButton->setFixedForeSize();
    ui->showLiveDanmakuWindowButton->setBgColor(Qt::white);

    ui->scrollArea->setItemSpacing(24, 24);
    ui->scrollArea->initFixedChildren();
    ui->scrollArea->resizeWidgetsToEqualWidth();
//    ui->scrollArea->setAllowDifferentWidth(true);
//    ui->scrollArea->resizeWidgetsToSizeHint();
    foreach (auto w, ui->scrollArea->getWidgets())
    {
        // 设置样式
        w->setStyleSheet("#" + w->objectName() + "{"
                                                 "background: white;"
                                                 "border: none;"
                                                 "border-radius:" + snum(rt->fluentRadius) + ";"
                                                 "}");

        // 设置阴影
        QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(w);
        effect->setColor(QColor(63, 63, 63, 32));
        effect->setBlurRadius(12);
        effect->setXOffset(4);
        effect->setYOffset(4);
        w->setGraphicsEffect(effect);
    }

    ui->SendMsgButton->setFixedForeSize();
    ui->SendMsgButton->setRadius(rt->fluentRadius);
    ui->sendMsgMoreButton->setSquareSize();
    ui->sendMsgMoreButton->setRadius(rt->fluentRadius);

    ui->saveEveryGiftButton->setSquareSize();
    ui->saveEveryGiftButton->setRadius(rt->fluentRadius);
    ui->saveEveryGuardButton->setSquareSize();
    ui->saveEveryGuardButton->setRadius(rt->fluentRadius);
    ui->saveMonthGuardButton->setSquareSize();
    ui->saveMonthGuardButton->setRadius(rt->fluentRadius);
    ui->saveDanmakuToFileButton->setSquareSize();
    ui->saveDanmakuToFileButton->setRadius(rt->fluentRadius);
    ui->calculateDailyDataButton->setSquareSize();
    ui->calculateDailyDataButton->setRadius(rt->fluentRadius);
    ui->recordDataButton->setSquareSize();
    ui->recordDataButton->setRadius(rt->fluentRadius);
    ui->ffmpegButton->setSquareSize();
    ui->ffmpegButton->setRadius(rt->fluentRadius);

    // 答谢页面
    thankTabButtons = {
        ui->thankWelcomeTabButton,
        ui->thankGiftTabButton,
        ui->thankAttentionTabButton,
        ui->blockTabButton
    };
    foreach (auto btn, thankTabButtons)
    {
        btn->setAutoTextColor(false);
        btn->setPaddings(18, 18, 0, 0);
    }
    ui->thankTopTabGroup->layout()->activate();
    ui->thankTopTabGroup->setFixedHeight(ui->thankTopTabGroup->sizeHint().height());
    ui->thankTopTabGroup->setStyleSheet("#thankTopTabGroup { background: white; border-radius: " + snum(ui->thankTopTabGroup->height() / 2) + "px; }");

    customVarsButton = new InteractiveButtonBase(QIcon(":/icons/settings"), ui->thankPage);
    customVarsButton->setRadius(rt->fluentRadius);
    customVarsButton->setSquareSize();
    customVarsButton->setCursor(Qt::PointingHandCursor);
    int tabBarHeight = ui->thankTopTabGroup->height();
    customVarsButton->setFixedSize(tabBarHeight, tabBarHeight);
    customVarsButton->show();
    connect(customVarsButton, &InteractiveButtonBase::clicked, this, [=]{
        newFacileMenu;
        menu->addAction(ui->actionCustom_Variant);
        menu->addAction(ui->actionReplace_Variant);
        menu->split()->addAction(ui->actionLocal_Mode);
        menu->addAction(ui->actionDebug_Mode);
        menu->addAction(ui->actionLast_Candidate);
        menu->exec();
    });

    // 禁言
    ui->eternalBlockListButton->adjustMinimumSize();
    ui->eternalBlockListButton->setBorderColor(Qt::lightGray);
    ui->eternalBlockListButton->setBgColor(Qt::white);
    ui->eternalBlockListButton->setRadius(rt->fluentRadius);
    ui->eternalBlockListButton->setFixedForePos();

    // 点歌页面
    ui->showOrderPlayerButton->setFixedForeSize(true, 12);
    ui->showOrderPlayerButton->setAutoTextColor(false);
    ui->showOrderPlayerButton->setBgColor(QColor(180, 166, 211));
    {
        QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(ui->showOrderPlayerButton);
        effect->setColor(QColor(63, 63, 63, 64));
        effect->setBlurRadius(24);
        effect->setXOffset(4);
        effect->setYOffset(4);
        ui->showOrderPlayerButton->setGraphicsEffect(effect);
    }
    {
        QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(ui->musicConfigCardWidget);
        effect->setColor(QColor(63, 63, 63, 32));
        effect->setBlurRadius(24);
        effect->setXOffset(2);
        effect->setYOffset(2);
        ui->musicConfigCardWidget->setGraphicsEffect(effect);
    }
    ui->musicImageWidget->setPixmap(QPixmap(":/bg/bg"));
    ui->musicImageWidget->setPart([=](QRect canvas) -> QPainterPath {
        int w = canvas.width(), h = canvas.height();
        QPainterPath path;
        path.addRoundedRect(w / 9, h / 3, w / 3, h * 2 / 3, rt->fluentRadius, rt->fluentRadius);
        path.addRoundedRect(w * 5 / 9, 0, w / 3, h * 2 / 3, rt->fluentRadius, rt->fluentRadius);
        return path;
    });
    musicTitleDecorateWidget = new CustomPaintWidget(ui->musicContainerWidget);
    QFont musicTitleColFont(ui->musicBigTitleLabel->font());
    musicTitleColFont.setPointSize(qMax(musicTitleColFont.pointSize(), 20) * 4 / 3);
    fm = QFontMetrics(musicTitleColFont);
    const QString musicTitleDecorate = "ORDER MUSIC";
    musicTitleDecorateWidget->setFixedSize(fm.height(), fm.horizontalAdvance(musicTitleDecorate));
    musicTitleDecorateWidget->setPaint([=](QRect geom, QPainter* painter) -> void {
        QString text = ui->musicBigTitleLabel->text();
        painter->translate(geom.width(), 0);
        painter->rotate(90);
        painter->setFont(musicTitleColFont);
        painter->setPen(QPen(QColor(128, 128, 128, 32)));
        painter->drawText(QRect(0, 0, geom.height(), geom.width()), Qt::AlignCenter, musicTitleDecorate);
    });
    musicTitleDecorateWidget->lower();
    musicTitleDecorateWidget->stackUnder(ui->musicBigTitleLabel);
    ui->musicBlackListButton->setTextColor(Qt::gray);
    ui->musicBlackListButton->setRadius(rt->fluentRadius);
    ui->musicBlackListButton->adjustMinimumSize();
    ui->musicBlackListButton->setFixedForePos();
    ui->musicConfigButton->setSquareSize();
    ui->musicConfigButton->setFixedForePos();
    ui->musicConfigButton->setRadius(rt->fluentRadius);
    ui->musicConfigButtonSpacingWidget->setFixedWidth(ui->musicConfigButton->width());
    ui->addMusicToLiveButton->setRadius(rt->fluentRadius);
    ui->addMusicToLiveButton->setPaddings(8);
    ui->addMusicToLiveButton->adjustMinimumSize();
    ui->addMusicToLiveButton->setBorderWidth(1);
    ui->addMusicToLiveButton->setBorderColor(Qt::lightGray);
    ui->addMusicToLiveButton->setFixedForePos();

    // 扩展页面
    extensionButton = new InteractiveButtonBase(QIcon(":/icons/settings"), ui->tabWidget);
    extensionButton->setRadius(rt->fluentRadius);
    extensionButton->setSquareSize();
    extensionButton->setCursor(Qt::PointingHandCursor);
    tabBarHeight = ui->tabWidget->tabBar()->height();
    extensionButton->setFixedSize(tabBarHeight, tabBarHeight);
    extensionButton->show();
    connect(extensionButton, &InteractiveButtonBase::clicked, this, [=]{
        newFacileMenu;
        menu->addAction(ui->actionCustom_Variant);
        menu->addAction(ui->actionReplace_Variant);
        menu->split()->addAction(ui->actionPaste_Code);
        menu->addAction(ui->actionGenerate_Default_Code);
        menu->addAction(ui->actionRead_Default_Code);
        menu->split()->addAction(ui->actionLocal_Mode);
        menu->addAction(ui->actionData_Path);
        menu->addAction(ui->actionDebug_Mode);
        menu->addAction(ui->actionLast_Candidate);
        menu->exec();
    });

    appendListItemButton = new AppendButton(ui->tabWidget);
    appendListItemButton->setFixedSize(rt->widgetSizeL, rt->widgetSizeL);
    appendListItemButton->setRadius(rt->widgetSizeL);
    appendListItemButton->setCursor(Qt::PointingHandCursor);
    appendListItemButton->setBgColor(Qt::white);
    {
        QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(appendListItemButton);
        effect->setColor(QColor(63, 63, 63, 128));
        effect->setBlurRadius(24);
        effect->setXOffset(4);
        effect->setYOffset(4);
        appendListItemButton->setGraphicsEffect(effect);
    }
    connect(appendListItemButton, SIGNAL(clicked()), this, SLOT(addListItemOnCurrentPage()));

    ui->vipExtensionButton->setBgColor(Qt::white);
    ui->vipExtensionButton->setRadius(rt->fluentRadius);

    // 网页
    ui->refreshExtensionListButton->setSquareSize();

#ifdef ZUOQI_ENTRANCE
    // 坐骑
    fakeEntrance = new SingleEntrance(this);
    fakeEntrance->setGeometry(this->rect());
    connect(fakeEntrance, &SingleEntrance::signalRoomIdChanged, this, [=](QString roomId){
        if (roomId == "esc")
        {
            fakeEntrance->deleteLater();
            return ;
        }
        ui->roomIdEdit->setText(roomId);
        on_roomIdEdit_editingFinished();
    });
    connect(fakeEntrance, &SingleEntrance::signalLogin, this, [=]{
        on_robotNameButton_clicked();
    });
    this->setWindowTitle("进场坐骑系统");
    if (!ac->identityCode.isEmpty())
        fakeEntrance->setRoomId(ac->identityCode);
#endif

    // 通知
    tip_box = new TipBox(this);
    connect(tip_box, &TipBox::signalCardClicked, [=](NotificationEntry* n){
        qInfo() << "卡片点击：" << n->toString();
    });
    connect(tip_box, &TipBox::signalBtnClicked, [=](NotificationEntry* n){
        qInfo() << "卡片按钮点击：" << n->toString();
    });
}

void MainWindow::initStyle()
{
    ui->roomInfoMainWidget->setStyleSheet("#roomInfoMainWidget\
                        {\
                            background: white;\
                            border-radius: " + snum(rt->fluentRadius) + "px;\
                        }");
    ui->guardCountCard->setStyleSheet("#guardCountWidget\
                        {\
                            background: #f7f7ff;\
                            border: none;\
                            border-radius: " + snum(rt->fluentRadius) + "px;\
                        }");
    ui->hotCountCard->setStyleSheet("#hotCountWidget\
                       {\
                           background: #f7f7ff;\
                           border: none;\
                           border-radius: " + snum(rt->fluentRadius) + "px;\
                                    }");
}

void MainWindow::initObject()
{
    rt->firstOpen = !QFileInfo(rt->dataPath + "settings.ini").exists();
    if (!rt->firstOpen)
    {
        // 备份当前配置：settings和heaps
        ensureDirExist(rt->dataPath + "backup");
        QString ts = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm");
        copyFile(rt->dataPath + "settings.ini", rt->dataPath + "/backup/settings_" + ts + ".ini");
        copyFile(rt->dataPath + "heaps.ini", rt->dataPath + "/backup/heaps_" + ts + ".ini");
    }

    us = new UserSettings(rt->dataPath + "settings.ini");
    cr->heaps = new MySettings(rt->dataPath + "heaps.ini", QSettings::Format::IniFormat);
    cr->extSettings = new MySettings(rt->dataPath + "ext_settings.ini", QSettings::Format::IniFormat);
    robotRecord = new MySettings(rt->dataPath + "robots.ini", QSettings::Format::IniFormat);

    // 版本
    rt->appVersion = GetFileVertion(QApplication::applicationFilePath()).trimmed();
    if (rt->appVersion.startsWith("v") || rt->appVersion.startsWith("V"))
            rt->appVersion.replace(0, 1, "");
    QString oldValue = us->value("runtime/appVersion").toString();
    if (rt->appVersion != oldValue)
    {
        upgradeVersionToLastest(oldValue);
        us->setValue("runtime/appVersion", rt->appVersion);
        QTimer::singleShot(1000, [=]{
            QString ch = QApplication::applicationDirPath() + "/CHANGELOG.md";
            if (!oldValue.isEmpty()
                    && ui->showChangelogCheck->isChecked()
                    && isFileExist(ch))
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(ch));
            }
        });
    }
    ui->appNameLabel->setText("神奇弹幕 v" + rt->appVersion);

    // 编译时间
    QString dateTime = __DATE__;
    //注意此处的replace()，可以保证日期为2位，不足的补0
    dateTime = QLocale(QLocale::English).toDateTime(dateTime.replace("  "," 0"), "MMM dd yyyy").toString("yyyy.MM.dd");
    QTime buildTime = QTime::fromString(__TIME__, "hh:mm:ss");
    ui->appNameLabel->setToolTip("编译时间：" + dateTime + " " + buildTime.toString());

    // 各种时钟
    // 定时移除弹幕
    removeTimer = new QTimer(this);
    removeTimer->setInterval(200);
    connect(removeTimer, SIGNAL(timeout()), this, SLOT(removeTimeoutDanmaku()));
    removeTimer->start();

    // 录播
    recordTimer = new QTimer(this);
    recordTimer->setInterval(30 * 60000); // 默认30分钟断开一次
    connect(recordTimer, &QTimer::timeout, this, [=]{
        if (!recordLoop) // 没有正在录制
            return ;

        qInfo() << "定时停止录播";
        recordLoop->quit(); // 这个就是停止录制了
        // 停止之后，录播会检测是否还需要重新录播
        // 如果是，则继续录
    });

    // 状态栏
    statusLabel = new QLabel(this);
    statusLabel->setObjectName("statusLabel");
    this->statusBar()->addWidget(statusLabel, 1);
    statusLabel->setAlignment(Qt::AlignLeft);

    // 托盘
    tray = new QSystemTrayIcon(this);//初始化托盘对象tray
    tray->setIcon(QIcon(QPixmap(":/icons/star")));//设定托盘图标，引号内是自定义的png图片路径
    tray->setToolTip("神奇弹幕");
    QString title="APP Message";
    QString text="神奇弹幕";
//    tray->showMessage(title,text,QSystemTrayIcon::Information,3000); //最后一个参数为提示时长，默认10000，即10s

    QAction *windowAction = new QAction(QIcon(":/icons/star"), "主界面", this);
    connect(windowAction, SIGNAL(triggered()), this, SLOT(show()));
    QAction *liveDanmakuAction = new QAction(QIcon(":/icons/danmu"), "弹幕姬", this);
    connect(liveDanmakuAction, SIGNAL(triggered()), this, SLOT(on_actionShow_Live_Danmaku_triggered()));
    QAction *orderPlayerAction = new QAction(QIcon(":/icons/order_song"), "点歌姬", this);
    connect(orderPlayerAction, SIGNAL(triggered()), this, SLOT(on_actionShow_Order_Player_Window_triggered()));
    QAction *videoAction = new QAction(QIcon(":/icons/live"), "视频流", this);
    connect(videoAction, SIGNAL(triggered()), this, SLOT(on_actionShow_Live_Video_triggered()));
    QAction *quitAction = new QAction(QIcon(":/icons/cry"), "退出", this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(prepareQuit()));

    connect(tray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(trayAction(QSystemTrayIcon::ActivationReason)));

    // 版本
    permissionText = us->value("mainwindow/permissionText", rt->asPlugin ? "Lite版" : permissionText).toString();

    // 10秒内不进行自动化操作
    QTimer::singleShot(3000, [=]{
        justStart = false;
    });

    // 读取拼音1
    QtConcurrent::run([=]{
        QFile pinyinFile(":/documents/pinyin");
        pinyinFile.open(QIODevice::ReadOnly);
        QTextStream pinyinIn(&pinyinFile);
        pinyinIn.setCodec("UTF-8");
        QString line = pinyinIn.readLine();
        while (!line.isNull())
        {
            if (!line.isEmpty())
            {
                QString han = line.at(0);
                QString pinyin = line.right(line.length()-1);
                rt->pinyinMap.insert(han, pinyin);
            }
            line = pinyinIn.readLine();
        }
    });

    // 读取自定义快捷房间
    QStringList list = us->value("custom/rooms", "").toString().split(";", QString::SkipEmptyParts);
    ui->menu_3->addSeparator();
    for (int i = 0; i < list.size(); i++)
    {
        QStringList texts = list.at(i).split(",", QString::SkipEmptyParts);
        if (texts.size() < 1)
            continue ;
        QString id = texts.first();
        QString name = texts.size() >= 2 ? texts.at(1) : id;
        QAction* action = new QAction(name, this);
        ui->menu_3->addAction(action);
        connect(action, &QAction::triggered, this, [=]{
            ui->roomIdEdit->setText(id);
            on_roomIdEdit_editingFinished();
        });
    }

    // sync
    syncTimer = new QTimer(this);
    syncTimer->setSingleShot(true);
    connect(syncTimer, &QTimer::timeout, this, [=]{
        /* if (ac->roomId.isEmpty() || !isLiving()) // 使用一段时间后才算真正用上
            return ; */
        syncMagicalRooms();
    });

    // permission
    permissionTimer = new QTimer(this);
    connect(permissionTimer, &QTimer::timeout, this, [=]{
        updatePermission();
    });

    // Lite版
    if (rt->asPlugin)
    {
        ui->droplight->setText("Lite版");
        ui->label_52->setText("<html><head/><body><p>暂</p><p>且</p><p>留</p><p>空</p></body></html>");
    }
    if (rt->asFreeOnly)
    {
        ui->vipExtensionButton->setText("Lite版未安装回复、事件等功能");
        ui->existExtensionsLabel->setText("Lite版不支持插件系统");
    }

    // 回复统计数据
    int appOpenCount = us->value("mainwindow/appOpenCount", 0).toInt();
    us->setValue("mainwindow/appOpenCount", ++appOpenCount);
    ui->robotSendCountTextLabel->setToolTip("累计启动 " + snum(appOpenCount) + " 次");
}

void MainWindow::initPath()
{
    rt->appFileName = QFileInfo(QApplication::applicationFilePath()).baseName();
    if (rt->appFileName == "start" || rt->appFileName == "start.exe")
    {
        rt->asPlugin = true;
        rt->asFreeOnly = true;
    }
    rt->dataPath = QApplication::applicationDirPath() + "/";
#ifdef Q_OS_WIN
    // 如果没有设置通用目录，则选择安装文件夹
    if (QFileInfo(rt->dataPath+"green_version").exists()
            || QFileInfo(rt->dataPath+"green_version.txt").exists())
    {
        // 安装路径，不需要改

    }
    else // 通用文件夹
    {
        rt->dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";
        // C:/Users/Administrator/AppData/Roaming/神奇弹幕    (未定义ApplicationName时为exe名)
        SOCKET_DEB << "路径：" << rt->dataPath;
    }
#else
    rt->dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";
    QDir().mkpath(rt->dataPath);
#endif
}

void MainWindow::initLiveService()
{
    liveService = new BiliLiveService(this);
    cr->setLiveService(liveService);

    /// 流程操作
    connect(liveService, &LiveRoomService::signalStartConnectRoom, this, [=]{
        startConnectRoom();
    });
    
    /// 变量到UI
    connect(liveService, &LiveRoomService::signalConnectionStateTextChanged, this, [=](const QString& text) {
        ui->connectStateLabel->setText(text);
    });
    connect(liveService, &LiveRoomService::signalStatusChanged, this, [=](const QString& text) {
        statusLabel->setText(text);
    });

    // 连接之后
    connect(liveService, &LiveRoomService::signalRoomInfoChanged, this, [=] {
        // 设置房间信息
        ui->roomIdEdit->setText(ac->roomId);
        tray->setToolTip(ac->roomTitle + " - " + ac->upName);
        if (ui->roomNameLabel->text().isEmpty() || ui->roomNameLabel->text() != warmWish)
            ui->roomNameLabel->setText(ac->roomTitle);
        ui->upNameLabel->setText(ac->upName);
        setRoomDescription(ac->roomDescription);
        ui->roomAreaLabel->setText(ac->areaName);
        ui->popularityLabel->setText(snum(liveService->online));
        ui->fansCountLabel->setText(snum(ac->currentFans));
        ui->fansClubCountLabel->setText(snum(ac->currentFansClub));
        ui->tagsButtonGroup->initStringList(ac->roomTags);

        // 榜单信息
        ui->roomRankLabel->setText(snum(ac->roomRank));
        if (!ac->rankArea.isEmpty())
        {
            if (!ac->rankArea.endsWith("榜"))
                ac->rankArea += "榜";
            ui->roomRankTextLabel->setText(ac->rankArea);
            ui->roomRankTextLabel->setToolTip("当前总人数:" + snum(ac->countdown));
        }
        ui->liveRankLabel->setText(ac->liveRank); // 直播榜（主播榜）
        ui->liveRankLabel->setToolTip("分区榜：" + ac->areaRank);
        ui->watchedLabel->setText(ac->watchedShow);

        // 设置直播状态
        if (ac->liveStatus == 0)
        {
            ui->liveStatusButton->setText("未开播");
            if (us->timerConnectServer && !liveService->connectServerTimer->isActive())
                liveService->connectServerTimer->start();
        }
        else if (ac->liveStatus == 1)
        {
            ui->liveStatusButton->setText("已开播");
        }
        else if (ac->liveStatus == 2)
        {
            ui->liveStatusButton->setText("轮播中");
        }
        else
        {
            ui->liveStatusButton->setText("未知状态" + snum(ac->liveStatus));
        }

        // 大乱斗信息
        ui->battleRankNameLabel->setText(ac->battleRankName);
        if (!ac->battleRankName.isEmpty()) // 已经使用过大乱斗
        {
            ui->battleInfoWidget->show();
        }
        else // 没有参加过大乱斗
        {
            ui->battleInfoWidget->hide();
        }

        if (danmakuWindow)
            danmakuWindow->setIds(ac->upUid.toLongLong(), ac->roomId.toLongLong());

        updatePermission();
    });

    connect(liveService, &LiveRoomService::signalImUpChanged, this, [=](bool isUp){
        if (isUp)
        {
            ui->actionJoin_Battle->setEnabled(true);
        }
    });

    connect(liveService, &LiveRoomService::signalRoomCoverChanged, this, [=](const QPixmap& pixmap) {
        setRoomCover(pixmap);
    });

    connect(liveService, &LiveRoomService::signalRoomTitleChanged, this, [=](const QString& title) {
        ui->roomNameLabel->setText(title);
    });

    connect(liveService, &LiveRoomService::signalRoomDescriptionChanged, this, [=](const QString& content) {
        ui->roomDescriptionBrowser->setPlainText(content);
    });

    connect(liveService, &LiveRoomService::signalRoomTagsChanged, this, [=](const QStringList& tags) {
        ui->tagsButtonGroup->initStringList(tags);
    });

    connect(liveService, &LiveRoomService::signalUpFaceChanged, this, [=](const QPixmap& pixmap) {
        QPixmap circlePixmap = PixmapUtil::toCirclePixmap(pixmap); // 圆图

        // 设置到窗口图标
        QPixmap face = liveService->isLiving() ? PixmapUtil::toLivingPixmap(circlePixmap) : circlePixmap;
        setWindowIcon(face);
        tray->setIcon(face);

        // 设置到UP头像
        face = liveService->upFace.scaled(ui->upHeaderLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPixmap circle = PixmapUtil::toCirclePixmap(face);
        ui->upHeaderLabel->setPixmap(circle);
        if (musicWindow)
        {
            musicWindow->setTitleIcon(circle);
        }
    });

    connect(liveService, &LiveRoomService::signalUpSignatureChanged, this, [=](const QString& signature) {
        ui->upLevelLabel->setText(signature);
    });

    connect(liveService, &LiveRoomService::signalFinishDove, this, [=]{
        ui->doveCheck->setChecked(false);
    });

    connect(liveService, &LiveRoomService::signalGuardsChanged, this, [=]{
        if (ui->saveMonthGuardCheck->isChecked())
            saveMonthGuard();

        if (liveService->dailySettings)
            liveService->dailySettings->setValue("guard_count", ac->currentGuards.size());
        liveService->updateGuarding = false;
        ui->guardCountLabel->setText(snum(ac->currentGuards.size()));
    });

    connect(liveService, &LiveRoomService::signalOnlineRankChanged, this, [=]{
        updateOnlineRankGUI();
    });

    connect(liveService, &LiveRoomService::signalAutoAdjustDanmakuLongest, this, [=]{
        if (ui->adjustDanmakuLongestCheck->isChecked())
            liveService->adjustDanmakuLongest();
    });

    connect(liveService, &LiveRoomService::signalDanmuPopularChanged, this, [=](const QString& text){
        ui->danmuCountLabel->setToolTip(text);
    });

    connect(liveService, &LiveRoomService::signalSignInfoChanged, this, [=](const QString& text){
        ui->autoDoSignCheck->setText(text);
    });

    connect(liveService, &LiveRoomService::signalSignDescChanged, this, [=](const QString& text){
        ui->autoDoSignCheck->setToolTip(text);
    });

    connect(liveService, &LiveRoomService::signalLOTInfoChanged, this, [=](const QString& text){
        ui->autoLOTCheck->setText(text);
    });

    connect(liveService, &LiveRoomService::signalLOTDescChanged, this, [=](const QString& text){
        ui->autoLOTCheck->setToolTip(text);
    });

    /// 直播事件
    connect(liveService, &LiveRoomService::signalGetRoomAndRobotFinished, this, [=]() {
        triggerCmdEvent("LOGIN_FINISHED", LiveDanmaku());
    });

    connect(liveService, &LiveRoomService::signalStartWork, this, [=] {
        slotStartWork();
    });

    connect(liveService, &LiveRoomService::signalCanRecord, this, [=] {
        if (ui->recordCheck->isChecked() && liveService->isLiving())
            startLiveRecord();
    });

    connect(liveService, &LiveRoomService::signalHeartTimeNumberChanged, this, [=](int num, int minute) {
        ui->acquireHeartCheck->setToolTip("今日已领" + snum(num) + "个小心心(" + snum(minute) + "分钟)");
    });
    
    connect(liveService, &LiveRoomService::signalDanmakuLongestChanged, this, [=](int length) {
        ui->danmuLongestSpin->setValue(length);
        on_danmuLongestSpin_editingFinished();
    });

    /// 信号传递
    connect(liveService, &LiveRoomService::signalAppendNewLiveDanmaku, this, [=](const LiveDanmaku& danmaku) {
        appendNewLiveDanmaku(danmaku);
    });

    connect(liveService, &LiveRoomService::signalTriggerCmdEvent, this, [=](const QString& cmd, const LiveDanmaku& danmaku, bool debug) {
        triggerCmdEvent(cmd, danmaku, debug);
    });

    connect(liveService, &LiveRoomService::signalLocalNotify, this, [=](const QString& text, qint64 uid) {
        localNotify(text, uid);
    });

    connect(liveService, &LiveRoomService::signalShowError, this, [=](const QString& title, const QString& info) {
        showError(title, info);
    });

    connect(liveService, &LiveRoomService::signalUpdatePermission, this, [=] {
        updatePermission();
    });

    connect(liveService, &LiveRoomService::signalNewHour, this, [=] {

    });

    connect(liveService, &LiveRoomService::signalNewDay, this, [=] {
        // 永久禁言
        detectEternalBlockUsers();
    });

    connect(liveService, &LiveRoomService::signalSendAutoMsg, this, [=](const QString& msg, const LiveDanmaku& danmaku) {
        cr->sendAutoMsg(msg, danmaku);
    });
    
    connect(liveService, &LiveRoomService::signalSendAutoMsgInFirst, this, [=](const QString& msg, const LiveDanmaku& danmaku, int interval) {
        cr->sendAutoMsgInFirst(msg, danmaku, interval);
    });

    /// 大乱斗
    connect(liveService, &LiveRoomService::signalBattleEnabled, this, [=](bool enable) {
        if (enable)
        {
            ui->battleInfoWidget->show();
        }
        else
        {
            ui->battleInfoWidget->hide();
        }
    });

    connect(liveService, &LiveRoomService::signalBattleRankGot, this, [=] {
        if (ui->battleInfoWidget->isHidden())
            ui->battleInfoWidget->show();
        ui->winningStreakLabel->setText("连胜" + snum(ac->winningStreak) + "场");
    });
    connect(liveService, &LiveRoomService::signalBattleRankNameChanged, this, [=](const QString& text) {
        ui->battleRankNameLabel->setText(text);
    });

    connect(liveService, &LiveRoomService::signalBattleRankIconChanged, this, [=](const QPixmap& pixmap) {
        QPixmap p = pixmap;
        if (!p.isNull())
            p = pixmap.scaledToHeight(ui->battleRankNameLabel->height() * 2, Qt::SmoothTransformation);
        ui->battleRankIconLabel->setPixmap(pixmap);
    });

    connect(liveService, &LiveRoomService::signalBattleNumsChanged, this, [=](const QString& text) {
        ui->winningStreakLabel->setToolTip(text);
    });

    connect(liveService, &LiveRoomService::signalBattleSeasonInfoChanged, this, [=](const QString& text) {
        ui->battleRankIconLabel->setToolTip(text);
    });

    connect(liveService, &LiveRoomService::signalBattleNumsChanged, this, [=](const QString& text) {
        ui->winningStreakLabel->setToolTip(text);
    });

    connect(liveService, &LiveRoomService::signalBattleScoreChanged, this, [=](const QString& text) {
        ui->battleRankNameLabel->setToolTip(text);
    });

    connect(liveService, &LiveRoomService::signalBattleStarted, this, [=] {
        if (danmakuWindow)
        {
            danmakuWindow->showStatusText();
            danmakuWindow->setToolTip(liveService->pkUname);
            danmakuWindow->setPkStatus(1, liveService->pkRoomId.toLongLong(), liveService->pkUid.toLongLong(), liveService->pkUname);
        }
        ui->actionShow_PK_Video->setEnabled(true);
    });

    connect(liveService, &LiveRoomService::signalBattleFinished, this, [=] {

    });

    connect(liveService, &LiveRoomService::signalBattleStartMatch, this, [=] {
        if (danmakuWindow)
            danmakuWindow->setStatusText("正在匹配...");
    });

    /// 需要调整的
    // 礼物连击
    connect(liveService->comboTimer, SIGNAL(timeout()), this, SLOT(slotComboSend()));

    // 大乱斗
    connect(liveService->pkTimer, &QTimer::timeout, this, [=]{
        // 更新PK信息
        int second = 0;
        int minute = 0;
        if (liveService->pkEndTime)
        {
            second = static_cast<int>(liveService->pkEndTime - QDateTime::currentSecsSinceEpoch());
            if (second < 0) // 结束后会继续等待一段时间，这时候会变成负数
                second = 0;
            minute = second / 60;
            second = second % 60;
        }
        QString text = QString("%1:%2 %3/%4")
                .arg(minute)
                .arg((second < 10 ? "0" : "") + QString::number(second))
                .arg(liveService->myVotes)
                .arg(liveService->matchVotes);
        if (danmakuWindow)
            danmakuWindow->setStatusText(text);
    });

    // 大乱斗自动赠送吃瓜
    connect(liveService->pkEndingTimer, &QTimer::timeout, this, &MainWindow::slotPkEndingTimeout);

    // 私信功能开关
    connect(liveService, &LiveRoomService::signalRefreshPrivateMsgEnabled, this, [=](bool enabled) {
        ui->receivePrivateMsgCheck->setChecked(enabled);
    });

}

/// 读取 settings 中的变量，并进行一系列初始化操作
/// 可能会读取多次，并且随用户命令重复读取
/// 所以里面的所有变量都要做好重复初始化的准备
void MainWindow::readConfig()
{
    // 平台
    rt->livePlatform = (LivePlatform)(us->value("platform/live", 0).toInt());

    // 界面效果
    ui->closeGuiCheck->setChecked(us->closeGui = us->value("mainwindow/closeGui", false).toBool());

    // 标签组
    int tabIndex = us->value("mainwindow/tabIndex", 0).toInt();
    if (tabIndex >= 0 && tabIndex < ui->tabWidget->count())
        ui->tabWidget->setCurrentIndex(tabIndex);
    if (tabIndex > 2)
        appendListItemButton->hide();
    else
        appendListItemButton->show();

    // 页面
    int stackIndex = us->value("mainwindow/stackIndex", 0).toInt();
    if (stackIndex >= 0 && stackIndex < ui->stackedWidget->count())
    {
        ui->stackedWidget->setCurrentIndex(stackIndex);
        adjustPageSize(stackIndex);
        QTimer::singleShot(0, [=]{
            switchPageAnimation(stackIndex);
        });
    }

    // 答谢标签
    int thankStackIndex = us->value("mainwindow/thankStackIndex", 0).toInt();
    if (thankStackIndex >= 0 && thankStackIndex < ui->thankStackedWidget->count())
        ui->thankStackedWidget->setCurrentIndex(thankStackIndex);

    // 音乐标签
    int musicStackIndex = us->value("mainwindow/musicStackIndex", 0).toInt();
    if (musicStackIndex >= 0 && musicStackIndex < ui->musicConfigStack->count())
        ui->musicConfigStack->setCurrentIndex(musicStackIndex);

    // 房间号
    ac->roomId = us->value("danmaku/roomId", "").toString();
    if (!ac->roomId.isEmpty())
    {
        ui->roomIdEdit->setText(ac->roomId);
#ifdef ZUOQI_ENTRANCE
        fakeEntrance->setRoomId(ac->identityCode);
#endif
    }
    else // 设置为默认界面
    {
        QTimer::singleShot(0, [=]{
            setRoomCover(QPixmap(":/bg/bg"));
        });
    }

    // 移除间隔
    int removeIv = us->value("danmaku/removeInterval", 60).toInt();
    ui->removeDanmakuIntervalSpin->setValue(removeIv); // 自动引发改变事件
    this->removeDanmakuInterval = removeIv * 1000;

    removeIv = us->value("danmaku/removeTipInterval", 20).toInt();
    ui->removeDanmakuTipIntervalSpin->setValue(removeIv); // 自动引发改变事件
    this->removeDanmakuTipInterval = removeIv * 1000;

    // 单条弹幕最长长度
    ac->danmuLongest = us->value("danmaku/danmuLongest", 20).toInt();
    ui->danmuLongestSpin->setValue(ac->danmuLongest);
    ui->adjustDanmakuLongestCheck->setChecked(us->value("danmaku/adjustDanmakuLongest", true).toBool());
    cr->robotTotalSendMsg = us->value("danmaku/robotTotalSend", 0).toInt();
    ui->robotSendCountLabel->setText(snum(cr->robotTotalSendMsg));
    ui->robotSendCountLabel->setToolTip("累计发送弹幕 " + snum(cr->robotTotalSendMsg) + " 条");

    // 失败重试
    ui->retryFailedDanmuCheck->setChecked(us->retryFailedDanmaku = us->value("danmaku/retryFailedDanmu", true).toBool());
    liveService->hostUseIndex = us->value("live/hostIndex").toInt();

    // 点歌
    diangeAutoCopy = us->value("danmaku/diangeAutoCopy", true).toBool();
    ui->DiangeAutoCopyCheck->setChecked(diangeAutoCopy);
    ui->diangeNeedMedalCheck->setChecked(us->value("danmaku/diangeNeedMedal", false).toBool());
    QString defaultDiangeFormat = "^[点點]歌[ :：,，]+(.+)";
    diangeFormatString = us->value("danmaku/diangeFormat", defaultDiangeFormat).toString();
    ui->diangeFormatEdit->setText(diangeFormatString);
    ui->diangeReplyCheck->setChecked(us->value("danmaku/diangeReply", false).toBool());
    ui->diangeShuaCheck->setChecked(us->value("danmaku/diangeShua", false).toBool());
    ui->autoPauseOuterMusicCheck->setChecked(us->value("danmaku/autoPauseOuterMusic", false).toBool());
    ui->outerMusicKeyEdit->setText(us->value("danmaku/outerMusicPauseKey").toString());
    ui->orderSongsToFileCheck->setChecked(us->value("danmaku/orderSongsToFile", false).toBool());
    ui->orderSongsToFileFormatEdit->setText(us->value("danmaku/orderSongsToFileFormat", "{歌名} - {歌手}").toString());
    ui->playingSongToFileCheck->setChecked(us->value("danmaku/playingSongToFile", false).toBool());
    ui->playingSongToFileFormatEdit->setText(us->value("danmaku/playingSongToFileFormat", "正在播放：{歌名} - {歌手}").toString());
    ui->orderSongsToFileMaxSpin->setValue(us->value("danmaku/orderSongsToFileMax", 9).toInt());
    ui->songLyricsToFileCheck->setChecked(us->value("danmaku/songLyricsToFile", false).toBool());
    ui->songLyricsToFileMaxSpin->setValue(us->value("danmaku/songLyricsToFileMax", 2).toInt());
    ui->orderSongShuaSpin->setValue(us->value("danmaku/diangeShuaCount", 0).toInt());

    // 自动翻译
    bool trans = us->value("danmaku/autoTrans", false).toBool();
    ui->languageAutoTranslateCheck->setChecked(trans);

    // 自动回复
    bool reply = us->value("danmaku/aiReply", false).toBool();
    ui->AIReplyCheck->setChecked(reply);
    ui->AIReplyMsgCheck->setCheckState(static_cast<Qt::CheckState>(us->value("danmaku/aiReplyMsg", 0).toInt()));
    ui->AIReplyMsgCheck->setEnabled(reply);

    // 黑名单管理
    ui->enableBlockCheck->setChecked(us->value("block/enableBlock", false).toBool());
    ui->syncShieldKeywordCheck->setChecked(us->value("block/syncShieldKeyword", false).toBool());

    // 新人提示
    ui->newbieTipCheck->setChecked(us->value("block/newbieTip", false).toBool());

    // 自动禁言
    ui->autoBlockNewbieCheck->setChecked(us->value("block/autoBlockNewbie", false).toBool());
    ui->autoBlockNewbieKeysEdit->setPlainText(us->value("block/autoBlockNewbieKeys").toString());

    ui->autoBlockNewbieNotifyCheck->setChecked(us->value("block/autoBlockNewbieNotify", false).toBool());
    ui->autoBlockNewbieNotifyWordsEdit->setPlainText(us->value("block/autoBlockNewbieNotifyWords").toString());
    ui->autoBlockNewbieNotifyCheck->setEnabled(ui->autoBlockNewbieCheck->isChecked());

    ui->promptBlockNewbieCheck->setChecked(us->value("block/promptBlockNewbie", false).toBool());
    ui->promptBlockNewbieKeysEdit->setPlainText(us->value("block/promptBlockNewbieKeys").toString());

    ui->notOnlyNewbieCheck->setChecked(us->value("block/notOnlyNewbie", false).toBool());
    ui->blockNotOnlyNewbieCheck->setChecked(us->value("block/blockNotOnlyNewbieCheck", false).toBool());

    ui->autoBlockTimeSpin->setValue(us->value("block/autoTime", 1).toInt());

    // 实时弹幕
#ifndef Q_OS_ANDROID
    // 安装因为界面的问题，不主动显示弹幕姬
    if (us->value("danmaku/liveWindow", false).toBool() && !danmakuWindow)
         on_actionShow_Live_Danmaku_triggered();
#endif

    // 点歌姬
    if (us->value("danmaku/playerWindow", false).toBool() && !musicWindow)
        on_actionShow_Order_Player_Window_triggered();
    orderSongBlackList = us->value("music/blackListKeys", "").toString().split(" ", QString::SkipEmptyParts);

    // 录播
    ui->recordCheck->setChecked(us->value("record/enabled", false).toBool());
    ui->recordFormatCheck->setChecked(us->value("record/format", false).toBool());
    rt->ffmpegPath = us->value("record/ffmpegPath").toString();
    int recordSplit = us->value("record/split", 30).toInt();
    ui->recordSplitSpin->setValue(recordSplit);
    recordTimer->setInterval(recordSplit * 60000); // 默认30分钟断开一次

    // 发送弹幕
    ac->browserCookie = us->value("danmaku/browserCookie", "").toString();
    ac->browserData = us->value("danmaku/browserData", "").toString();
    int posl = ac->browserCookie.indexOf("bili_jct=") + 9;
    int posr = ac->browserCookie.indexOf(";", posl);
    if (posr == -1) posr = ac->browserCookie.length();
    ac->csrf_token = ac->browserCookie.mid(posl, posr - posl);
    ac->userCookies = getCookies();
    getCookieAccount();

    // 保存弹幕
    ui->saveDanmakuToFileCheck->setChecked(us->saveDanmakuToFile = us->value("danmaku/saveDanmakuToFile", false).toBool());

    // 每日数据
    us->calculateDailyData = us->value("live/calculateDaliyData", true).toBool();
    ui->calculateDailyDataCheck->setChecked(us->calculateDailyData);
    if (us->calculateDailyData)
        liveService->startCalculateDailyData();

    // PK串门提示
    liveService->pkChuanmenEnable = us->value("pk/chuanmen", false).toBool();
    ui->pkChuanmenCheck->setChecked(liveService->pkChuanmenEnable);

    // PK消息同步
    liveService->pkMsgSync = us->value("pk/msgSync", 0).toInt();
    if (liveService->pkMsgSync == 0)
        ui->pkMsgSyncCheck->setCheckState(Qt::Unchecked);
    else if (liveService->pkMsgSync == 1)
        ui->pkMsgSyncCheck->setCheckState(Qt::PartiallyChecked);
    else if (liveService->pkMsgSync == 2)
        ui->pkMsgSyncCheck->setCheckState(Qt::Checked);
    ui->pkMsgSyncCheck->setText(liveService->pkMsgSync == 1 ? "PK同步消息(仅视频)" : "PK同步消息");
    ui->pkMsgSyncCheck->setEnabled(liveService->pkChuanmenEnable);

    // 判断机器人
    judgeRobot = us->value("danmaku/judgeRobot", 0).toInt();
    ui->judgeRobotCheck->setCheckState((Qt::CheckState)judgeRobot);
    ui->judgeRobotCheck->setText(judgeRobot == 1 ? "机器人判断(仅关注)" : "机器人判断");

    // 本地昵称
    us->localNicknames.clear();
    QStringList namePares = us->value("danmaku/localNicknames").toString().split(";", QString::SkipEmptyParts);
    foreach (QString pare, namePares)
    {
        QStringList sl = pare.split("=>");
        if (sl.size() < 2)
            continue;

        us->localNicknames.insert(sl.at(0).toLongLong(), sl.at(1));
    }
    if (us->localNicknames.empty())
        us->localNicknames.insert(20285041, "神奇弹幕开发者");

    // 礼物别名
    us->giftAlias.clear();
    namePares = us->value("danmaku/giftNames").toString().split(";", QString::SkipEmptyParts);
    foreach (QString pare, namePares)
    {
        QStringList sl = pare.split("=>");
        if (sl.size() < 2)
            continue;

        us->giftAlias.insert(sl.at(0).toInt(), sl.at(1));
    }

    // 特别关心
    us->careUsers.clear();
    QStringList usersS = us->value("danmaku/careUsers", "20285041").toString().split(";", QString::SkipEmptyParts);
    foreach (QString s, usersS)
    {
        us->careUsers.append(s.toLongLong());
    }
    if (us->careUsers.empty())
        us->careUsers.append(20285041);

    // 强提醒
    us->strongNotifyUsers.clear();
    QStringList usersSN = us->value("danmaku/strongNotifyUsers", "").toString().split(";", QString::SkipEmptyParts);
    foreach (QString s, usersSN)
    {
        us->strongNotifyUsers.append(s.toLongLong());
    }
    if (us->strongNotifyUsers.isEmpty())
        us->strongNotifyUsers.append(20285041);

    // 不自动欢迎
    us->notWelcomeUsers.clear();
    QStringList usersNW = us->value("danmaku/notWelcomeUsers", "").toString().split(";", QString::SkipEmptyParts);
    foreach (QString s, usersNW)
    {
        us->notWelcomeUsers.append(s.toLongLong());
    }

    // 不自动回复
    us->notReplyUsers.clear();
    QStringList usersNR = us->value("danmaku/notReplyUsers", "").toString().split(";", QString::SkipEmptyParts);
    foreach (QString s, usersNR)
    {
        us->notReplyUsers.append(s.toLongLong());
    }

    // 礼物连击
    ui->giftComboSendCheck->setChecked(us->value("danmaku/giftComboSend", false).toBool());
    ui->giftComboDelaySpin->setValue(us->value("danmaku/giftComboDelay",  5).toInt());
    ui->giftComboTopCheck->setChecked(us->value("danmaku/giftComboTop", false).toBool());
    ui->giftComboMergeCheck->setChecked(us->value("danmaku/giftComboMerge", false).toBool());

    // 仅开播发送
    ui->sendAutoOnlyLiveCheck->setChecked(us->value("danmaku/sendAutoOnlyLive", false).toBool());

    // 勋章升级
    ui->listenMedalUpgradeCheck->setChecked(us->value("danmaku/listenMedalUpgrade", false).toBool());

    // 弹幕次数
    us->danmakuCounts = new QSettings(rt->dataPath+"danmu_count.ini", QSettings::Format::IniFormat);

    // 用户备注
    us->userMarks = new QSettings(rt->dataPath+"user_mark.ini", QSettings::Format::IniFormat);

    // 接收私信
    ui->receivePrivateMsgCheck->setChecked(us->value("privateMsg/enabled", false).toBool());
    ui->processUnreadMsgCheck->setChecked(us->value("privateMsg/processUnread", false).toBool());
    liveService->privateMsgTimestamp = QDateTime::currentMSecsSinceEpoch();

    // 过滤器
    cr->enableFilter = us->value("danmaku/enableFilter", cr->enableFilter).toBool();
    ui->enableFilterCheck->setChecked(cr->enableFilter);
    /* filter_musicOrder = settings->value("filter/musicOrder", "").toString();
    filter_musicOrderRe = QRegularExpression(filter_musicOrder);
    filter_danmakuCome = settings->value("filter/danmakuCome", "").toString();
    filter_danmakuGift = settings->value("filter/danmakuGift").toString();*/

    // 编程
    ui->syntacticSugarCheck->setChecked(us->value("programming/syntacticSugar", true).toBool());
    ui->complexCalcCheck->setChecked(us->complexCalc = us->value("programming/complexCalc", false).toBool());
    ui->stringSimilarCheck->setChecked(us->useStringSimilar = us->value("programming/stringSimilar", false).toBool());
    us->stringSimilarThreshold = us->value("programming/stringSimilarThreshold", 80).toInt();
    us->danmuSimilarJudgeCount = us->value("programming/danmuSimilarJudgeCount", 10).toInt();

    // 大乱斗自动赠送吃瓜
    bool melon = us->value("pk/autoMelon", false).toBool();
    ui->pkAutoMelonCheck->setChecked(melon);
    liveService->pkMaxGold = us->value("pk/maxGold", 300).toInt();
    liveService->pkJudgeEarly = us->value("pk/judgeEarly", 2000).toInt();
    liveService->toutaCount = us->value("pk/toutaCount", 0).toInt();
    liveService->chiguaCount = us->value("pk/chiguaCount", 0).toInt();
    liveService->toutaGold = us->value("pk/toutaGold", 0).toInt();
    liveService->goldTransPk = us->value("pk/goldTransPk", liveService->goldTransPk).toInt();
    liveService->toutaBlankList = us->value("pk/blankList").toString().split(";");
    ui->pkAutoMaxGoldCheck->setChecked(us->value("pk/autoMaxGold", true).toBool());

    // 大乱斗自动赠送礼物
    ui->toutaGiftCheck->setChecked(us->value("danmaku/toutaGift").toBool());
    QString toutaGiftCountsStr = us->value("danmaku/toutaGiftCounts").toString();
    ui->toutaGiftCountsEdit->setText(toutaGiftCountsStr);
    liveService->toutaGiftCounts.clear();
    foreach (QString s, toutaGiftCountsStr.split(" ", QString::SkipEmptyParts))
        liveService->toutaGiftCounts.append(s.toInt());
    restoreToutaGifts(us->value("danmaku/toutaGifts", "").toString());

    // 自定义变量
    restoreCustomVariant(us->value("danmaku/customVariant", "").toString());
    restoreReplaceVariant(us->value("danmaku/replaceVariant", "").toString());

    // 多语言翻译
    restoreVariantTranslation();
    ui->autoWelcomeWordsEdit->updateCompleterModel();
    ui->autoThankWordsEdit->updateCompleterModel();
    ui->autoAttentionWordsEdit->updateCompleterModel();

    // 定时任务
    srand((unsigned)time(0));
    restoreTaskList();

    // 自动回复
    restoreReplyList();

    // 事件动作
    restoreEventList();

    // 保存舰长
    ui->saveEveryGuardCheck->setChecked(us->value("danmaku/saveEveryGuard", false).toBool());
    ui->saveMonthGuardCheck->setChecked(us->value("danmaku/saveMonthGuard", false).toBool());
    ui->saveEveryGiftCheck->setChecked(us->value("danmaku/saveEveryGift", false).toBool());

    // 自动发送
    ui->autoSendWelcomeCheck->setChecked(us->value("danmaku/sendWelcome", false).toBool());
    ui->autoSendGiftCheck->setChecked(us->value("danmaku/sendGift", false).toBool());
    ui->autoSendAttentionCheck->setChecked(us->value("danmaku/sendAttention", false).toBool());
    ui->sendWelcomeCDSpin->setValue(us->value("danmaku/sendWelcomeCD", 10).toInt());
    ui->sendGiftCDSpin->setValue(us->value("danmaku/sendGiftCD", 5).toInt());
    ui->sendAttentionCDSpin->setValue(us->value("danmaku/sendAttentionCD", 5).toInt());
    ui->autoWelcomeWordsEdit->setPlainText(us->value("danmaku/autoWelcomeWords", ui->autoWelcomeWordsEdit->toPlainText()).toString());
    ui->autoThankWordsEdit->setPlainText(us->value("danmaku/autoThankWords", ui->autoThankWordsEdit->toPlainText()).toString());
    ui->autoAttentionWordsEdit->setPlainText(us->value("danmaku/autoAttentionWords", ui->autoAttentionWordsEdit->toPlainText()).toString());
    ui->sendWelcomeTextCheck->setChecked(us->value("danmaku/sendWelcomeText", true).toBool());
    ui->sendWelcomeVoiceCheck->setChecked(us->value("danmaku/sendWelcomeVoice", false).toBool());
    ui->sendGiftTextCheck->setChecked(us->value("danmaku/sendGiftText", true).toBool());
    ui->sendGiftVoiceCheck->setChecked(us->value("danmaku/sendGiftVoice", false).toBool());
    ui->sendAttentionTextCheck->setChecked(us->value("danmaku/sendAttentionText", true).toBool());
    ui->sendAttentionVoiceCheck->setChecked(us->value("danmaku/sendAttentionVoice", false).toBool());
    ui->sendWelcomeTextCheck->setEnabled(ui->autoSendWelcomeCheck->isChecked());
    ui->sendWelcomeVoiceCheck->setEnabled(ui->autoSendWelcomeCheck->isChecked());
    ui->sendGiftTextCheck->setEnabled(ui->autoSendGiftCheck->isChecked());
    ui->sendGiftVoiceCheck->setEnabled(ui->autoSendGiftCheck->isChecked());
    ui->sendAttentionTextCheck->setEnabled(ui->autoSendAttentionCheck->isChecked());
    ui->sendAttentionVoiceCheck->setEnabled(ui->autoSendAttentionCheck->isChecked());

    // 文字转语音
    ui->autoSpeekDanmakuCheck->setChecked(us->value("danmaku/autoSpeek", false).toBool());
    ui->dontSpeakOnPlayingSongCheck->setChecked(us->value("danmaku/dontSpeakOnPlayingSong", false).toBool());
    if (ui->sendWelcomeVoiceCheck->isChecked() || ui->sendGiftVoiceCheck->isChecked()
            || ui->sendAttentionVoiceCheck->isChecked() || ui->autoSpeekDanmakuCheck->isChecked())
        initTTS();

    voiceService->voicePlatform = static_cast<VoicePlatform>(us->value("voice/platform", 0).toInt());
    if (voiceService->voicePlatform == VoiceLocal)
    {
        ui->voiceLocalRadio->setChecked(true);
        ui->voiceNameEdit->setText(us->value("voice/localName").toString());
    }
    else if (voiceService->voicePlatform == VoiceXfy)
    {
        ui->voiceXfyRadio->setChecked(true);
        ui->voiceNameEdit->setText(us->value("xfytts/name").toString());
        ui->xfyAppIdEdit->setText(us->value("xfytts/appid").toString());
        ui->xfyApiKeyEdit->setText(us->value("xfytts/apikey").toString());
        ui->xfyApiSecretEdit->setText(us->value("xfytts/apisecret").toString());
    }
    else if (voiceService->voicePlatform == VoiceMS)
    {
        ui->voiceConfigSettingsCard->hide();
        ui->voiceMSRadio->setChecked(true);
        ui->MSAreaCodeEdit->setText(us->value("mstts/areaCode").toString());
        ui->MSSubscriptionKeyEdit->setText(us->value("mstts/subscriptionKey").toString());
        voiceService->msTTSFormat = us->value("mstts/format", DEFAULT_MS_TTS_SSML_FORMAT).toString();
    }
    else if (voiceService->voicePlatform == VoiceCustom)
    {
        ui->voiceCustomRadio->setChecked(true);
        ui->voiceNameEdit->setText(us->value("voice/customName").toString());
    }

    ui->voicePitchSlider->setSliderPosition(us->value("voice/pitch", 50).toInt());
    ui->voiceSpeedSlider->setSliderPosition(us->value("voice/speed", 50).toInt());
    ui->voiceVolumeSlider->setSliderPosition(us->value("voice/volume", 50).toInt());
    ui->voiceCustomUrlEdit->setText(us->value("voice/customUrl", "").toString());

    // AI回复
    chatService->chatPlatform = static_cast<ChatPlatform>(us->value("chat/platform", 0).toInt());
    if (chatService->chatPlatform == ChatGPT)
    {
        ui->chatGPTRadio->setChecked(true);
    }
    else if (chatService->chatPlatform == TxNLP)
    {
        ui->chatTxRadio->setChecked(true);
    }
    else
    {
        ui->chatGPTRadio->setChecked(true);
    }

    // chatgpt
    ui->chatGPTModelNameCombo->setCurrentText(us->chatgpt_model_name = us->value("chatgpt/model_name", us->chatgpt_model_name).toString());
    ui->chatGPTMaxTokenCountSpin->setValue(us->chatgpt_max_token_count = us->value("chatgpt/max_token_count", us->chatgpt_max_token_count).toInt());
    ui->chatGPTMaxContextCountSpin->setValue(us->chatgpt_max_context_count = us->value("chatgpt/max_context_count", us->chatgpt_max_context_count).toInt());

    // 腾讯闲聊
    QString TXSecretId = us->value("tx_nlp/secretId").toString();
    if (!TXSecretId.isEmpty())
    {
        chatService->txNlp->setSecretId(TXSecretId);
        ui->TXSecretIdEdit->setText(TXSecretId);
    }
    QString TXSecretKey = us->value("tx_nlp/secretKey").toString();
    if (!TXSecretKey.isEmpty())
    {
        chatService->txNlp->setSecretKey(TXSecretKey);
        ui->TXSecretKeyEdit->setText(TXSecretKey);
    }
    connect(chatService->txNlp, &TxNlp::signalError, this, [=](const QString& err){
        showError("智能闲聊", err);
    });

    // 开播
    ui->startLiveWordsEdit->setText(us->value("live/startWords").toString());
    ui->endLiveWordsEdit->setText(us->value("live/endWords").toString());
    ui->startLiveSendCheck->setChecked(us->value("live/startSend").toBool());

    // 启动动画
#ifdef ZUOQI_ENTRANCE
    ui->startupAnimationCheck->setChecked(us->value("mainwindow/splash", false).toBool());
#else
    ui->startupAnimationCheck->setChecked(us->value("mainwindow/splash", rt->firstOpen).toBool());
#endif
    ui->enableTrayCheck->setChecked(us->value("mainwindow/enableTray", false).toBool());
    if (ui->enableTrayCheck->isChecked())
        tray->show(); // 让托盘图标显示在系统托盘上
    else
        tray->hide();
    permissionText = us->value("mainwindow/permissionText", rt->asPlugin ? "Lite版" : permissionText).toString();

    // 定时连接
    ui->timerConnectServerCheck->setChecked(us->timerConnectServer = us->value("live/timerConnectServer", false).toBool());
    ui->startLiveHourSpin->setValue(us->startLiveHour = us->value("live/startLiveHour", 0).toInt());
    ui->endLiveHourSpin->setValue(us->endLiveHour = us->value("live/endLiveHour", 0).toInt());
    ui->timerConnectIntervalSpin->setValue(us->timerConnectInterval = us->value("live/timerConnectInterval", 30).toInt());
    liveService->connectServerTimer->setInterval(us->timerConnectInterval * 60000);

    // WS连接
    initWS();
    startConnectRoom();

    // 隐藏偷塔
    if (!us->value("danmaku/touta", false).toBool())
    {
        ui->pkAutoMelonCheck->setText("此项禁止使用");
        ui->danmakuToutaSettingsCard->hide();
        ui->scrollArea->removeWidget(ui->danmakuToutaSettingsCard);
        ui->toutaGiftSettingsCard->hide();
        ui->scrollArea->removeWidget(ui->toutaGiftSettingsCard);
    }

    // 粉丝勋章
    ui->autoSwitchMedalCheck->setChecked(us->value("danmaku/autoSwitchMedal", false).toBool());

    // 滚屏
    ui->enableScreenDanmakuCheck->setChecked(us->value("screendanmaku/enableDanmaku", false).toBool());
    ui->enableScreenMsgCheck->setChecked(us->value("screendanmaku/enableMsg", false).toBool());
    ui->screenDanmakuWithNameCheck->setChecked(us->value("screendanmaku/showName", true).toBool());
    ui->screenDanmakuLeftSpin->setValue(us->value("screendanmaku/left", 0).toInt());
    ui->screenDanmakuRightSpin->setValue(us->value("screendanmaku/right", 0).toInt());
    ui->screenDanmakuTopSpin->setValue(us->value("screendanmaku/top", 10).toInt());
    ui->screenDanmakuBottomSpin->setValue(us->value("screendanmaku/bottom", 60).toInt());
    ui->screenDanmakuSpeedSpin->setValue(us->value("screendanmaku/speed", 10).toInt());
    ui->enableScreenMsgCheck->setEnabled(ui->enableScreenDanmakuCheck->isChecked());
    ui->screenDanmakuWithNameCheck->setEnabled(ui->enableScreenDanmakuCheck->isChecked());
    QString danmakuFontString = us->value("screendanmaku/font").toString();
    if (!danmakuFontString.isEmpty())
        screenDanmakuFont.fromString(danmakuFontString);
    screenDanmakuColor = qvariant_cast<QColor>(us->value("screendanmaku/color", QColor(0, 0, 0)));

    // 自动签到
    ui->autoDoSignCheck->setChecked(us->autoDoSign = us->value("danmaku/autoDoSign", false).toBool());

    // 自动参与天选
    ui->autoLOTCheck->setChecked(us->autoJoinLOT = us->value("danmaku/autoLOT", false).toBool());

    // 自动获取小心心
    ui->acquireHeartCheck->setChecked(us->value("danmaku/acquireHeart", false).toBool());
    ui->heartTimeSpin->setValue(us->getHeartTimeCount = us->value("danmaku/acquireHeartTime", 120).toInt());

    // 自动赠送过期礼物
    ui->sendExpireGiftCheck->setChecked(us->value("danmaku/sendExpireGift", false).toBool());

    // 永久禁言
    us->eternalBlockUsers.clear();
    QJsonArray eternalBlockArray = us->value("danmaku/eternalBlockUsers").toJsonArray();
    int eternalBlockSize = eternalBlockArray.size();
    for (int i = 0; i < eternalBlockSize; i++)
    {
        EternalBlockUser eb = EternalBlockUser::fromJson(eternalBlockArray.at(i).toObject());
        if (eb.uid && eb.roomId)
            us->eternalBlockUsers.append(eb);
    }

#ifndef ZUOQI_ENTRANCE
    // 开机自启
    ui->startOnRebootCheck->setChecked(us->value("runtime/startOnReboot", false).toBool());
    // 自动更新
    ui->autoUpdateCheck->setChecked(us->value("runtime/autoUpdate", !rt->asPlugin).toBool());
    ui->autoUpdateCheck->setEnabled(!rt->asPlugin);
    ui->showChangelogCheck->setChecked(us->value("runtime/showChangelog", true).toBool());
    ui->updateBetaCheck->setChecked(us->value("runtime/updateBeta", false).toBool());
#endif

    // 互动玩法
    ui->identityCodeEdit->setText(ac->identityCode = us->value("live-open/identityCode").toString());
    ui->liveOpenCheck->setChecked(us->value("live-open/enabled").toBool());

    // 数据清理
    ui->autoClearComeIntervalSpin->setValue(us->value("danmaku/clearDidntComeInterval", 7).toInt());

    // 调试模式
    us->localMode = us->value("debug/localDebug", false).toBool();
    ui->actionLocal_Mode->setChecked(us->localMode);
    us->debugPrint = us->value("debug/debugPrint", false).toBool();
    ui->actionDebug_Mode->setChecked(us->debugPrint);
    saveRecvCmds = us->value("debug/saveRecvCmds", false).toBool();
    ui->saveRecvCmdsCheck->setChecked(saveRecvCmds);
    if (saveRecvCmds)
        on_saveRecvCmdsCheck_clicked();
    ui->saveLogCheck->setChecked(us->value("debug/logFile", false).toBool());

    // 模拟CMDS
    ui->timerPushCmdCheck->setChecked(us->value("debug/pushCmdsTimer", false).toBool());
    ui->timerPushCmdSpin->setValue(us->value("debug/pushCmdsInterval", 10).toInt());

    if (!us->value("danmaku/copyright", false).toBool())
    {
        /* if (shallAutoMsg() && (ui->autoSendWelcomeCheck->isChecked() || ui->autoSendGiftCheck->isChecked() || ui->autoSendAttentionCheck->isChecked()))
        {
            localNotify(QString(QByteArray::fromBase64("44CQ"))
                        +QApplication::applicationName()
                        +QByteArray::fromBase64("44CR5Li65oKo5pyN5Yqhfg=="));
        } */
    }

    // 开启服务端
    bool enableServer = us->value("server/enabled", false).toBool();
    ui->serverCheck->setChecked(enableServer);
    int port = us->value("server/port", 5520).toInt();
    ui->serverPortSpin->setValue(port);
    webServer->serverDomain = us->value("server/domain", "localhost").toString();
    ui->allowWebControlCheck->setChecked(us->value("server/allowWebControl", false).toBool());
    ui->allowRemoteControlCheck->setChecked(us->remoteControl = us->value("danmaku/remoteControl", true).toBool());
    ui->allowAdminControlCheck->setChecked(us->value("danmaku/adminControl", false).toBool());
    ui->domainEdit->setText(webServer->serverDomain);
    if (enableServer)
    {
        openServer();
    }

    // 加载网页
    loadWebExtensionList();

    // 设置默认配置
    if (rt->firstOpen)
    {
        readDefaultCode();
        us->setValue("runtime/first_use_time", QDateTime::currentSecsSinceEpoch());
    }

    // 恢复游戏数据
    restoreGameNumbers();
    restoreGameTexts();
}

/**
 * 第二次读取设置
 * 登录后再进行
 */
void MainWindow::readConfig2()
{
    // 数据库
    saveToSqlite = hasPermission() && us->value("db/sqlite").toBool();
    ui->saveToSqliteCheck->setChecked(saveToSqlite);
    ui->saveCmdToSqliteCheck->setChecked(saveCmdToSqlite = us->value("db/cmd").toBool());
    ui->saveCmdToSqliteCheck->setEnabled(saveToSqlite);
    if (saveToSqlite)
        sqlService.open();
}

void MainWindow::initEvent()
{
    initLiveRecord();

    // 点歌
    connect(this, SIGNAL(signalNewDanmaku(const LiveDanmaku&)), this, SLOT(slotDiange(const LiveDanmaku&)));

    // 滚屏
    connect(this, &MainWindow::signalNewDanmaku, this, [=](const LiveDanmaku &danmaku){
        showScreenDanmaku(danmaku);
    });

    connect(this, &MainWindow::signalNewDanmaku, this, [=](const LiveDanmaku &danmaku){
        if (danmaku.isPkLink()) // 大乱斗对面的弹幕不朗读
            return ;
        if (!_loadingOldDanmakus && ui->autoSpeekDanmakuCheck->isChecked() && danmaku.getMsgType() == MSG_DANMAKU
                && cr->shallSpeakText())
        {
            if (cr->hasSimilarOldDanmaku(danmaku.getText()))
                return ;
            speakText(danmaku.getText());
        }
    });
}

void MainWindow::initCodeRunner()
{
    connect(cr, &CodeRunner::signalTriggerCmdEvent, this, [=](const QString& cmd, const LiveDanmaku& danmaku, bool debug) {
        triggerCmdEvent(cmd, danmaku, debug);
    });

    connect(cr, &CodeRunner::signalLocalNotify, this, [=](const QString& text, qint64 uid) {
        localNotify(text, uid);
    });

    connect(cr, &CodeRunner::signalShowError, this, [=](const QString& title, const QString& info) {
        showError(title, info);
    });
    connect(cr, &CodeRunner::signalSpeakText, this, [=](const QString& text) {
        speakText(text);
    });

    cr->execFuncCallback = [=](QString msg, LiveDanmaku &danmaku, CmdResponse &res, int &resVal) -> bool{
        return this->execFunc(msg, danmaku, res, resVal);
    };
}

void MainWindow::initWebServer()
{
    webServer = new WebServer(this);
    webServer->wwwDir = QDir(rt->dataPath + "www");
    cr->setWebServer(webServer);
}

void MainWindow::initVoiceService()
{
    voiceService = new VoiceService(this);
    cr->setVoiceService(voiceService);
}

void MainWindow::initChatService()
{
    chatService = new ChatService(this);
}

void MainWindow::adjustPageSize(int page)
{
    if (page == PAGE_ROOM)
    {
        // 自动调整封面大小
        if (!liveService->roomCover.isNull())
        {
            adjustCoverSizeByRoomCover(liveService->roomCover);

            /* int w = ui->roomCoverLabel->width();
            if (w > ui->tabWidget->contentsRect().width())
                w = ui->tabWidget->contentsRect().width();
            pixmap = pixmap.scaledToWidth(w, Qt::SmoothTransformation);
            ui->roomCoverLabel->setPixmap(PixmapUtil::getRoundedPixmap(pixmap));
            ui->roomCoverLabel->setMinimumSize(1, 1); */
        }
    }
    else if (page == PAGE_DANMAKU)
    {

    }
    else if (page == PAGE_THANK)
    {
        customVarsButton->move(ui->thankStackedWidget->geometry().right() - customVarsButton->width(),
                               ui->thankTopTabGroup->y());
    }
    else if (page == PAGE_MUSIC)
    {
        {
            QRect g = ui->musicBigTitleLabel->geometry();
            musicTitleDecorateWidget->move(g.right() - g.width()/4, g.top() - musicTitleDecorateWidget->height()/2 + g.height());
        }
    }
    else if (page == PAGE_EXTENSION)
    {
        extensionButton->move(ui->tabWidget->width() - extensionButton->height(), 0);

        appendListItemButton->move(ui->tabWidget->width() - appendListItemButton->width() * 1.25 - 9 - 15, // 减去滚动条宽度
                                   ui->tabWidget->height() - appendListItemButton->height() * 1.25 - 9); // 还有减去margin

        // 自动调整列表大小
        int tabIndex = ui->tabWidget->currentIndex();
        if (tabIndex == TAB_TIMER_TASK)
        {
            static bool first = false;
            if (!first)
            {
                first = true;
                QTimer::singleShot(0, [=]{
                    ui->taskListWidget->verticalScrollBar()->setSliderPosition(us->i("mainwindow/timerTaskScroll"));
                });
            }
            for (int row = 0; row < ui->taskListWidget->count(); row++)
            {
                auto item = ui->taskListWidget->item(row);
                auto widget = ui->taskListWidget->itemWidget(item);
                if (!widget)
                    continue;
                QSize size(ui->taskListWidget->contentsRect().width() - ui->taskListWidget->verticalScrollBar()->width(), widget->height());
                auto taskWidget = static_cast<TaskWidget*>(widget);
                taskWidget->resize(size);
                taskWidget->autoResizeEdit();
            }
        }
        else if (tabIndex == TAB_AUTO_REPLY)
        {
            static bool first = false;
            if (!first)
            {
                first = true;
                QTimer::singleShot(0, [=]{
                    ui->replyListWidget->verticalScrollBar()->setSliderPosition(us->i("mainwindow/autoReplyScroll"));
                });
            }
            for (int row = 0; row < ui->replyListWidget->count(); row++)
            {
                auto item = ui->replyListWidget->item(row);
                auto widget = ui->replyListWidget->itemWidget(item);
                if (!widget)
                    continue;
                QSize size(ui->replyListWidget->contentsRect().width() - ui->replyListWidget->verticalScrollBar()->width(), widget->height());
                auto replyWidget = static_cast<ReplyWidget*>(widget);
                replyWidget->resize(size);
                replyWidget->autoResizeEdit();
            }
        }
        else if (tabIndex == TAB_EVENT_ACTION)
        {
            static bool first = false;
            if (!first)
            {
                first = true;
                QTimer::singleShot(0, [=]{
                    ui->eventListWidget->verticalScrollBar()->setSliderPosition(us->i("mainwindow/eventActionScroll"));
                });
            }
            for (int row = 0; row < ui->eventListWidget->count(); row++)
            {
                auto item = ui->eventListWidget->item(row);
                auto widget = ui->eventListWidget->itemWidget(item);
                if (!widget)
                    continue;
                QSize size(ui->eventListWidget->contentsRect().width() - ui->eventListWidget->verticalScrollBar()->width(), widget->height());
                auto eventWidget = static_cast<EventWidget*>(widget);
                eventWidget->resize(size);
                eventWidget->autoResizeEdit();
            }
        }
    }
    else if (page == PAGE_PREFENCE)
    {

    }
}

/// 显示动画
/// 在不影响布局的基础上
void MainWindow::switchPageAnimation(int page)
{
    // 房间ID，可能经常触发
    if (page == PAGE_ROOM)
    {
        adjustRoomIdWidgetPos();
        showRoomIdWidget();
        if (!liveService->roomCover.isNull())
            adjustCoverSizeByRoomCover(liveService->roomCover);
    }
    else
    {
        hideRoomIdWidget();
    }

    auto startGeometryAni = [=](QWidget* widget, QVariant start, QVariant end, int duration = 400, QEasingCurve curve = QEasingCurve::OutCubic){
        if (widget->isHidden())
            widget->show();
        QPropertyAnimation* ani = new QPropertyAnimation(widget, "geometry");
        ani->setStartValue(start);
        ani->setEndValue(end);
        ani->setEasingCurve(curve);
        ani->setDuration(duration);
        connect(ani, &QPropertyAnimation::stateChanged, ani, [=](QAbstractAnimation::State newState, QAbstractAnimation::State oldState){
            if (newState != QAbstractAnimation::Running)
                ani->deleteLater();
        });
        ani->start();
    };

    auto startGeometryAniDelay = [&](int delay, QWidget* widget, QVariant start, QVariant end, int duration = 400, QEasingCurve curve = QEasingCurve::OutCubic){
        widget->resize(0, 0);
        QTimer::singleShot(delay, [=]{
            startGeometryAni(widget, start, end, duration, curve);
        });
    };

    if (page == PAGE_ROOM)
    {
        if (ui->upHeaderLabel->pixmap())
        {
            QLabel* label = new QLabel(this);
            label->setFixedSize(ui->upHeaderLabel->size());
            QPixmap pixmap = *ui->upHeaderLabel->pixmap();
            label->setPixmap(pixmap);
            label->show();
            QPoint pos(ui->upHeaderLabel->mapTo(this, QPoint(0, 0)));

            QPropertyAnimation* ani = new QPropertyAnimation(label, "pos");
            ani->setStartValue(QPoint(pos.x(), this->height()));
            ani->setEndValue(pos);
            ani->setEasingCurve(QEasingCurve::OutBack);
            ani->setDuration(400);
            connect(ani, &QPropertyAnimation::finished, this, [=]{
                ani->deleteLater();
                label->deleteLater();
                ui->upHeaderLabel->setPixmap(PixmapUtil::toCirclePixmap(pixmap));
            });
            ani->start();

            QPixmap emptyPixmap(pixmap.size());
            emptyPixmap.fill(Qt::transparent);
            ui->upHeaderLabel->setStyleSheet("");
            ui->upHeaderLabel->setPixmap(emptyPixmap);
        }

    }
    else if (page == PAGE_DANMAKU)
    {
        static bool restored = false;
        if (!restored)
        {
            restored = true;
            QTimer::singleShot(0, [=]{
                ui->scrollArea->verticalScrollBar()->setSliderPosition(us->i("mainwindow/configFlowScroll"));
            });
        }

        QRect g(ui->danmakuTitleSplitWidget1->geometry());
        startGeometryAni(ui->danmakuTitleSplitWidget1,
                         QRect(g.right()-1, g.top(), 1, 1), g);

        g = QRect(ui->danmakuTitleSplitWidget2->geometry());
        startGeometryAni(ui->danmakuTitleSplitWidget2,
                         QRect(g.left(), g.top(), 1, 1), g);
    }
    else if (page == PAGE_THANK)
    {
        QRect g(ui->thankTopTabGroup->geometry());
        startGeometryAni(ui->thankTopTabGroup,
                         QRect(g.center().x(), g.top(), 1, g.height()), g);
    }
    else if (page == PAGE_MUSIC)
    {
//        ui->musicContainerWidget->layout()->setEnabled(false);
//        QTimer::singleShot(900, [=]{
//            ui->musicContainerWidget->layout()->setEnabled(true);
//        });

        // 标题装饰文字背景动画
        QRect g(musicTitleDecorateWidget->geometry());
        startGeometryAni(musicTitleDecorateWidget,
                         QRect(g.left(), -g.height(), g.width(), g.height()), g);

        // 下划线展开动画
        g = QRect(ui->musicSplitWidget1->geometry());
        startGeometryAniDelay(500, ui->musicSplitWidget1,
                         QRect(g.center().x(), g.top(), 1, 1), g);

        // 没开启的话总开关动画高亮
        if (!ui->DiangeAutoCopyCheck->isChecked())
        {
            g = ui->DiangeAutoCopyCheck->geometry();
            startGeometryAniDelay(200, ui->DiangeAutoCopyCheck,
                             QRect(g.left() - g.width(), g.top(), g.width(), g.height()), g,
                                  600, QEasingCurve::OutBounce);
        }

        // 三个小功能开关的设置项
//        ui->label_14->hide();
//        ui->label_43->hide();
//        ui->label_44->hide();
//        ui->label_14->setMinimumSize(0, 0);
//        ui->label_43->setMinimumSize(0, 0);
//        ui->label_44->setMinimumSize(0, 0);
//        g = ui->label_41->geometry();
//        startGeometryAni(ui->label_41,
//                         QRect(g.center(), QPoint(1, 1)), g);
//        g = ui->label_43->geometry();
//        startGeometryAniDelay(300, ui->label_43,
//                         QRect(g.center(), QPoint(1, 1)), g);
//        g = ui->label_44->geometry();
//        startGeometryAniDelay(600, ui->label_44,
//                         QRect(g.center(), QPoint(1, 1)), g);

    }
    else if (page == PAGE_EXTENSION)
    {
        QRect g(appendListItemButton->geometry());
        if (!appendListItemButton->isHidden())
        {
            startGeometryAni(appendListItemButton,
                             QRect(g.left(), ui->tabWidget->height(), g.width(), g.height()), g,
                             400, QEasingCurve::OutBack);
        }
    }
    else if (page == PAGE_PREFENCE)
    {
        QRect g(ui->label_36->geometry());
        startGeometryAni(ui->label_36,
                         QRect(g.center(), QSize(1,1)), g,
                         400, QEasingCurve::OutCubic);

        g = QRect(ui->label_39->geometry());
        startGeometryAniDelay(400, ui->label_39,
                         QRect(g.center(), QSize(1,1)), g,
                         400, QEasingCurve::OutCubic);
    }
}

MainWindow::~MainWindow()
{
    // 保存配置
    if (liveService->danmuLogFile)
    {
        liveService->finishSaveDanmuToFile();
    }
    if (us->calculateDailyData)
    {
        liveService->saveCalculateDailyData();
    }
    us->setValue("mainwindow/configFlowScroll", ui->scrollArea->verticalScrollBar()->sliderPosition());
    us->setValue("mainwindow/timerTaskScroll", ui->taskListWidget->verticalScrollBar()->sliderPosition());
    us->setValue("mainwindow/autoReplyScroll", ui->replyListWidget->verticalScrollBar()->sliderPosition());
    us->setValue("mainwindow/eventActionScroll", ui->eventListWidget->verticalScrollBar()->sliderPosition());
    us->sync();

    // 关闭窗口
    if (danmakuWindow)
    {
        danmakuWindow->close();
        danmakuWindow->deleteLater();
        danmakuWindow = nullptr;
    }

    if (musicWindow)
    {
        delete musicWindow;
        musicWindow = nullptr;
    }

    /*if (playerWindow)
    {
        settings->setValue("danmaku/playerWindow", !playerWindow->isHidden());
        playerWindow->close();
        playerWindow->deleteLater();
    }*/

    triggerCmdEvent("SHUT_DOWN", LiveDanmaku(), true);

    // 清理过期备份
    auto files = QDir(rt->dataPath + "backup").entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
    qint64 overdue = QDateTime::currentSecsSinceEpoch() - 3600 * 24 * qMax(ui->autoClearComeIntervalSpin->value(), 7); // 至少备份7天
    foreach (auto info, files)
    {
        if (info.lastModified().toSecsSinceEpoch() < overdue)
        {
            qInfo() << "删除备份：" << info.absoluteFilePath();
            deleteFile(info.absoluteFilePath());
        }
    }

    // 删除缓存
    if (isFileExist(webCache("")))
        deleteDir(webCache(""));

    // 删除界面
    delete ui;
    tray->deleteLater();

    // 自动更新
    QString pkgPath = QApplication::applicationDirPath() + "/update.zip";
    if (isFileExist(pkgPath) && QFileInfo(pkgPath).size() > 100 * 1024) // 起码大于100K，可能没更新完
    {
        // 更新历史版本
        qInfo() << "检测到已下载的安装包，进行更新";
        QString appPath = QApplication::applicationDirPath();
        QProcess process;
        process.startDetached(UPDATE_TOOL_NAME, { "-u", pkgPath, appPath, "-d", "-8", "-4"} );
    }
}

const QSettings* MainWindow::getSettings() const
{
    return us;
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    static bool firstShow = true;
    if (firstShow)
    {
        firstShow = false;

        // 恢复窗口位置
        restoreGeometry(us->value("mainwindow/geometry").toByteArray());

        // 显示启动动画
        startSplash();

        // 显示 RoomIdEdit 动画
        adjustRoomIdWidgetPos();
        if (ui->stackedWidget->currentIndex() != 0)
            roomIdBgWidget->hide();
    }
    us->setValue("mainwindow/autoShow", true);

    if (!liveService->roomCover.isNull())
        adjustCoverSizeByRoomCover(liveService->roomCover);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    us->setValue("mainwindow/geometry", this->saveGeometry());
    us->sync();

#if defined(ENABLE_TRAY)
    if (!tray->isVisible())
    {
        prepareQuit();
        return ;
    }

    event->ignore();
    this->hide();
    QTimer::singleShot(5000, [=]{
        if (!this->isHidden())
            return ;
        us->setValue("mainwindow/autoShow", false);
    });
#else
    QMainWindow::closeEvent(event);
#endif
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    adjustPageSize(ui->stackedWidget->currentIndex());
    tip_box->adjustPosition();

    if (fakeEntrance)
    {
        fakeEntrance->setGeometry(this->rect());
    }
}

void MainWindow::changeEvent(QEvent *event)
{

    QMainWindow::changeEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    QLinearGradient linearGrad(QPointF(width() / 2, 0), QPointF(width() / 2, height()));
    linearGrad.setColorAt(0, themeBg);
    double pos = 1.0;
    if (liveService->dailyMaxPopul > 100 && liveService->dailyAvePopul > 100)
    {
        pos = liveService->dailyAvePopul / double(liveService->dailyMaxPopul);
        pos = qMin(qMax(pos, 0.3), 1.0);
    }
    linearGrad.setColorAt(pos, themeGradient);
//    linearGrad.setColorAt(0, themeGradient);
//    linearGrad.setColorAt(1, Qt::white);
    painter.fillRect(rect(), linearGrad);
}

/// 恢复之前的弹幕
void MainWindow::pullLiveDanmaku()
{
    if (ac->roomId.isEmpty())
        return ;
    QString url = "https://api.live.bilibili.com/ajax/msg";
    QStringList param{"roomid", ac->roomId};
    connect(new NetUtil(url, param), &NetUtil::finished, this, [=](QString result){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(result.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << "pullLiveDanmaku.ERROR:" << error.errorString();
            qCritical() << result;
            return ;
        }

        _loadingOldDanmakus = true;
        QJsonObject json = document.object();
        QJsonArray danmakus = json.value("data").toObject().value("room").toArray();
        QDateTime time = QDateTime::currentDateTime();
        qint64 removeTime = time.toMSecsSinceEpoch() - removeDanmakuInterval;
        for (int i = 0; i < danmakus.size(); i++)
        {
            LiveDanmaku danmaku = LiveDanmaku::fromDanmakuJson(danmakus.at(i).toObject());
            if (danmaku.getTimeline().toMSecsSinceEpoch() < removeTime)
                continue;
            danmaku.transToDanmu();
            danmaku.setTime(time);
            danmaku.setNoReply();
            appendNewLiveDanmaku(danmaku);
        }
        _loadingOldDanmakus = false;
    });
}

void MainWindow::removeTimeoutDanmaku()
{
    if (pushCmdsFile && liveService->roomDanmakus.size() < 1000) // 不移除弹幕
        return ;

    // 移除过期队列，单位：毫秒
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    qint64 removeTime = timestamp - removeDanmakuInterval;
    for (int i = 0; i < liveService->roomDanmakus.size(); i++)
    {
        auto danmaku = liveService->roomDanmakus.at(i);
        QDateTime dateTime = liveService->roomDanmakus.first().getTimeline();
        if (dateTime.toMSecsSinceEpoch() < removeTime)
        {
            auto danmaku = liveService->roomDanmakus.takeFirst();
            oldLiveDanmakuRemoved(danmaku);
        }
    }

    // 移除多余的提示（一般时间更短）
    removeTime = timestamp - removeDanmakuTipInterval;
    for (int i = 0; i < liveService->roomDanmakus.size(); i++)
    {
        auto danmaku = liveService->roomDanmakus.at(i);
        auto type = danmaku.getMsgType();
        // 提示类型
        if (type == MSG_ATTENTION || type == MSG_WELCOME || type == MSG_FANS
                || (type == MSG_GIFT && (!danmaku.isGoldCoin() || danmaku.getTotalCoin() < 1000))
                || (type == MSG_DANMAKU && danmaku.isNoReply())
                || type == MSG_MSG
                || danmaku.isToView() || danmaku.isPkLink())
        {
            QDateTime dateTime = danmaku.getTimeline();
            if (dateTime.toMSecsSinceEpoch() < removeTime)
            {
                liveService->roomDanmakus.removeAt(i--);
                oldLiveDanmakuRemoved(danmaku);
                // break; // 不break，就是一次性删除多个
            }
            else
            {
                break;
            }
        }
    }
}

void MainWindow::appendNewLiveDanmakus(const QList<LiveDanmaku> &danmakus)
{
    // 添加到队列
    liveService->roomDanmakus.append(danmakus);
    rt->allDanmakus.append(danmakus);
}

void MainWindow::appendNewLiveDanmaku(const LiveDanmaku &danmaku)
{
    liveService->roomDanmakus.append(danmaku);
    lastDanmaku = danmaku;
    rt->allDanmakus.append(danmaku);
    newLiveDanmakuAdded(danmaku);
}

void MainWindow::newLiveDanmakuAdded(const LiveDanmaku &danmaku)
{
    SOCKET_DEB << "+++++新弹幕：" << danmaku.toString();
    emit signalNewDanmaku(danmaku);

    // 保存到文件
    if (liveService->danmuLogStream)
    {
        if (danmaku.is(MSG_DEF) && danmaku.getText().startsWith("["))
            return ;
        (*liveService->danmuLogStream) << danmaku.toString() << "\n";
        (*liveService->danmuLogStream).flush(); // 立刻刷新到文件里
    }
}

void MainWindow::oldLiveDanmakuRemoved(const LiveDanmaku &danmaku)
{
    SOCKET_DEB << "-----旧弹幕：" << danmaku.toString();
    emit signalRemoveDanmaku(danmaku);
}

void MainWindow::localNotify(const QString &text)
{
	if (text.isEmpty())
    {
        qWarning() << ">localNotify([空文本])";
        return ;
    }
    appendNewLiveDanmaku(LiveDanmaku(text));
}

void MainWindow::localNotify(const QString &text, qint64 uid)
{
    LiveDanmaku danmaku(text);
    danmaku.setUid(uid);
    appendNewLiveDanmaku(danmaku);
}

/**
 * 连击定时器到达
 */
void MainWindow::slotComboSend()
{
    if (!liveService->giftCombos.size())
    {
        liveService->comboTimer->stop();
        return ;
    }

    qint64 timestamp = QDateTime::currentSecsSinceEpoch();
    int delta = ui->giftComboDelaySpin->value();

    auto thankGift = [=](LiveDanmaku danmaku) -> bool {
        QStringList words = cr->getEditConditionStringList(ui->autoThankWordsEdit->toPlainText(), danmaku);
        if (words.size())
        {
            int r = qrand() % words.size();
            QString msg = words.at(r);
            if (us->strongNotifyUsers.contains(danmaku.getUid()))
            {
                if (us->debugPrint)
                    localNotify("[强提醒]");
                cr->sendCdMsg(msg, danmaku, NOTIFY_CD, GIFT_CD_CN,
                          ui->sendGiftTextCheck->isChecked(), ui->sendGiftVoiceCheck->isChecked(), false);
            }
            else
            {
                cr->sendGiftMsg(msg, danmaku);
            }
            return true;
        }
        else if (us->debugPrint)
        {
            localNotify("[没有可发送的连击答谢弹幕]");
            return false;
        }
        return false;
    };

    if (!ui->giftComboTopCheck->isChecked()) // 全部答谢，看冷却时间
    {
        for (auto it = liveService->giftCombos.begin(); it != liveService->giftCombos.end(); )
        {
            const LiveDanmaku& danmaku = it.value();
            if (danmaku.getTimeline().toSecsSinceEpoch() + delta > timestamp) // 未到达统计时间
            {
                it++;
                continue;
            }

            // 开始答谢该项
            thankGift(danmaku);
            it = liveService->giftCombos.erase(it);
        }
    }
    else // 等待结束，只答谢最贵的一项
    {
        qint64 maxGold = 0;
        auto maxIt = liveService->giftCombos.begin();
        for (auto it = liveService->giftCombos.begin(); it != liveService->giftCombos.end(); it++)
        {
            const LiveDanmaku& danmaku = it.value();
            if (danmaku.getTimeline().toSecsSinceEpoch() + delta > timestamp) // 还有礼物未到答谢时间
                return ;
            if (danmaku.isGoldCoin() && danmaku.getTotalCoin() > maxGold)
            {
                maxGold = danmaku.getTotalCoin();
                maxIt = it;
            }
        }
        if (maxGold > 0)
        {
            thankGift(maxIt.value());
            liveService->giftCombos.clear();
            return ;
        }

        // 没有金瓜子，只有银瓜子
        int maxSilver = 0;
        maxIt = liveService->giftCombos.begin();
        for (auto it = liveService->giftCombos.begin(); it != liveService->giftCombos.end(); it++)
        {
            const LiveDanmaku& danmaku = it.value();
            if (danmaku.getTotalCoin() > maxGold)
            {
                maxSilver = danmaku.getTotalCoin();
                maxIt = it;
            }
        }
        thankGift(maxIt.value());
        liveService->giftCombos.clear();
        return ;
    }

}

void MainWindow::on_DiangeAutoCopyCheck_stateChanged(int)
{
    us->setValue("danmaku/diangeAutoCopy", diangeAutoCopy = ui->DiangeAutoCopyCheck->isChecked());
}

void MainWindow::on_testDanmakuButton_clicked()
{
    QString text = ui->testDanmakuEdit->text();
    if (text.isEmpty())
        text = "测试弹幕";
    QRegularExpressionMatch match;

    qint64 uid = 123;
    int r = qrand() % 7 + 1;
    if (text.startsWith("$"))
    {
        text.replace(0, 1, "");
        uid = ac->cookieUid.toLongLong();
    }
    else
    {
        uid = 10000 + r;
    }

    if (text == "赠送吃瓜")
    {
        sendGift(20004, 1);
    }
    else if (text == "测试送礼")
    {
        QString username = "测试用户";
        QString giftName = "测试礼物";
        int giftId = 123;
        int num = qrand() % 3 + 1;
        qint64 timestamp = QDateTime::currentSecsSinceEpoch();
        QString coinType = qrand()%2 ? "gold" : "silver";
        int totalCoin = qrand() % 20 * 100;
        LiveDanmaku danmaku(username, giftId, giftName, num, uid, QDateTime::fromSecsSinceEpoch(timestamp), coinType, totalCoin);
        appendNewLiveDanmaku(danmaku);
        if (ui->saveEveryGiftCheck->isChecked())
            saveEveryGift(danmaku);

        QStringList words = cr->getEditConditionStringList(ui->autoThankWordsEdit->toPlainText(), danmaku);
        qInfo() << "条件替换结果：" << words;

        triggerCmdEvent("SEND_GIFT", danmaku, true);
    }
    else if (text == "测试礼物连击")
    {
        QString username = "测试用户";
        QString giftName = "测试礼物";
        int giftId = 123;
        int num = qrand() % 3 + 1;
        qint64 timestamp = QDateTime::currentSecsSinceEpoch();
        QString coinType = qrand()%2 ? "gold" : "silver";
        int totalCoin = qrand() % 20 * 100;
        uid = 123456;
        LiveDanmaku danmaku(username, giftId, giftName, num, uid, QDateTime::fromSecsSinceEpoch(timestamp), coinType, totalCoin);

        bool merged = mergeGiftCombo(danmaku); // 如果有合并，则合并到之前的弹幕上面
        if (!merged)
        {
            appendNewLiveDanmaku(danmaku);
        }
    }
    else if (text == "测试醒目留言")
    {
        LiveDanmaku danmaku("测试用户", "测试弹幕", 1001, 12, QDateTime::currentDateTime(), "", "", 39, "醒目留言", 1, 30);
        appendNewLiveDanmaku(danmaku);
    }
    else if (text == "测试消息")
    {
        localNotify("测试通知消息");
    }
    else if (text == "测试舰长进入")
    {
        QString uname = "测试舰长";
        LiveDanmaku danmaku(qrand() % 3 + 1, uname, uid, QDateTime::currentDateTime());
        appendNewLiveDanmaku(danmaku);

        if (ui->autoSendWelcomeCheck->isChecked() && !us->notWelcomeUsers.contains(uid))
        {
            sendWelcome(danmaku);
        }
    }
    else if (text == "测试舰长")
    {
        QString username = "测试用户";
        QString giftName = "舰长";
        int num = qrand() % 3 + 1;
        int gift_id = 10003;
        int guard_level = 3;
        int price = 198000;
        int rg = qrand() % 6;
        if (rg < 3)
        {
            giftName = "舰长";
            guard_level = 3;
            price = 198000;
        }
        else if (rg < 5)
        {
            giftName = "提督";
            guard_level = 2;
            price = 1980000;
        }
        else
        {
            giftName = "总督";
            guard_level = 1;
            price = 19800000;
        }
        gift_id = 10000 + guard_level;
        LiveDanmaku danmaku(username, uid, giftName, num, guard_level, gift_id, price, 2);
        appendNewLiveDanmaku(danmaku);
        appendLiveGuard(danmaku);
        if (ui->saveEveryGuardCheck->isChecked())
            saveEveryGuard(danmaku);

        if (!justStart && ui->autoSendGiftCheck->isChecked())
        {
            QStringList words = cr->getEditConditionStringList(ui->autoThankWordsEdit->toPlainText(), danmaku);
            if (words.size())
            {
                int r = qrand() % words.size();
                QString msg = words.at(r);
                cr->sendCdMsg(msg, danmaku, NOTIFY_CD, NOTIFY_CD_CN,
                          ui->sendGiftTextCheck->isChecked(), ui->sendGiftVoiceCheck->isChecked(), false);
            }
            else if (us->debugPrint)
            {
                localNotify("[没有可发送的上船弹幕]");
            }
        }
    }
    else if (text.startsWith(">"))
    {
        processRemoteCmd(text.replace(0, 1, ""));
    }
    else if (text == "测试开播")
    {
        QString text = ui->startLiveWordsEdit->text();
        if (ui->startLiveSendCheck->isChecked() && !text.trimmed().isEmpty())
            cr->sendAutoMsg(text, LiveDanmaku());
        ui->liveStatusButton->setText("已开播");
        ac->liveStatus = 1;
        if (ui->timerConnectServerCheck->isChecked() && liveService->connectServerTimer->isActive())
            liveService->connectServerTimer->stop();
        slotStartWork(); // 每个房间第一次开始工作

        triggerCmdEvent("LIVE", LiveDanmaku(), true);
    }
    else if (text == "测试对面主播进入")
    {
        QJsonObject json;
        json.insert("cmd", "INTERACT_WORD");
        QJsonObject data;
        data.insert("msg_type", 1);
        data.insert("timestamp", QDateTime::currentSecsSinceEpoch());
        data.insert("uid", liveService->pkUid.toLongLong());
        data.insert("uname", "测试用户");
        json.insert("data", data);
        handleMessage(json);
        qInfo() << liveService->pkUid << liveService->oppositeAudience;
    }
    else if (text == "测试花灯")
    {
        QString username = "测试用户";
        QString giftName = "炫彩花灯";
        int r = qrand() % 5;
        if (r == 1)
            giftName = "漫天花灯";
        else if (r == 2)
            giftName = "暴富花灯";
        int giftId = 123;
        int num = qrand() % 3 + 1;
        qint64 timestamp = QDateTime::currentSecsSinceEpoch();
        QString coinType = qrand()%2 ? "gold" : "silver";
        int totalCoin = qrand() % 20 * 100;
        uid = 123456;
        LiveDanmaku danmaku(username, giftId, giftName, num, uid, QDateTime::fromSecsSinceEpoch(timestamp), coinType, totalCoin);
        appendNewLiveDanmaku(danmaku);
        triggerCmdEvent("SEND_GIFT", danmaku, true);
    }
    else if (text == "测试高能榜")
    {
        QByteArray ba = "{ \"cmd\": \"ENTRY_EFFECT\", \"data\": { \"basemap_url\": \"https://i0.hdslb.com/bfs/live/mlive/586f12135b6002c522329904cf623d3f13c12d2c.png\", \"business\": 3, \"copy_color\": \"#000000\", \"copy_writing\": \"欢迎 <%___君陌%> 进入直播间\", \"copy_writing_v2\": \"欢迎 <^icon^> <%___君陌%> 进入直播间\", \"effective_time\": 2, \"face\": \"https://i1.hdslb.com/bfs/face/8fb8336e1ae50001ca76b80c30b01d23b07203c9.jpg\", \"highlight_color\": \"#FFF100\", \"icon_list\": [ 2 ], \"id\": 136, \"max_delay_time\": 7, \"mock_effect\": 0, \"priority\": 1, \"privilege_type\": 0, \"show_avatar\": 1, \"target_id\": 5988102, \"uid\": 453364, \"web_basemap_url\": \"https://i0.hdslb.com/bfs/live/mlive/586f12135b6002c522329904cf623d3f13c12d2c.png\", \"web_close_time\": 900, \"web_effect_close\": 0, \"web_effective_time\": 2 } }";
        handleMessage(QJsonDocument::fromJson(ba).object());
    }
    else if (text == "测试对面信息")
    {
        getPkMatchInfo();
    }
    else if (text == "测试对面舰长")
    {
        getPkOnlineGuardPage(0);
    }
    else if (text == "测试偷塔")
    {
        execTouta();
    }
    else if (text == "测试心跳")
    {
        auto biliLive = static_cast<BiliLiveService*>(liveService);
        if (biliLive)
        {
            if (biliLive->xliveHeartBeatBenchmark.isEmpty())
                biliLive->sendXliveHeartBeatE();
            else
                biliLive->sendXliveHeartBeatX();
        }
    }
    else if (text == "测试对面信息")
    {
        getPkMatchInfo();
    }
    else if (text == "测试断线")
    {
        liveService->liveSocket->close();
    }
    else if (text == "爆出礼物")
    {
        QByteArray ba = "{\
                        \"cmd\": \"SEND_GIFT\",\
                        \"data\": {\
                            \"anchor_roomid\": \"0\",\
                            \"coin_type\": \"gold\",\
                            \"extra\": {\
                                \"action\": \"投喂\",\
                                \"batch_combo_id\": \"f1100ee9-9068-4122-808d-2f4c7ebd5143\",\
                                \"batch_combo_send\": {\
                                    \"action\": \"投喂\",\
                                    \"batch_combo_id\": \"f1100ee9-9068-4122-808d-2f4c7ebd5143\",\
                                    \"batch_combo_num\": 1,\
                                    \"blind_gift\": {\
                                        \"blind_gift_config_id\": 27,\
                                        \"gift_action\": \"爆出\",\
                                        \"original_gift_id\": 31026,\
                                        \"original_gift_name\": \"白银宝盒\"\
                                    },\
                                    \"gift_id\": 20011,\
                                    \"gift_name\": \"金币\",\
                                    \"gift_num\": 1,\
                                    \"send_master\": null,\
                                    \"uid\": 4303190,\
                                    \"uname\": \"舞月雅白\"\
                                },\
                                \"beatId\": \"\",\
                                \"biz_source\": \"Live\",\
                                \"blind_gift\": {\
                                    \"blind_gift_config_id\": 27,\
                                    \"gift_action\": \"爆出\",\
                                    \"original_gift_id\": 31026,\
                                    \"original_gift_name\": \"白银宝盒\"\
                                },\
                                \"broadcast_id\": 0,\
                                \"coin_type\": \"gold\",\
                                \"combo_resources_id\": 1,\
                                \"combo_send\": {\
                                    \"action\": \"投喂\",\
                                    \"combo_id\": \"4fa537d8-abf7-4e28-8f74-581ea050fde1\",\
                                    \"combo_num\": 1,\
                                    \"gift_id\": 20011,\
                                    \"gift_name\": \"金币\",\
                                    \"gift_num\": 1,\
                                    \"send_master\": null,\
                                    \"uid\": 4303190,\
                                    \"uname\": \"舞月雅白\"\
                                },\
                                \"combo_stay_time\": 3,\
                                \"combo_total_coin\": 600,\
                                \"crit_prob\": 0,\
                                \"demarcation\": 1,\
                                \"discount_price\": 600,\
                                \"dmscore\": 112,\
                                \"draw\": 0,\
                                \"effect\": 0,\
                                \"effect_block\": 0,\
                                \"face\": \"http://i2.hdslb.com/bfs/face/bdd1084ccdd3ecc55969a0ac4dd9be588ad1959a.jpg\",\
                                \"float_sc_resource_id\": 0,\
                                \"giftId\": 20011,\
                                \"giftName\": \"金币\",\
                                \"giftType\": 0,\
                                \"gold\": 0,\
                                \"guard_level\": 0,\
                                \"is_first\": true,\
                                \"is_special_batch\": 0,\
                                \"magnification\": 1,\
                                \"medal_info\": {\
                                    \"anchor_roomid\": 0,\
                                    \"anchor_uname\": \"\",\
                                    \"guard_level\": 0,\
                                    \"icon_id\": 0,\
                                    \"is_lighted\": 0,\
                                    \"medal_color\": 13081892,\
                                    \"medal_color_border\": 12632256,\
                                    \"medal_color_end\": 12632256,\
                                    \"medal_color_start\": 12632256,\
                                    \"medal_level\": 20,\
                                    \"medal_name\": \"东方\",\
                                    \"special\": \"\",\
                                    \"target_id\": 12836417\
                                },\
                                \"name_color\": \"\",\
                                \"num\": 1,\
                                \"original_gift_name\": \"\",\
                                \"price\": 600,\
                                \"rcost\": 282839,\
                                \"remain\": 0,\
                                \"rnd\": \"1097737974\",\
                                \"send_master\": null,\
                                \"silver\": 0,\
                                \"super\": 0,\
                                \"super_batch_gift_num\": 1,\
                                \"super_gift_num\": 1,\
                                \"svga_block\": 0,\
                                \"tag_image\": \"\",\
                                \"tid\": \"1636295428121000002\",\
                                \"timestamp\": 1636295428,\
                                \"top_list\": null,\
                                \"total_coin\": 1000,\
                                \"uid\": 4303190,\
                                \"uname\": \"舞月雅白\"\
                            },\
                            \"gift_id\": 20011,\
                            \"gift_name\": \"金币\",\
                            \"medal_color\": \"c79d24\",\
                            \"medal_level\": 20,\
                            \"medal_name\": \"东方\",\
                            \"medal_up\": \"\",\
                            \"msgType\": 2,\
                            \"nickname\": \"舞月雅白\",\
                            \"number\": 1,\
                            \"timeline\": \"2021-11-07 22:30:33\",\
                            \"total_coin\": 1000,\
                            \"uid\": 4303190\
                        }\
                    }";

        sendTextToSockets("SEND_GIFT", ba);
    }
    else if (text == "投喂银瓜子")
    {
        QByteArray ba = "{\
                        \"cmd\": \"SEND_GIFT\",\
                        \"data\": {\
                            \"anchor_roomid\": \"0\",\
                            \"coin_type\": \"silver\",\
                            \"extra\": {\
                                \"action\": \"投喂\",\
                                \"batch_combo_id\": \"\",\
                                \"batch_combo_send\": null,\
                                \"beatId\": \"\",\
                                \"biz_source\": \"Live\",\
                                \"blind_gift\": null,\
                                \"broadcast_id\": 0,\
                                \"coin_type\": \"silver\",\
                                \"combo_resources_id\": 1,\
                                \"combo_send\": null,\
                                \"combo_stay_time\": 3,\
                                \"combo_total_coin\": 0,\
                                \"crit_prob\": 0,\
                                \"demarcation\": 1,\
                                \"discount_price\": 0,\
                                \"dmscore\": 12,\
                                \"draw\": 0,\
                                \"effect\": 0,\
                                \"effect_block\": 1,\
                                \"face\": \"http://i0.hdslb.com/bfs/face/member/noface.jpg\",\
                                \"float_sc_resource_id\": 0,\
                                \"giftId\": 1,\
                                \"giftName\": \"辣条\",\
                                \"giftType\": 5,\
                                \"gold\": 0,\
                                \"guard_level\": 0,\
                                \"is_first\": true,\
                                \"is_special_batch\": 0,\
                                \"magnification\": 1,\
                                \"medal_info\": {\
                                    \"anchor_roomid\": 0,\
                                    \"anchor_uname\": \"\",\
                                    \"guard_level\": 0,\
                                    \"icon_id\": 0,\
                                    \"is_lighted\": 1,\
                                    \"medal_color\": 6126494,\
                                    \"medal_color_border\": 6126494,\
                                    \"medal_color_end\": 6126494,\
                                    \"medal_color_start\": 6126494,\
                                    \"medal_level\": 7,\
                                    \"medal_name\": \"戒不掉\",\
                                    \"special\": \"\",\
                                    \"target_id\": 300702024\
                                },\
                                \"name_color\": \"\",\
                                \"num\": 1,\
                                \"original_gift_name\": \"\",\
                                \"price\": 100,\
                                \"rcost\": 292387,\
                                \"remain\": 0,\
                                \"rnd\": \"AD3B6A47-5D22-45F3-8340-7C4A20B5DA53\",\
                                \"send_master\": null,\
                                \"silver\": 0,\
                                \"super\": 0,\
                                \"super_batch_gift_num\": 0,\
                                \"super_gift_num\": 0,\
                                \"svga_block\": 0,\
                                \"tag_image\": \"\",\
                                \"tid\": \"1636639739130000002\",\
                                \"timestamp\": 1636639739,\
                                \"top_list\": null,\
                                \"total_coin\": 100,\
                                \"uid\": 702014570,\
                                \"uname\": \"花冢㐅\"\
                            },\
                            \"gift_id\": 1,\
                            \"gift_name\": \"辣条\",\
                            \"medal_color\": \"5d7b9e\",\
                            \"medal_level\": 7,\
                            \"medal_name\": \"戒不掉\",\
                            \"medal_up\": \"\",\
                            \"msgType\": 2,\
                            \"nickname\": \"花冢㐅\",\
                            \"number\": 1,\
                            \"timeline\": \"2021-11-11 22:08:59\",\
                            \"total_coin\": 100,\
                            \"uid\": 702014570\
                        }\
                    }";
        sendTextToSockets("SEND_GIFT", ba);
    }
    else if (text == "投喂电池")
    {
        QByteArray ba = "{\
                        \"cmd\": \"SEND_GIFT\",\
                        \"data\": {\
                            \"anchor_roomid\": \"0\",\
                            \"coin_type\": \"gold\",\
                            \"extra\": {\
                                \"action\": \"投喂\",\
                                \"batch_combo_id\": \"batch:gift:combo_id:37794207:113579884:31039:1636639828.0140\",\
                                \"batch_combo_send\": {\
                                    \"action\": \"投喂\",\
                                    \"batch_combo_id\": \"batch:gift:combo_id:37794207:113579884:31039:1636639828.0140\",\
                                    \"batch_combo_num\": 1,\
                                    \"blind_gift\": null,\
                                    \"gift_id\": 31039,\
                                    \"gift_name\": \"牛哇牛哇\",\
                                    \"gift_num\": 1,\
                                    \"send_master\": null,\
                                    \"uid\": 37794207,\
                                    \"uname\": \"雷神专用柒柒\"\
                                },\
                                \"beatId\": \"\",\
                                \"biz_source\": \"Live\",\
                                \"blind_gift\": null,\
                                \"broadcast_id\": 0,\
                                \"coin_type\": \"gold\",\
                                \"combo_resources_id\": 1,\
                                \"combo_send\": {\
                                    \"action\": \"投喂\",\
                                    \"combo_id\": \"gift:combo_id:37794207:113579884:31039:1636639828.0131\",\
                                    \"combo_num\": 1,\
                                    \"gift_id\": 31039,\
                                    \"gift_name\": \"牛哇牛哇\",\
                                    \"gift_num\": 1,\
                                    \"send_master\": null,\
                                    \"uid\": 37794207,\
                                    \"uname\": \"雷神专用柒柒\"\
                                },\
                                \"combo_stay_time\": 3,\
                                \"combo_total_coin\": 100,\
                                \"crit_prob\": 0,\
                                \"demarcation\": 1,\
                                \"discount_price\": 100,\
                                \"dmscore\": 120,\
                                \"draw\": 0,\
                                \"effect\": 0,\
                                \"effect_block\": 0,\
                                \"face\": \"http://i0.hdslb.com/bfs/face/eb101ef90ebc4e9bf79f65312a22ebac84946700.jpg\",\
                                \"float_sc_resource_id\": 0,\
                                \"giftId\": 31039,\
                                \"giftName\": \"牛哇牛哇\",\
                                \"giftType\": 0,\
                                \"gold\": 0,\
                                \"guard_level\": 3,\
                                \"is_first\": true,\
                                \"is_special_batch\": 0,\
                                \"magnification\": 1,\
                                \"medal_info\": {\
                                    \"anchor_roomid\": 0,\
                                    \"anchor_uname\": \"\",\
                                    \"guard_level\": 3,\
                                    \"icon_id\": 0,\
                                    \"is_lighted\": 1,\
                                    \"medal_color\": 1725515,\
                                    \"medal_color_border\": 6809855,\
                                    \"medal_color_end\": 5414290,\
                                    \"medal_color_start\": 1725515,\
                                    \"medal_level\": 21,\
                                    \"medal_name\": \"女仆厨\",\
                                    \"special\": \"\",\
                                    \"target_id\": 113579884\
                                },\
                                \"name_color\": \"#00D1F1\",\
                                \"num\": 1,\
                                \"original_gift_name\": \"\",\
                                \"price\": 100,\
                                \"rcost\": 292391,\
                                \"remain\": 0,\
                                \"rnd\": \"180269212\",\
                                \"send_master\": null,\
                                \"silver\": 0,\
                                \"super\": 0,\
                                \"super_batch_gift_num\": 1,\
                                \"super_gift_num\": 1,\
                                \"svga_block\": 0,\
                                \"tag_image\": \"\",\
                                \"tid\": \"1636639827121700001\",\
                                \"timestamp\": 1636639827,\
                                \"top_list\": null,\
                                \"total_coin\": 100,\
                                \"uid\": 37794207,\
                                \"uname\": \"雷神专用柒柒\"\
                            },\
                            \"gift_id\": 31039,\
                            \"gift_name\": \"牛哇牛哇\",\
                            \"medal_color\": \"1a544b\",\
                            \"medal_level\": 21,\
                            \"medal_name\": \"女仆厨\",\
                            \"medal_up\": \"\",\
                            \"msgType\": 2,\
                            \"nickname\": \"雷神专用柒柒\",\
                            \"number\": 1,\
                            \"timeline\": \"2021-11-11 22:10:29\",\
                            \"total_coin\": 100,\
                            \"uid\": 37794207\
                        }\
                    }";
        sendTextToSockets("SEND_GIFT", ba);
    }
    else if (text == "礼物连击")
    {
        QByteArray ba = "{\
                        \"cmd\": \"COMBO_SEND\",\
                        \"data\": {\
                            \"extra\": {\
                                \"action\": \"投喂\",\
                                \"batch_combo_id\": \"batch:gift:combo_id:14729370:113579884:30869:1637386968.9309\",\
                                \"batch_combo_num\": 2,\
                                \"combo_id\": \"gift:combo_id:14729370:113579884:30869:1637386968.9301\",\
                                \"combo_num\": 200,\
                                \"combo_total_coin\": 20000,\
                                \"dmscore\": 112,\
                                \"gift_id\": 30869,\
                                \"gift_name\": \"心动卡\",\
                                \"gift_num\": 100,\
                                \"is_show\": 1,\
                                \"medal_info\": {\
                                    \"anchor_roomid\": 0,\
                                    \"anchor_uname\": \"\",\
                                    \"guard_level\": 0,\
                                    \"icon_id\": 0,\
                                    \"is_lighted\": 1,\
                                    \"medal_color\": 6067854,\
                                    \"medal_color_border\": 6067854,\
                                    \"medal_color_end\": 6067854,\
                                    \"medal_color_start\": 6067854,\
                                    \"medal_level\": 4,\
                                    \"medal_name\": \"绒冰球\",\
                                    \"special\": \"\",\
                                    \"target_id\": 21374533\
                                },\
                                \"name_color\": \"\",\
                                \"r_uname\": \"水良子\",\
                                \"ruid\": 113579884,\
                                \"send_master\": null,\
                                \"total_num\": 200,\
                                \"uid\": 14729370,\
                                \"uname\": \"半夏惜沫\"\
                            },\
                            \"msgType\": 0,\
                            \"nickname\": \"\",\
                            \"timeline\": \"\",\
                            \"uid\": 0\
                        }\
                    }";
        sendTextToSockets("COMBO_SEND", ba);
    }
    else if (text == "开启互动玩法")
    {
        liveOpenService->start();
    }
    else if (text == "关闭互动玩法")
    {
        liveOpenService->end();
    }
    else if (text == "昵称简化测试")
    {
        auto query = sqlService.getQuery("select distinct uname from interact where msg_type = 1");
        QStringList sl;
        while (query.next())
        {
            QString uname = query.value(0).toString();
            QString ainame = cr->nicknameSimplify(LiveDanmaku(uname, 0));
            sl.append(uname + "," +ainame);
        }
        writeTextFile("昵称简化测试.csv", sl.join("\n"));
    }
    else if (text == "测试每月")
    {
        triggerCmdEvent("NEW_DAY", LiveDanmaku());
        triggerCmdEvent("NEW_DAY_FIRST", LiveDanmaku());
        triggerCmdEvent("NEW_MONTH", LiveDanmaku());
        triggerCmdEvent("NEW_MONTH_FIRST", LiveDanmaku());
    }
    else if (text == "测试崩溃")
    {
        int* p = nullptr;
        *p = 1;
    }
    else if (text == "测试批量点歌")
    {
        if (musicWindow)
        {
            musicWindow->slotSearchAndAutoAppend("1", "测试用户");
            musicWindow->slotSearchAndAutoAppend("2", "测试用户");
            musicWindow->slotSearchAndAutoAppend("3", "测试用户");
            musicWindow->slotSearchAndAutoAppend("4", "测试用户");
            musicWindow->slotSearchAndAutoAppend("5", "测试用户");
        }
    }
    else
    {
        appendNewLiveDanmaku(LiveDanmaku("测试用户" + QString::number(r), text,
                             uid, 12,
                             QDateTime::currentDateTime(), "", ""));
    }
}

void MainWindow::on_removeDanmakuIntervalSpin_valueChanged(int arg1)
{
    this->removeDanmakuInterval = arg1 * 1000;
    us->setValue("danmaku/removeInterval", arg1);
}

void MainWindow::on_roomIdEdit_editingFinished()
{
    if (ac->roomId == ui->roomIdEdit->text() || ac->shortId == ui->roomIdEdit->text())
        return ;

    // 关闭旧的
    if (liveService->liveSocket)
    {
        ac->liveStatus = 0;
        if (liveService->liveSocket->state() != QAbstractSocket::UnconnectedState)
            liveService->liveSocket->abort();
    }
    ac->roomId = ui->roomIdEdit->text();
    ac->upUid = "";
    us->setValue("danmaku/roomId", ac->roomId);

    releaseLiveData();

    // 判断是身份码还是房间号
    bool isPureRoomId = (ac->roomId.contains(QRegularExpression("^\\d+$")));

    // 先解析身份码
    if (!isPureRoomId)
    {
        ac->identityCode = ac->roomId;
        ui->identityCodeEdit->setText(ac->identityCode);
        us->set("live-open/identityCode", ac->identityCode);
        startConnectIdentityCode();
        return ;
    }

    // 直接继续
    emit signalRoomChanged(ac->roomId);

    // 开启新的
    if (liveService->liveSocket)
    {
        startConnectRoom();
    }
    pullLiveDanmaku();
}

void MainWindow::on_languageAutoTranslateCheck_stateChanged(int)
{
    auto trans = ui->languageAutoTranslateCheck->isChecked();
    us->setValue("danmaku/autoTrans", trans);
    if (danmakuWindow)
        danmakuWindow->setAutoTranslate(trans);
}

void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    us->setValue("mainwindow/tabIndex", index);
    if (index > 2)
        appendListItemButton->hide();
    else
        appendListItemButton->show();
    QTimer::singleShot(0, [=]{
        adjustPageSize(ui->stackedWidget->currentIndex());
    });
}

void MainWindow::on_SendMsgButton_clicked()
{
//    QString msg = ui->SendMsgEdit->text();
//    msg = processDanmakuVariants(msg, LiveDanmaku());
//    sendAutoMsg(msg);
}

void MainWindow::on_AIReplyCheck_stateChanged(int)
{
    bool reply = ui->AIReplyCheck->isChecked();
    us->setValue("danmaku/aiReply", reply);
    if (danmakuWindow)
        danmakuWindow->setAIReply(reply);

    ui->AIReplyMsgCheck->setEnabled(reply);
}

void MainWindow::on_testDanmakuEdit_returnPressed()
{
    on_testDanmakuButton_clicked();

    ui->testDanmakuEdit->setText("");
}

void MainWindow::on_SendMsgEdit_returnPressed()
{
    QString msg = ui->SendMsgEdit->text();
    msg = cr->processDanmakuVariants(msg, LiveDanmaku());
    cr->sendAutoMsg(msg, LiveDanmaku());
    ui->SendMsgEdit->clear();
}

TaskWidget* MainWindow::addTimerTask(bool enable, int second, QString text, int index)
{
    TaskWidget* tw = new TaskWidget(this);
    QListWidgetItem* item;

    if (index == -1)
    {
        item = new QListWidgetItem(ui->taskListWidget);
        ui->taskListWidget->addItem(item);
        ui->taskListWidget->setItemWidget(item, tw);
        // ui->taskListWidget->setCurrentRow(ui->taskListWidget->count()-1);
    }
    else
    {
        item = new QListWidgetItem();
        ui->taskListWidget->insertItem(index, item);
        ui->taskListWidget->setItemWidget(item, tw);
        ui->taskListWidget->setCurrentRow(index);
    }

    // 连接信号
    connectTimerTaskEvent(tw, item);

    us->remoteControl = us->value("danmaku/remoteControl", us->remoteControl).toBool();

    // 设置属性
    tw->check->setChecked(enable);
    tw->spin->setValue(second);
    tw->slotSpinChanged(second);
    tw->edit->setPlainText(text);
    tw->autoResizeEdit();
    tw->adjustSize();

    return tw;
}

TaskWidget *MainWindow::addTimerTask(const MyJson &json)
{
    auto item = addTimerTask(false, 1800, "");
    item->fromJson(json);
    return item;
}

void MainWindow::connectTimerTaskEvent(TaskWidget *tw, QListWidgetItem *item)
{
    connect(tw->check, &QCheckBox::stateChanged, this, [=](int){
        bool enable = tw->check->isChecked();
        int row = ui->taskListWidget->row(item);
        us->setValue("task/r"+QString::number(row)+"Enable", enable);
    });

    connect(tw, &TaskWidget::spinChanged, this, [=](int val){
        int row = ui->taskListWidget->row(item);
        us->setValue("task/r"+QString::number(row)+"Interval", val);
    });

    connect(tw->edit, &ConditionEditor::textChanged, this, [=]{
        item->setSizeHint(tw->sizeHint());

        QString content = tw->edit->toPlainText();
        int row = ui->taskListWidget->row(item);
        us->setValue("task/r"+QString::number(row)+"Msg", content);
    });

    connect(tw, &TaskWidget::signalSendMsgs, this, [=](QString sl, bool manual){
        if (us->debugPrint)
            localNotify("[定时任务:" + snum(ui->taskListWidget->row(item)) + "]");
        if (!manual && !cr->shallAutoMsg(sl, manual)) // 没有开播，不进行定时任务
        {
            qInfo() << "未开播，不做回复(timer)" << sl;
            if (us->debugPrint)
                localNotify("[未开播，不做回复]");
            return ;
        }

        if (cr->sendVariantMsg(sl, LiveDanmaku(), TASK_CD_CN, manual))
        {
        }
        else if (us->debugPrint)
        {
            localNotify("[没有可发送的定时弹幕]");
        }
    });

    connect(tw, &TaskWidget::signalResized, tw, [=]{
        item->setSizeHint(tw->size());
    });

    connect(tw, &TaskWidget::signalInsertCodeSnippets, this, [=](const QJsonDocument& doc) {
        if (tw->isEmpty())
        {
            int row = ui->taskListWidget->row(item);
            ui->taskListWidget->removeItemWidget(item);
            ui->taskListWidget->takeItem(row);
            tw->deleteLater();
            saveTaskList();
        }
        addCodeSnippets(doc);
    });
}

void MainWindow::saveTaskList()
{
    us->setValue("task/count", ui->taskListWidget->count());
    for (int row = 0; row < ui->taskListWidget->count(); row++)
    {
        auto widget = ui->taskListWidget->itemWidget(ui->taskListWidget->item(row));
        auto tw = static_cast<TaskWidget*>(widget);
        us->setValue("task/r"+QString::number(row)+"Enable", tw->check->isChecked());
        us->setValue("task/r"+QString::number(row)+"Interval", tw->spin->value());
        us->setValue("task/r"+QString::number(row)+"Msg", tw->edit->toPlainText());
    }
}

void MainWindow::restoreTaskList()
{
    int count = us->value("task/count", 0).toInt();
    for (int row = 0; row < count; row++)
    {
        bool enable = us->value("task/r"+QString::number(row)+"Enable", false).toBool();
        int interval = us->value("task/r"+QString::number(row)+"Interval", 1800).toInt();
        QString msg = us->value("task/r"+QString::number(row)+"Msg", "").toString();
        addTimerTask(enable, interval, msg);
    }
}

ReplyWidget* MainWindow::addAutoReply(bool enable, QString key, QString reply, int index)
{
    ReplyWidget* rw = new ReplyWidget(this);
    QListWidgetItem* item;

    if (index == -1)
    {
        item = new QListWidgetItem(ui->replyListWidget);
        ui->replyListWidget->addItem(item);
        ui->replyListWidget->setItemWidget(item, rw);
        // ui->replyListWidget->setCurrentRow(ui->replyListWidget->count()-1);
    }
    else
    {
        item = new QListWidgetItem();
        ui->replyListWidget->insertItem(index, item);
        ui->replyListWidget->setItemWidget(item, rw);
        ui->replyListWidget->setCurrentRow(index);
    }

    // 连接信号
    connectAutoReplyEvent(rw, item);

    us->remoteControl = us->value("danmaku/remoteControl", us->remoteControl).toBool();

    // 设置属性
    rw->check->setChecked(enable);
    rw->keyEdit->setText(key);
    rw->replyEdit->setPlainText(reply);
    rw->autoResizeEdit();
    rw->adjustSize();
    item->setSizeHint(rw->sizeHint());

    return rw;
}

ReplyWidget *MainWindow::addAutoReply(const MyJson &json)
{
    auto item = addAutoReply(false, "", "");
    item->fromJson(json);
    return item;
}

void MainWindow::connectAutoReplyEvent(ReplyWidget *rw, QListWidgetItem *item)
{
    connect(rw->check, &QCheckBox::stateChanged, this, [=](int){
        bool enable = rw->check->isChecked();
        int row = ui->replyListWidget->row(item);
        us->setValue("reply/r"+QString::number(row)+"Enable", enable);
    });

    connect(rw->keyEdit, &QLineEdit::textChanged, this, [=](const QString& text){
        int row = ui->replyListWidget->row(item);
        us->setValue("reply/r"+QString::number(row)+"Key", text);
    });

    connect(rw->replyEdit, &ConditionEditor::textChanged, this, [=]{
        item->setSizeHint(rw->sizeHint());

        QString content = rw->replyEdit->toPlainText();
        int row = ui->replyListWidget->row(item);
        us->setValue("reply/r"+QString::number(row)+"Reply", content);
    });

    connect(this, SIGNAL(signalNewDanmaku(const LiveDanmaku&)), rw, SLOT(slotNewDanmaku(const LiveDanmaku&)));

    connect(rw, &ReplyWidget::signalReplyMsgs, this, [=](QString sl, LiveDanmaku danmaku, bool manual){
#ifndef ZUOQI_ENTRANCE
        if (!hasPermission())
            return ;
#endif
        if (cr->isFilterRejected("FILTER_AUTO_REPLY", danmaku))
            return ;
        if ((!manual && !cr->shallAutoMsg(sl, manual)) || danmaku.isPkLink()) // 没有开播，不进行自动回复
        {
            if (!danmaku.isPkLink())
                qInfo() << "未开播，不做回复(reply)" << sl;
            if (us->debugPrint)
                localNotify("[未开播，不做回复]");
            return ;
        }

        if (cr->sendVariantMsg(sl, danmaku, REPLY_CD_CN, manual, true))
        {
        }
        else if (us->debugPrint)
        {
            localNotify("[没有可发送的回复弹幕]");
        }
    });

    connect(rw, &ReplyWidget::signalResized, rw, [=]{
        item->setSizeHint(rw->size());
    });

    connect(rw, &ReplyWidget::signalInsertCodeSnippets, this, [=](const QJsonDocument& doc) {
        if (rw->isEmpty())
        {
            int row = ui->replyListWidget->row(item);
            ui->replyListWidget->removeItemWidget(item);
            ui->replyListWidget->takeItem(row);
            rw->deleteLater();
            saveReplyList();
        }
        addCodeSnippets(doc);
    });
}

void MainWindow::saveReplyList()
{
    us->setValue("reply/count", ui->replyListWidget->count());
    for (int row = 0; row < ui->replyListWidget->count(); row++)
    {
        auto widget = ui->replyListWidget->itemWidget(ui->replyListWidget->item(row));
        auto tw = static_cast<ReplyWidget*>(widget);
        us->setValue("reply/r"+QString::number(row)+"Enable", tw->check->isChecked());
        us->setValue("reply/r"+QString::number(row)+"Key", tw->keyEdit->text());
        us->setValue("reply/r"+QString::number(row)+"Reply", tw->replyEdit->toPlainText());
    }
}

void MainWindow::restoreReplyList()
{
    int count = us->value("reply/count", 0).toInt();
    for (int row = 0; row < count; row++)
    {
        bool enable = us->value("reply/r"+QString::number(row)+"Enable", false).toBool();
        QString key = us->value("reply/r"+QString::number(row)+"Key").toString();
        QString reply = us->value("reply/r"+QString::number(row)+"Reply").toString();
        addAutoReply(enable, key, reply);
    }
}

void MainWindow::addListItemOnCurrentPage()
{
    auto w = ui->tabWidget->currentWidget();
    if (w == ui->tabTimer)
    {
        addTimerTask(false, 1800, "");
        saveTaskList();
        auto widget = ui->taskListWidget->itemWidget(ui->taskListWidget->item(ui->taskListWidget->count()-1));
        auto tw = static_cast<TaskWidget*>(widget);
        QTimer::singleShot(0, [=]{
            ui->taskListWidget->scrollToBottom();
        });
        tw->edit->setFocus();
    }
    else if (w == ui->tabReply)
    {
        addAutoReply(false, "", "");
        saveReplyList();
        auto widget = ui->replyListWidget->itemWidget(ui->replyListWidget->item(ui->replyListWidget->count()-1));
        auto rw = static_cast<ReplyWidget*>(widget);
        QTimer::singleShot(0, [=]{
            ui->replyListWidget->scrollToBottom();
        });
        rw->keyEdit->setFocus();
    }
    else if (w == ui->tabEvent)
    {
        addEventAction(false, "", "");
        saveEventList();
        auto widget = ui->eventListWidget->itemWidget(ui->eventListWidget->item(ui->eventListWidget->count()-1));
        auto ew = static_cast<EventWidget*>(widget);
        QTimer::singleShot(0, [=]{
            ui->eventListWidget->scrollToBottom();
        });
        ew->eventEdit->setFocus();
    }
}

EventWidget* MainWindow::addEventAction(bool enable, QString cmd, QString action, int index)
{
    EventWidget* rw = new EventWidget(this);
    QListWidgetItem* item;

    if (index == -1)
    {
        item = new QListWidgetItem(ui->eventListWidget);
        ui->eventListWidget->addItem(item);
        ui->eventListWidget->setItemWidget(item, rw);
        // ui->eventListWidget->setCurrentRow(ui->eventListWidget->count()-1);
    }
    else
    {
        item = new QListWidgetItem();
        ui->eventListWidget->insertItem(index, item);
        ui->eventListWidget->setItemWidget(item, rw);
        ui->eventListWidget->setCurrentRow(index);
    }

    // 连接信号
    connectEventActionEvent(rw, item);

    us->remoteControl = us->value("danmaku/remoteControl", us->remoteControl).toBool();

    // 设置属性
    rw->check->setChecked(enable);
    rw->eventEdit->setText(cmd);
    rw->actionEdit->setPlainText(action);
    rw->autoResizeEdit();
    rw->adjustSize();
    item->setSizeHint(rw->sizeHint());

    return rw;
}

EventWidget* MainWindow::addEventAction(const MyJson &json)
{
    auto item = addEventAction(false, "", "");
    item->fromJson(json);
    return item;
}

void MainWindow::connectEventActionEvent(EventWidget *rw, QListWidgetItem *item)
{
    connect(rw->check, &QCheckBox::stateChanged, this, [=](int){
        bool enable = rw->check->isChecked();
        int row = ui->eventListWidget->row(item);
        us->setValue("event/r"+QString::number(row)+"Enable", enable);
    });

    connect(rw->eventEdit, &QLineEdit::textChanged, this, [=](const QString& text){
        int row = ui->eventListWidget->row(item);
        us->setValue("event/r"+QString::number(row)+"Cmd", text);
    });

    connect(rw->actionEdit, &ConditionEditor::textChanged, this, [=]{
        item->setSizeHint(rw->sizeHint());

        QString content = rw->actionEdit->toPlainText();
        int row = ui->eventListWidget->row(item);
        us->setValue("event/r"+QString::number(row)+"Action", content);

        /* // 处理特殊操作，比如过滤器
        QString event = rw->title();
        if (!event.isEmpty())
        {
            setFilter(event, content);
        } */
    });

    connect(this, SIGNAL(signalCmdEvent(QString, LiveDanmaku)), rw, SLOT(triggerCmdEvent(QString,LiveDanmaku)));

    connect(rw, &EventWidget::signalEventMsgs, this, [=](QString sl, LiveDanmaku danmaku, bool manual){
#ifndef ZUOQI_ENTRANCE
        if (!hasPermission())
            return ;
#endif
        if (!manual && !cr->shallAutoMsg(sl, manual)) // 没有开播，不进行自动回复
        {
            qInfo() << "未开播，不做操作(event)" << sl;
            if (us->debugPrint)
                localNotify("[未开播，不做操作]");
            return ;
        }
        if (!liveService->isLiving() && !manual)
            manual = true;

        if (cr->sendVariantMsg(sl, danmaku, EVENT_CD_CN, manual))
        {
        }
        else if (us->debugPrint)
        {
            localNotify("[没有可发送的事件弹幕]");
        }
    });

    connect(rw, &EventWidget::signalResized, rw, [=]{
        item->setSizeHint(rw->size());
    });

    connect(rw, &EventWidget::signalInsertCodeSnippets, this, [=](const QJsonDocument& doc) {
        if (rw->isEmpty())
        {
            int row = ui->eventListWidget->row(item);
            ui->eventListWidget->removeItemWidget(item);
            ui->eventListWidget->takeItem(row);
            rw->deleteLater();
            saveEventList();
        }
        addCodeSnippets(doc);
    });
}

void MainWindow::saveEventList()
{
    us->setValue("event/count", ui->eventListWidget->count());
    for (int row = 0; row < ui->eventListWidget->count(); row++)
    {
        auto widget = ui->eventListWidget->itemWidget(ui->eventListWidget->item(row));
        auto tw = static_cast<EventWidget*>(widget);
        us->setValue("event/r"+QString::number(row)+"Enable", tw->check->isChecked());
        us->setValue("event/r"+QString::number(row)+"Cmd", tw->eventEdit->text());
        us->setValue("event/r"+QString::number(row)+"Action", tw->actionEdit->toPlainText());
    }
}

void MainWindow::restoreEventList()
{
    int count = us->value("event/count", 0).toInt();
    for (int row = 0; row < count; row++)
    {
        bool enable = us->value("event/r"+QString::number(row)+"Enable", false).toBool();
        QString key = us->value("event/r"+QString::number(row)+"Cmd").toString();
        QString event = us->value("event/r"+QString::number(row)+"Action").toString();
        addEventAction(enable, key, event);
    }
}

bool MainWindow::hasEvent(const QString &cmd) const
{
    for (int row = 0; row < ui->eventListWidget->count(); row++)
    {
        auto widget = ui->eventListWidget->itemWidget(ui->eventListWidget->item(row));
        auto tw = static_cast<EventWidget*>(widget);
        if (tw->eventEdit->text() == cmd && tw->check->isChecked())
            return true;
    }
    return false;
}

void MainWindow::addCodeSnippets(const QJsonDocument &doc)
{
    bool scrolled[3] = {};

    auto pasteFromJson = [&](QJsonObject json) {
        QString anchor_key = json.value("anchor_key").toString();
        ListItemInterface* item = nullptr;
        if (anchor_key == CODE_TIMER_TASK_KEY)
        {
            item = addTimerTask(false, 1800, "");
            ui->tabWidget->setCurrentWidget(ui->tabTimer);
            if (!scrolled[0])
            {
                scrolled[0] = true;
                QTimer::singleShot(0, [=]{
                    ui->taskListWidget->scrollToBottom();
                });
            }
            us->setValue("task/count", ui->taskListWidget->count());
        }
        else if (anchor_key == CODE_AUTO_REPLY_KEY)
        {
            item = addAutoReply(false, "","");
            ui->tabWidget->setCurrentWidget(ui->tabReply);
            if (!scrolled[1])
            {
                scrolled[1] = true;
                QTimer::singleShot(0, [=]{
                    ui->replyListWidget->scrollToBottom();
                });
            }
            us->setValue("reply/count", ui->replyListWidget->count());
        }
        else if (anchor_key == CODE_EVENT_ACTION_KEY)
        {
            item = addEventAction(false, "", "");
            ui->tabWidget->setCurrentWidget(ui->tabEvent);
            if (!scrolled[2])
            {
                scrolled[2] = true;
                QTimer::singleShot(0, [=]{
                    ui->eventListWidget->scrollToBottom();
                });
            }
            us->setValue("event/count", ui->eventListWidget->count());
        }
        else
        {
            qWarning() << "未知格式：" << json;
            return ;
        }

        item->fromJson(json);
        item->autoResizeEdit();

        // 检查重复
        if (anchor_key == CODE_AUTO_REPLY_KEY)
        {
            for (int row = 0; row < ui->replyListWidget->count() - 1; row++)
            {
                auto rowItem = ui->replyListWidget->item(row);
                auto widget = ui->replyListWidget->itemWidget(rowItem);
                if (!widget)
                    continue;
                QSize size(ui->replyListWidget->contentsRect().width() - ui->replyListWidget->verticalScrollBar()->width(), widget->height());
                auto replyWidget = static_cast<ReplyWidget*>(widget);

                auto rw = static_cast<ReplyWidget*>(item);
                if (rw->keyEdit->text() == replyWidget->keyEdit->text()
                        && rw->replyEdit->toPlainText() == replyWidget->replyEdit->toPlainText())
                {
                    rw->check->setChecked(false);
                    rw->keyEdit->setText(rw->keyEdit->text() + "_重复");
                    break;
                }
            }
        }
        else if (anchor_key == CODE_EVENT_ACTION_KEY)
        {
            for (int row = 0; row < ui->eventListWidget->count() - 1; row++)
            {
                auto rowItem = ui->eventListWidget->item(row);
                auto widget = ui->eventListWidget->itemWidget(rowItem);
                if (!widget)
                    continue;
                QSize size(ui->eventListWidget->contentsRect().width() - ui->eventListWidget->verticalScrollBar()->width(), widget->height());
                auto eventWidget = static_cast<EventWidget*>(widget);

                auto rw = static_cast<EventWidget*>(item);
                if (rw->eventEdit->text() == eventWidget->eventEdit->text()
                        && rw->actionEdit->toPlainText() == eventWidget->actionEdit->toPlainText())
                {
                    rw->check->setChecked(false);
                    rw->eventEdit->setText(rw->eventEdit->text() + "_重复");
                    break;
                }
            }
        }
    };

    if (doc.isObject())
    {
        pasteFromJson(doc.object());
    }
    else if (doc.isArray())
    {
        QJsonArray array = doc.array();
        foreach (QJsonValue val, array)
        {
            pasteFromJson(val.toObject());
        }
    }
    adjustPageSize(PAGE_EXTENSION);
}

void MainWindow::autoSetCookie(const QString &s)
{
    us->setValue("danmaku/browserCookie", ac->browserCookie = s);
    if (ac->browserCookie.isEmpty())
        return ;

    ac->userCookies = getCookies();
    getCookieAccount();

    // 自动设置弹幕格式
    int posl = ac->browserCookie.indexOf("bili_jct=") + 9;
    if (posl == -1)
        return ;
    int posr = ac->browserCookie.indexOf(";", posl);
    if (posr == -1) posr = ac->browserCookie.length();
    ac->csrf_token = ac->browserCookie.mid(posl, posr - posl);
    qInfo() << "检测到csrf_token:" << ac->csrf_token;

    if (ac->browserData.isEmpty())
        ac->browserData = "color=4546550&fontsize=25&mode=4&msg=&rnd=1605156247&roomid=&bubble=5&csrf_token=&csrf=";
    ac->browserData.replace(QRegularExpression("csrf_token=[^&]*"), "csrf_token=" + ac->csrf_token);
    ac->browserData.replace(QRegularExpression("csrf=[^&]*"), "csrf=" + ac->csrf_token);
    us->setValue("danmaku/browserData", ac->browserData);
    qInfo() << "设置弹幕格式：" << ac->browserData;
}

QVariant MainWindow::getCookies() const
{
    QList<QNetworkCookie> cookies;

    // 设置cookie
    QString cookieText = ac->browserCookie;
    QStringList sl = cookieText.split(";");
    foreach (auto s, sl)
    {
        s = s.trimmed();
        int pos = s.indexOf("=");
        QString key = s.left(pos);
        QString val = s.right(s.length() - pos - 1);
        cookies.push_back(QNetworkCookie(key.toUtf8(), val.toUtf8()));
    }

    // 请求头里面加入cookies
    QVariant var;
    var.setValue(cookies);
    return var;
}

/**
 * 获取用户信息
 */
void MainWindow::getCookieAccount()
{
    if (ac->browserCookie.isEmpty())
        return ;
    liveService->gettingUser = true;
    get("https://api.bilibili.com/x/member/web/account", [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            showError("登录返回不为0", json.value("message").toString());
            liveService->gettingUser = false;
            if (!liveService->gettingRoom)
                triggerCmdEvent("LOGIN_FINISHED", LiveDanmaku());
            updatePermission();
            return ;
        }

        // 获取用户信息
        QJsonObject dataObj = json.value("data").toObject();
        ac->cookieUid = snum(static_cast<qint64>(dataObj.value("mid").toDouble()));
        ac->cookieUname = dataObj.value("uname").toString();
        qInfo() << "当前账号：" << ac->cookieUid << ac->cookieUname;
        ui->robotNameButton->setText(ac->cookieUname);
        ui->robotNameButton->adjustMinimumSize();
        ui->robotInfoWidget->setMinimumWidth(ui->robotNameButton->width());
#ifdef ZUOQI_ENTRANCE
        fakeEntrance->setRobotName(ac->cookieUname);
#endif

        getRobotInfo();
        liveService->gettingUser = false;
        if (!liveService->gettingRoom)
            triggerCmdEvent("LOGIN_FINISHED", LiveDanmaku());
        updatePermission();
        getPositiveVote();
    });
}

QString MainWindow::getDomainPort() const
{
    QString domain = ui->domainEdit->text().trimmed();
    if (domain.isEmpty())
        domain = "http://localhost";
    if (!domain.contains("://"))
        domain = "http://" + domain;
    QString url = domain + ":" + snum(ui->serverPortSpin->value());
    return url;
}

void MainWindow::getRobotInfo()
{
    QString url = "https://api.bilibili.com/x/space/acc/info?mid=" + ac->cookieUid;
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取账号信息返回结果不为0：") << json.value("message").toString();
            return ;
        }
        QJsonObject data = json.value("data").toObject();

        // 开始下载头像
        QString face = data.value("face").toString();
        get(face, [=](QNetworkReply* reply){
            QByteArray jpegData = reply->readAll();
            QPixmap pixmap;
            pixmap.loadFromData(jpegData);
            if (pixmap.isNull())
            {
                showError("获取账号头像出错");
                return ;
            }

            // 设置成圆角
            int side = qMin(pixmap.width(), pixmap.height());
            QPixmap p(side, side);
            p.fill(Qt::transparent);
            QPainter painter(&p);
            painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
            QPainterPath path;
            path.addEllipse(0, 0, side, side);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, side, side, pixmap);

            // 设置到Robot头像
            QPixmap f = p.scaled(ui->robotHeaderLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ui->robotHeaderLabel->setPixmap(f);
        });
    });

    // 检查是否需要用户信息
    if (!ui->adjustDanmakuLongestCheck->isChecked())
        return ;

    // 获取弹幕字数
    url = "https://api.vc.bilibili.com/user_ex/v1/user/detail?uid=" + ac->cookieUid + "&user[]=role&user[]=level&room[]=live_status&room[]=room_link&feed[]=fans_count&feed[]=feed_count&feed[]=is_followed&feed[]=is_following&platform=pc";
    get(url, [=](MyJson json) {
        /*{
            "code": 0,
            "msg": "success",
            "message": "success",
            "data": {
                "user": {
                    "role": 2,
                    "user_level": 19,
                    "master_level": 5,
                    "next_master_level": 6,
                    "need_master_score": 704,
                    "master_rank": 100010,
                    "verify": 0
                },
                "feed": {
                    "fans_count": 7084,
                    "feed_count": 41,
                    "is_followed": 0,
                    "is_following": 0
                },
                "room": {
                    "live_status": 0,
                    "room_id": 11584296,
                    "short_room_id": 0,
                    "title": "用户20285041的直播间",
                    "cover": "",
                    "keyframe": "",
                    "online": 0,
                    "room_link": "http://live.bilibili.com/11584296?src=draw"
                },
                "uid": "20285041"
            }
        }*/
        MyJson data = json.data();
        ac->cookieULevel = data.o("user").i("user_level");
        if (ui->adjustDanmakuLongestCheck->isChecked())
            liveService->adjustDanmakuLongest();
    });
}

/**
 * 获取用户在当前直播间的信息
 * 会触发进入直播间，就不能偷看了……
 */
void MainWindow::getRoomUserInfo()
{
    /*{
        "code": 0,
        "message": "0",
        "ttl": 1,
        "data": {
            "user_level": {
                "level": 13,
                "next_level": 14,
                "color": 6406234,
                "level_rank": ">50000"
            },
            "vip": {
                "vip": 0,
                "vip_time": "0000-00-00 00:00:00",
                "svip": 0,
                "svip_time": "0000-00-00 00:00:00"
            },
            "title": {
                "title": ""
            },
            "badge": {
                "is_room_admin": false
            },
            "privilege": {
                "target_id": 0,
                "privilege_type": 0,
                "privilege_uname_color": "",
                "buy_guard_notice": null,
                "sub_level": 0,
                "notice_status": 1,
                "expired_time": "",
                "auto_renew": 0,
                "renew_remind": null
            },
            "info": {
                "uid": 20285041,
                "uname": "懒一夕智能科技",
                "uface": "http://i1.hdslb.com/bfs/face/97ae8f0f0e09fbc22fa680c4f5ed93f92678c9eb.jpg",
                "main_rank": 10000,
                "bili_vip": 2,
                "mobile_verify": 1,
                "identification": 1
            },
            "property": {
                "uname_color": "",
                "bubble": 0,
                "danmu": {
                    "mode": 1,
                    "color": 16777215,
                    "length": 20,
                    "room_id": 13328782
                },
                "bubble_color": ""
            },
            "recharge": {
                "status": 0,
                "type": 1,
                "value": "",
                "color": "fb7299",
                "config_id": 0
            },
            "relation": {
                "is_followed": false,
                "is_fans": false,
                "is_in_fansclub": false
            },
            "wallet": {
                "gold": 6100,
                "silver": 3294
            },
            "medal": {
                "cnt": 16,
                "is_weared": false,
                "curr_weared": null,
                "up_medal": {
                    "uid": 358629230,
                    "medal_name": "石乐志",
                    "medal_color": 6067854,
                    "level": 1
                }
            },
            "extra_config": {
                "show_bag": false,
                "show_vip_broadcast": true
            },
            "mailbox": {
                "switch_status": 0,
                "red_notice": 0
            },
            "user_reward": {
                "entry_effect": {
                    "id": 0,
                    "privilege_type": 0,
                    "priority": 0,
                    "web_basemap_url": "",
                    "web_effective_time": 0,
                    "web_effect_close": 0,
                    "web_close_time": 0,
                    "copy_writing": "",
                    "copy_color": "",
                    "highlight_color": "",
                    "mock_effect": 0,
                    "business": 0,
                    "face": "",
                    "basemap_url": "",
                    "show_avatar": 0,
                    "effective_time": 0
                },
                "welcome": {
                    "allow_mock": 1
                }
            },
            "shield_info": {
                "shield_user_list": [],
                "keyword_list": [],
                "shield_rules": {
                    "rank": 0,
                    "verify": 0,
                    "level": 0
                }
            },
            "super_chat_message": {
                "list": []
            },
            "lpl_info": {
                "lpl": 0
            },
            "cd": {
                "guide_free_medal_cost": 0,
                "guide_light_medal": 0,
                "guide_follow": 1,
                "guard_compensate": 0,
                "interact_toasts": []
            }
        }
    }*/

    if (ac->browserCookie.isEmpty())
        return ;
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByUser?room_id=" + ac->roomId;
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取房间本用户信息返回结果不为0：") << json.value("message").toString();
            return ;
        }

        // 获取用户在这个房间信息
        QJsonObject info = json.value("data").toObject().value("info").toObject();
        ac->cookieUid = snum(static_cast<qint64>(info.value("uid").toDouble()));
        ac->cookieUname = info.value("uname").toString();
        QString uface = info.value("uface").toString(); // 头像地址
        qInfo() << "当前cookie用户：" << ac->cookieUid << ac->cookieUname;
    });
}

template<class T>
void MainWindow::showListMenu(QListWidget *listWidget, QString listKey, VoidFunc saveFunc)
{
    QListWidgetItem* item = listWidget->currentItem();
    int row = listWidget->currentRow();

    QString clipText = QApplication::clipboard()->text();
    bool canPaste = false, canContinueCopy = false;
    MyJson clipJson;
    QJsonArray clipArray;
    if (!clipText.isEmpty())
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(clipText.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError)
        {
            if (doc.isObject())
            {
                clipJson = doc.object();
                JS(clipJson, anchor_key);
                canPaste = (anchor_key == listKey);
                qInfo() << anchor_key << listKey;
            }
            else if (doc.isArray())
            {
                clipArray = doc.array();
            }
            canContinueCopy = true;
        }
    }

    auto moveToRow = [=](int newRow){
        auto widget = listWidget->itemWidget(item);
        auto tw = static_cast<ListItemInterface*>(widget);
        MyJson json = tw->toJson();
        listWidget->removeItemWidget(item);
        listWidget->takeItem(row);

        delete item;
        tw->deleteLater();

        auto newTw = new T(listWidget);
        newTw->fromJson(json);
        QListWidgetItem* newItem = new QListWidgetItem;

        listWidget->insertItem(newRow, newItem);
        listWidget->setItemWidget(newItem, newTw);
        newTw->autoResizeEdit();
        newItem->setSizeHint(newTw->sizeHint());

        /* // 这个最简单的方式行不通，不知道为什么widget都会被删掉
        auto item = listWidget->takeItem(row);
        listWidget->insertItem(newRow, item); */

        if (listKey == CODE_TIMER_TASK_KEY)
            connectTimerTaskEvent((TaskWidget*)newTw, newItem);
        else if (listKey == CODE_AUTO_REPLY_KEY)
            connectAutoReplyEvent((ReplyWidget*)newTw, newItem);
        else if (listKey == CODE_EVENT_ACTION_KEY)
            connectEventActionEvent((EventWidget*)newTw, newItem);
        (this->*saveFunc)();
        listWidget->setCurrentRow(newRow);
    };

    auto menu = new FacileMenu(this);
    menu->addAction("插入 (&a)", [=]{
        if (listKey == CODE_TIMER_TASK_KEY)
            addTimerTask(false, 1800, "", row);
        else if (listKey == CODE_AUTO_REPLY_KEY)
            addAutoReply(false, "", "", row);
        else if (listKey == CODE_EVENT_ACTION_KEY)
            addEventAction(false, "", "", row);
        (this->*saveFunc)();
    })->disable(!item);
    menu->addAction("上移 (&w)", [=]{
        moveToRow(row - 1);
    })->disable(!item || row <= 0);
    menu->addAction("下移 (&s)", [=]{
        moveToRow(row + 1);
    })->disable(!item || row >= listWidget->count()-1);
    menu->split()->addAction("复制片段 (&c)", [=]{
        auto widget = listWidget->itemWidget(item);
        auto tw = static_cast<ListItemInterface*>(widget);
        QApplication::clipboard()->setText(tw->toJson().toBa());
    })->disable(!item);
    menu->addAction("继续复制 (&n)", [=]{
        auto widget = listWidget->itemWidget(item);
        auto tw = static_cast<ListItemInterface*>(widget);
        MyJson twJson = tw->toJson();
        if (!clipArray.isEmpty()) // 已经是JSON数组了，继续复制
        {
            QJsonArray array = clipArray;
            array.append(twJson);
            QApplication::clipboard()->setText(QJsonDocument(array).toJson());
        }
        else // 是JSON对象，需要变成数组
        {
            QJsonArray array;
            array.append(clipJson);
            array.append(twJson);
            QApplication::clipboard()->setText(QJsonDocument(array).toJson());
        }
    })->disable(!item)->hide(!canContinueCopy)->tooltip("多次复制代码片段，组合成一串json格式数组\n可用来导入导出");
    menu->addAction("粘贴片段 (&v)", [=]{
        auto widget = listWidget->itemWidget(item);
        auto tw = static_cast<ListItemInterface*>(widget);
        tw->fromJson(clipJson);

        (this->*saveFunc)();
    })->disable(!canPaste)->tooltip("复制代码片段后可粘贴至当前位置\n可以快速导入他人的片段");
    menu->split()->addAction("删除 (&d)", [=]{
        auto widget = listWidget->itemWidget(item);
        auto tw = static_cast<ListItemInterface*>(widget);

        // 特殊操作
        /* if (listKey == CODE_EVENT_ACTION_KEY)
        {
            setFilter(tw->title(), "");
        } */

        listWidget->removeItemWidget(item);
        listWidget->takeItem(listWidget->currentRow());

        (this->*saveFunc)();
        tw->deleteLater();
    })->disable(!item);
    menu->exec();
}

void MainWindow::on_taskListWidget_customContextMenuRequested(const QPoint &)
{
    showListMenu<TaskWidget>(ui->taskListWidget, CODE_TIMER_TASK_KEY, &MainWindow::saveTaskList);
}

void MainWindow::on_replyListWidget_customContextMenuRequested(const QPoint &)
{
    showListMenu<ReplyWidget>(ui->replyListWidget, CODE_AUTO_REPLY_KEY, &MainWindow::saveReplyList);
}

void MainWindow::on_eventListWidget_customContextMenuRequested(const QPoint &)
{
    showListMenu<EventWidget>(ui->eventListWidget, CODE_EVENT_ACTION_KEY, &MainWindow::saveEventList);
}

void MainWindow::slotDiange(const LiveDanmaku &danmaku)
{
    if (danmaku.getMsgType() != MSG_DANMAKU || danmaku.isPkLink() || danmaku.isNoReply())
        return ;
    QRegularExpression re(diangeFormatString);
    QRegularExpressionMatch match;
    if (danmaku.getText().indexOf(re, 0, &match) == -1) // 不是点歌文本
        return ;

    if (match.capturedTexts().size() < 2)
    {
        showError("无法获取点歌内容，请检测点歌格式");
        return ;
    }
    if (ui->diangeNeedMedalCheck->isChecked())
    {
        if (danmaku.getAnchorRoomid() != ac->roomId) // 不是对应的粉丝牌
        {
            qWarning() << "点歌未戴粉丝勋章：" << danmaku.getNickname() << danmaku.getAnchorRoomid() << "!=" << ac->roomId;
            localNotify("点歌未戴粉丝勋章");
            triggerCmdEvent("ORDER_SONG_NO_MEDAL", danmaku, true);
            return ;
        }
    }

    // 记录到历史（先不复制）
    QString text = match.capturedTexts().at(1);
    text = text.trimmed();
    qInfo() << s8("检测到点歌：") << text;
    diangeHistory.append(Diange{danmaku.getNickname(), danmaku.getUid(), text, danmaku.getTimeline()});
    ui->diangeHistoryListWidget->insertItem(0, text + " - " + danmaku.getNickname());

    // 总开关，是否进行复制/播放操作
    if (!diangeAutoCopy)
        return ;

    // 判断关键词
    QString lower = text.toLower();
    foreach (QString s, orderSongBlackList)
    {
        if (lower.contains(s)) // 有违禁词
        {
            MyJson json;
            json.insert("key", s);
            qInfo() << "阻止点歌，关键词：" << s;
            LiveDanmaku dmk = danmaku;
            triggerCmdEvent("ORDER_SONG_BLOCKED", dmk.with(json), true);
            return ;
        }
    }

    // 判断过滤器
    if (cr->isFilterRejected("FILTER_MUSIC_ORDER", danmaku))
        return ;

    // 成功点歌
    if (saveToSqlite)
    {
        LiveDanmaku md = danmaku;
        md.setText(text);
        sqlService.insertMusic(md);
    }

    if (musicWindow && !musicWindow->isHidden()) // 自动播放
    {
        int count = 0;
        if (ui->diangeShuaCheck->isChecked() && (count = musicWindow->userOrderCount(danmaku.getNickname())) >= ui->orderSongShuaSpin->value()) // 已经点了
        {
            localNotify("已阻止频繁点歌：" + snum(count));
            LiveDanmaku dmk = danmaku;
            dmk.setNumber(count);
            triggerCmdEvent("ORDER_SONG_FREQUENCY", dmk, true);
        }
        else
        {
            musicWindow->slotSearchAndAutoAppend(text, danmaku.getNickname());
        }
    }
    else // 复制到剪贴板
    {
        QClipboard* clip = QApplication::clipboard();
        clip->setText(text);
        triggerCmdEvent("ORDER_SONG_COPY", danmaku, true);

        cr->addNoReplyDanmakuText(danmaku.getText()); // 点歌回复
        QTimer::singleShot(10, [=]{
            appendNewLiveDanmaku(LiveDanmaku(danmaku.getNickname(), danmaku.getUid(), text, danmaku.getTimeline()));
        });
    }
}

void MainWindow::slotSocketError(QAbstractSocket::SocketError error)
{
    showError("socket", liveService->liveSocket->errorString());
}

void MainWindow::initWS()
{
    liveService->liveSocket = new QWebSocket();

    connect(liveService->liveSocket, &QWebSocket::connected, this, [=]{
        SOCKET_DEB << "socket connected";
        ui->connectStateLabel->setText("状态：已连接");

        // 5秒内发送认证包
        liveService->sendVeriPacket(liveService->liveSocket, ac->roomId, ac->cookieToken);

        // 定时发送心跳包
        liveService->heartTimer->start();
        liveService->minuteTimer->start();
    });

    connect(liveService->liveSocket, &QWebSocket::disconnected, this, [=]{
        // 正在直播的时候突然断开了
        if (liveService->isLiving())
        {
            qWarning() << "正在直播的时候突然断开，" << (liveService->reconnectWSDuration/1000) << "秒后重连..." << "    " << QDateTime::currentDateTime().toString("HH:mm:ss");
            localNotify("[连接断开，重连...]");
            ac->liveStatus = false;
            // 尝试5秒钟后重连
            // TODO: 每次重连间隔翻倍
            liveService->connectServerTimer->setInterval(liveService->reconnectWSDuration);
            liveService->reconnectWSDuration *= 2;
            if (liveService->reconnectWSDuration > INTERVAL_RECONNECT_WS_MAX)
                liveService->reconnectWSDuration = INTERVAL_RECONNECT_WS_MAX;
        }

        SOCKET_DEB << "disconnected";
        ui->connectStateLabel->setText("状态：未连接");
        ui->liveStatusButton->setText("");

        liveService->heartTimer->stop();
        liveService->minuteTimer->stop();

        // 如果不是主动连接的话，这个会断开
        if (!liveService->connectServerTimer->isActive())
            liveService->connectServerTimer->start();
    });

    connect(liveService->liveSocket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
//        qInfo() << "binaryMessageReceived" << message;
//        for (int i = 0; i < 100; i++) // 测试内存泄漏
        try {
            slotBinaryMessageReceived(message);

            // 保存到CMDS里
            if (saveRecvCmds && saveCmdsFile)
            {
                QByteArray ba = message;
                ba.replace("\n", "__bmd__n__").replace("\r", "__bmd__r__");
                saveCmdsFile->write(ba);
                saveCmdsFile->write("\n");
            }
        } catch (...) {
            qCritical() << "!!!!!!!error:slotBinaryMessageReceived";
        }

    });

    connect(liveService->liveSocket, &QWebSocket::textFrameReceived, this, [=](const QString &frame, bool isLastFrame){
        qInfo() << "textFrameReceived";
    });

    connect(liveService->liveSocket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
        qInfo() << "textMessageReceived";
    });

    connect(liveService->liveSocket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        SOCKET_DEB << "stateChanged" << state;
        QString str = "未知";
        if (state == QAbstractSocket::UnconnectedState)
            str = "未连接";
        else if (state == QAbstractSocket::ConnectingState)
            str = "连接中";
        else if (state == QAbstractSocket::ConnectedState)
            str = "已连接";
        else if (state == QAbstractSocket::BoundState)
            str = "已绑定";
        else if (state == QAbstractSocket::ClosingState)
            str = "断开中";
        else if (state == QAbstractSocket::ListeningState)
            str = "监听中";
        ui->connectStateLabel->setText(str);
    });

    liveService->heartTimer = new QTimer(this);
    liveService->heartTimer->setInterval(30000);
    connect(liveService->heartTimer, &QTimer::timeout, this, [=]{
        if (liveService->liveSocket->state() == QAbstractSocket::ConnectedState)
            liveService->sendHeartPacket();
        else if (liveService->liveSocket->state() == QAbstractSocket::UnconnectedState)
            startConnectRoom();

        // PK Socket
        if (liveService->pkLiveSocket && liveService->pkLiveSocket->state() == QAbstractSocket::ConnectedState)
        {
            QByteArray ba;
            ba.append("[object Object]");
            ba = BiliApiUtil::makePack(ba, OP_HEARTBEAT);
            liveService->pkLiveSocket->sendBinaryMessage(ba);
        }

        // 机器人 Socket
        for (int i = 0; i < robots_sockets.size(); i++)
        {
            QWebSocket* socket = robots_sockets.at(i);
            if (socket->state() == QAbstractSocket::ConnectedState)
            {
                QByteArray ba;
                ba.append("[object Object]");
                ba = BiliApiUtil::makePack(ba, OP_HEARTBEAT);
                socket->sendBinaryMessage(ba);
            }
        }
    });
}

void MainWindow::startConnectIdentityCode()
{
    MyJson json;
    json.insert("code", ac->identityCode); // 主播身份码
    json.insert("app_id", (qint64)BILI_APP_ID);
    liveOpenService->post(BILI_API_DOMAIN + "/v2/app/start", json, [=](MyJson json){
        if (json.code() != 0)
        {
            showError("解析身份码出错", snum(json.code()) + " " + json.msg());
            return ;
        }

        auto data = json.data();
        auto anchor = data.o("anchor_info");
        qint64 roomId = anchor.l("room_id");

        // 通过房间号连接
        ui->roomIdEdit->setText(snum(roomId));
        on_roomIdEdit_editingFinished();
    });
}

void MainWindow::startConnectRoom()
{
    if (ac->roomId.isEmpty())
        return ;

    // 判断是身份码还是房间号
    bool isPureRoomId = (ac->roomId.contains(QRegularExpression("^\\d+$")));
    if (!isPureRoomId)
    {
        QTimer::singleShot(100, [=]{
            startConnectIdentityCode();
        });
        return ;
    }

    // 初始化主播数据
    ac->currentFans = 0;
    ac->currentFansClub = 0;
    liveService->popularVal = 2;

    // 准备房间数据
    if (us->danmakuCounts)
        us->danmakuCounts->deleteLater();
    QDir dir;
    dir.mkdir(rt->dataPath+"danmaku_counts");
    us->danmakuCounts = new QSettings(rt->dataPath+"danmaku_counts/" + ac->roomId + ".ini", QSettings::Format::IniFormat);
    if (us->calculateDailyData)
        liveService->startCalculateDailyData();

    // 开始获取房间信息
    liveService->getRoomInfo(true);

    // 如果是管理员，可以获取禁言的用户
    if (ui->enableBlockCheck->isChecked())
        liveService->refreshBlockList();
}

void MainWindow::updatePermission()
{
    permissionLevel = 0;
    if (liveService->gettingRoom || liveService->gettingUser)
        return ;
    QString userId = ac->cookieUid;
    get(serverPath + "pay/isVip", {"room_id", ac->roomId, "user_id", userId}, [=](MyJson json) {
        MyJson jdata = json.data();
        qint64 timestamp = QDateTime::currentSecsSinceEpoch();
        qint64 deadline = 0;
        if (!jdata.value("RR").isNull())
        {
            MyJson info = jdata.o("RR");
            if (info.l("deadline") > timestamp)
            {
                permissionLevel = qMax(permissionLevel, info.i("vipLevel"));
                deadline = qMax(deadline, info.l("deadline"));
                permissionType[1] = true;
            }
        }
        if (!jdata.value("ROOM").isNull())
        {
            MyJson info = jdata.o("ROOM");
            if (info.l("deadline") > timestamp)
            {
                permissionLevel = qMax(permissionLevel, info.i("vipLevel"));
                deadline = qMax(deadline, info.l("deadline"));
                permissionType[2] = true;
            }
        }
        if (!jdata.value("ROBOT").isNull())
        {
            MyJson info = jdata.o("ROBOT");
            if (info.l("deadline") > timestamp)
            {
                permissionLevel = qMax(permissionLevel, info.i("vipLevel"));
                deadline = qMax(deadline, info.l("deadline"));
                permissionType[3] = true;
            }
        }
        if (!jdata.value("GIFT").isNull())
        {
            MyJson info = jdata.o("GIFT");
            if (info.l("deadline") > timestamp)
            {
                permissionLevel = qMax(permissionLevel, info.i("vipLevel"));
                deadline = qMax(deadline, info.l("deadline"));
                permissionType[10] = true;
            }
        }

        this->permissionDeadline = deadline;
        if (permissionLevel)
        {
            ui->droplight->setText(permissionText);
            ui->droplight->adjustMinimumSize();
            ui->droplight->setNormalColor(themeSbg);
            ui->droplight->setTextColor(themeSfg);
            ui->droplight->setToolTip("剩余时长：" + snum((deadline - timestamp) / (24 * 3600)) + "天");
            ui->vipExtensionButton->hide();
            ui->heartTimeSpin->setMaximum(1440); // 允许挂机24小时

            qint64 tm = deadline - timestamp;
            if (tm > 24 * 3600)
                tm = 24 * 3600;
            permissionTimer->setInterval(tm * 1000);
            permissionTimer->start();
        }
        else
        {
            ui->droplight->setText("免费版");
            ui->droplight->setNormalColor(Qt::white);
            ui->droplight->setTextColor(Qt::black);
            ui->droplight->setToolTip("点击解锁新功能");
            ui->vipExtensionButton->show();
            ui->heartTimeSpin->setMaximum(120); // 仅允许刚好获取完小心心
            permissionTimer->stop();
        }

        readConfig2();
    });
}

int MainWindow::hasPermission()
{
    return justStart || permissionLevel;
}

void MainWindow::setRoomCover(const QPixmap& pixmap)
{
    liveService->roomCover = pixmap; // 原图
    if (liveService->roomCover.isNull())
        return ;

    adjustCoverSizeByRoomCover(liveService->roomCover);

    /* int w = ui->roomCoverLabel->width();
    if (w > ui->tabWidget->contentsRect().width())
        w = ui->tabWidget->contentsRect().width();
    pixmap = pixmap.scaledToWidth(w, Qt::SmoothTransformation);
    ui->roomCoverLabel->setPixmap(PixmapUtil::getRoundedPixmap(pixmap));
    ui->roomCoverLabel->setMinimumSize(1, 1); */

    // 设置程序主题
    QColor bg, fg, sbg, sfg;
    auto colors = ColorOctreeUtil::extractImageThemeColors(liveService->roomCover.toImage(), 7);
    ColorOctreeUtil::getBgFgSgColor(colors, &bg, &fg, &sbg, &sfg);
    prevPa = BFSColor::fromPalette(palette());
    currentPa = BFSColor(QList<QColor>{bg, fg, sbg, sfg});
    QPropertyAnimation* ani = new QPropertyAnimation(this, "paletteProg");
    ani->setStartValue(0);
    ani->setEndValue(1.0);
    ani->setDuration(500);
    connect(ani, &QPropertyAnimation::valueChanged, this, [=](const QVariant& val){
        setRoomThemeByCover(val.toDouble());
    });
    connect(ani, &QPropertyAnimation::stateChanged, ani, [=](QAbstractAnimation::State newState, QAbstractAnimation::State oldState){
        if (newState != QAbstractAnimation::Running)
            ani->deleteLater();
    });
    ani->start();

    // 设置主要界面主题
    ui->tabWidget->setBg(liveService->roomCover);
}

void MainWindow::setRoomThemeByCover(double val)
{
    double d = val;
    BFSColor bfs = prevPa + (currentPa - prevPa) * d;
    QColor bg, fg, sbg, sfg;
    bfs.toColors(&bg, &fg, &sbg, &sfg);

    themeBg = bg;
    themeFg = fg;
    themeSbg = sbg;
    themeSfg = sfg;
    ListItemInterface::triggerColor = sbg;

    QPalette pa;
    pa.setColor(QPalette::Window, bg);
    pa.setColor(QPalette::Background, bg);
    pa.setColor(QPalette::Button, bg);
    pa.setColor(QPalette::Base, bg);

    pa.setColor(QPalette::Foreground, fg);
    pa.setColor(QPalette::Text, fg);
    pa.setColor(QPalette::ButtonText, fg);
    pa.setColor(QPalette::WindowText, fg);

    // 设置背景透明
    QString labelStyle = "color:" + QVariant(fg).toString() + ";";
    statusLabel->setStyleSheet(labelStyle);
    ui->roomNameLabel->setStyleSheet(labelStyle + "font-size: 18px;");

    pa.setColor(QPalette::Highlight, sbg);
    pa.setColor(QPalette::HighlightedText, sfg);
    setPalette(pa);
    // setStyleSheet("QMainWindow{background:"+QVariant(bg).toString()+"}");
    ui->menubar->setStyleSheet("QMenuBar:item{background:transparent;}QMenuBar{background:transparent; color:"+QVariant(fg).toString()+"}");
    roomIdBgWidget->setStyleSheet("#roomIdBgWidget{ background: " + QVariant(themeSbg).toString() + "; border-radius: " + snum(roomIdBgWidget->height()/2) + "px; }");
    sideButtonList.at(ui->stackedWidget->currentIndex())->setNormalColor(sbg);
    ui->tagsButtonGroup->setMouseColor([=]{QColor c = themeSbg; c.setAlpha(127); return c;}(),
                                       [=]{QColor c = themeSbg; c.setAlpha(255); return c;}());
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setNormalColor(sbg);
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setTextColor(sfg);
    ui->showLiveDanmakuWindowButton->setBgColor(sbg);
    ui->showLiveDanmakuWindowButton->setTextColor(sfg);
    ui->SendMsgButton->setTextColor(fg);
    ui->showOrderPlayerButton->setNormalColor(sbg);
    ui->showOrderPlayerButton->setTextColor(sfg);
    // ui->addMusicToLiveButton->setBorderColor(sbg); // 感觉太花哨了
    appendListItemButton->setBgColor(sbg);
    appendListItemButton->setIconColor(sfg);
    if (hasPermission())
    {
        ui->droplight->setNormalColor(sbg);
        ui->droplight->setTextColor(sfg);
    }

    // 纯文字label的
    pa = ui->appNameLabel->palette();
    pa.setColor(QPalette::Foreground, fg);
    pa.setColor(QPalette::Text, fg);
    pa.setColor(QPalette::ButtonText, fg);
    pa.setColor(QPalette::WindowText, fg);
    ui->appNameLabel->setPalette(pa);
    ui->appDescLabel->setPalette(pa);

    QColor bgTrans = sbg;
    int alpha = (3 * ( bgTrans.red()) * (bgTrans.red())
                 + 4 * (bgTrans.green()) * (bgTrans.green())
                 + 2 * (bgTrans.blue()) * (bgTrans.blue()))
                 / 9 / 255;
    alpha = 16 + alpha / 4; // 16~80
    bgTrans.setAlpha(alpha);
    QString cardStyleSheet = "{ background: " + QVariant(bgTrans).toString() + "; border: none; border-radius: " + snum(rt->fluentRadius) + " }";
    ui->guardCountCard->setStyleSheet("#guardCountCard" + cardStyleSheet);
    ui->hotCountCard->setStyleSheet("#hotCountCard" + cardStyleSheet);
    ui->watchedCountCard->setStyleSheet("#watchedCountCard" + cardStyleSheet);
    ui->robotSendCountCard->setStyleSheet("#robotSendCountCard" + cardStyleSheet);

    alpha = (3 * ( themeGradient.red()) * (themeGradient.red())
                 + 4 * (themeGradient.green()) * (themeGradient.green())
                 + 2 * (themeGradient.blue()) * (themeGradient.blue()))
                 / 9 / 255;
    alpha = 16 + alpha / 4; // 16~80
    themeGradient = themeBg;
    themeGradient.setAlpha(alpha);
}

void MainWindow::adjustCoverSizeByRoomCover(QPixmap pixmap)
{
    // 计算尺寸
    int w = ui->roomInfoMainWidget->width();
    int maxH = this->height() * (1 - 0.618); // 封面最大高度
    pixmap = pixmap.scaledToWidth(w, Qt::SmoothTransformation);
    int showH = pixmap.height(); // 当前高度下图片应当显示的高度
    if (showH > maxH) // 不能超过原始高度
        showH = maxH;
    int spacingH = showH - (ui->upNameLabel->y() - ui->upHeaderLabel->y()) - ui->roomCoverSpacingLabel->y(); // 间隔控件的高度
    spacingH -= 18;
    ui->roomCoverSpacingLabel->setFixedHeight(spacingH);
    ui->roomInfoMainWidget->layout()->activate(); // 不激活一下布局的话，在启动时会有问题

    // 因为布局原因，实际上显示的不一定是完整图片所需的尺寸（至少宽度是够的）
    QPixmap& p = pixmap;
    int suitH = ui->upNameLabel->y() - 6; // 最适合的高度
    if (suitH < p.height())
        p = p.copy(0, (p.height() - suitH) / 2, p.width(), suitH);
    roomCoverLabel->setPixmap(PixmapUtil::getTopRoundedPixmap(p, rt->fluentRadius));
    roomCoverLabel->resize(p.size());
    roomCoverLabel->lower();
}

void MainWindow::adjustRoomIdWidgetPos()
{
    QSize editSize = ui->roomIdEdit->size();
    QSize sz = QSize(ui->roomIdSpacingWidget->x() + editSize.width() + editSize.height() * 2, editSize.height() + roomIdBgWidget->layout()->margin() * 2);
    roomIdBgWidget->setFixedSize(sz);
    ui->roomIdSpacingWidget->setFixedHeight(sz.height() - 9); // 有个间距
    ui->roomIdSpacingWidget_3->setFixedHeight(sz.height() - 9); // 有个间距
    roomIdBgWidget->move(ui->sideBarWidget->x() + ui->sideBarWidget->width() / 20, ui->sideBarWidget->y());
    roomIdBgWidget->setStyleSheet("#roomIdBgWidget{ background: " + QVariant(themeSbg).toString() + "; border-radius: " + snum(roomIdBgWidget->height()/2) + "px; }");
    ui->sideBarWidget->raise();
    roomIdBgWidget->stackUnder(ui->sideBarWidget);
}

void MainWindow::showRoomIdWidget()
{
    QPropertyAnimation* ani = new QPropertyAnimation(roomIdBgWidget, "pos");
    if (roomIdBgWidget->isHidden())
        ani->setStartValue(QPoint(-roomIdBgWidget->width(), roomIdBgWidget->y()));
    else
        ani->setStartValue(roomIdBgWidget->pos());
    ani->setEndValue(roomIdBgWidget->pos());
    ani->setDuration(300);
    ani->setEasingCurve(QEasingCurve::OutCirc);
    connect(ani, &QPropertyAnimation::finished, this, [=]{
        ani->deleteLater();
    });
    roomIdBgWidget->show();
    ani->start();
}

void MainWindow::hideRoomIdWidget()
{
    if (roomIdBgWidget->isHidden())
        return ;
    QPropertyAnimation* ani = new QPropertyAnimation(roomIdBgWidget, "pos");
    ani->setStartValue(roomIdBgWidget->pos());
    ani->setEndValue(QPoint(-roomIdBgWidget->width(), roomIdBgWidget->y()));
    ani->setDuration(300);
    ani->setEasingCurve(QEasingCurve::OutCirc);
    connect(ani, &QPropertyAnimation::finished, this, [=]{
        roomIdBgWidget->hide();
        ani->deleteLater();
    });
    ani->start();
}

double MainWindow::getPaletteBgProg() const
{
    return paletteProg;
}

void MainWindow::setPaletteBgProg(double x)
{
    this->paletteProg = x;
}

void MainWindow::saveTouta()
{
    us->setValue("pk/toutaCount", liveService->toutaCount);
    us->setValue("pk/chiguaCount", liveService->chiguaCount);
    us->setValue("pk/toutaGold", liveService->toutaGold);
    ui->pkAutoMelonCheck->setToolTip(QString("偷塔次数：%1\n吃瓜数量：%2\n金瓜子数：%3").arg(liveService->toutaCount).arg(liveService->chiguaCount).arg(liveService->toutaGold));
}

void MainWindow::restoreToutaGifts(QString text)
{
    liveService->toutaGifts.clear();
    QStringList sl = text.split("\n", QString::SkipEmptyParts);
    foreach (QString s, sl)
    {
        QStringList l = s.split(" ", QString::SkipEmptyParts);
        // 礼物ID 名字 金瓜子
        if (l.size() < 3)
            continue;
        int id = l.at(0).toInt();
        QString name = l.at(1);
        int gold = l.at(2).toInt();
        liveService->toutaGifts.append(LiveDanmaku("", id, name, 1, 0, QDateTime(), "gold", gold));
    }
}

void MainWindow::initLiveRecord()
{
    connect(&m3u8Downloader, &M3u8Downloader::signalProgressChanged, this, [=](qint64 size) {
        qint64 MB = size / 1024 / 1024; // 单位：M
        ui->recordCheck->setToolTip("文件大小：" + snum(MB) + " M");
    });
    connect(&m3u8Downloader, &M3u8Downloader::signalTimeChanged, this, [=](qint64 msecond) {
        qint64 second = msecond / 1000;
        qint64 hour = second / 60 / 60;
        qint64 mint = second / 60 % 60;
        qint64 secd = second % 60;
        QString timeStr;
        if (hour > 0)
        timeStr = QString("录制中：%1:%2:%3").arg(hour, 2, 10, QLatin1Char('0'))
                .arg(mint, 2, 10, QLatin1Char('0'))
                .arg(secd, 2, 10, QLatin1Char('0'));
        else
            timeStr = QString("录制中：%2:%3").arg(mint, 2, 10, QLatin1Char('0'))
                                             .arg(secd, 2, 10, QLatin1Char('0'));
        ui->recordCheck->setText(timeStr);
    });
    connect(&m3u8Downloader, &M3u8Downloader::signalSaved, this, [=](QString path) {
        // 发送事件
        LiveDanmaku dmk;
        dmk.setText(path);
        QString fileName = QFileInfo(path).fileName();
        dmk.setNickname(fileName);
        triggerCmdEvent("LIVE_RECORD_SAVED", dmk);

        // 进行格式转换
        if (ui->recordFormatCheck->isChecked())
        {
            QDir dir = QFileInfo(path).dir();
            QString newPath = QFileInfo(dir.absoluteFilePath(fileName + ".mp4")).absoluteFilePath();

            // 调用 FFmpeg 转换格式
            recordConvertProcess = new QProcess(nullptr);
            QProcess* currPoc = recordConvertProcess;
            QString cmd = QString(rt->ffmpegPath + " -i \"%1\" -c copy \"%2\"").arg(path).arg(newPath);
            connect(currPoc, &QProcess::errorOccurred, this, [=](QProcess::ProcessError error){
                qWarning() << "录播转换出错：" << error << currPoc;
                QString err = snum(error);
                if (rt->ffmpegPath.isEmpty() || !isFileExist(rt->ffmpegPath))
                    err = "FFmpeg文件路径出错";
                showError("录播转换出错", err);
                currPoc->close();
                currPoc->deleteLater();
                if (recordConvertProcess == currPoc)
                    recordConvertProcess = nullptr;
            });
            connect(currPoc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
                qInfo() << "录播转换格式保存路径：" << newPath;
                LiveDanmaku dmk;
                dmk.setText(newPath);
                dmk.setNickname(fileName);
                triggerCmdEvent("LIVE_RECORD_FORMATTED", dmk);
                currPoc->close();
                currPoc->deleteLater();
                if (recordConvertProcess == currPoc)
                    recordConvertProcess = nullptr;
                QTimer::singleShot(1000, [=]{
                    deleteFile(path); // 延迟删除旧文件，怕被占用
                });
            });
            currPoc->start(cmd);
        }

        ui->recordCheck->setText("原画录播");
    });
}

void MainWindow::startLiveRecord()
{
    if (m3u8Downloader.isDownloading())
        finishLiveRecord();
    if (ac->roomId.isEmpty())
        return ;

    getRoomLiveVideoUrl([=](QString url){
        // 检查能否下载
        /* recordUrl = "";
        qInfo() << "准备录播：" << url;
        QNetworkAccessManager manager;
        QNetworkRequest* request = new QNetworkRequest(QUrl(url));
        QEventLoop* loop = new QEventLoop();
        QNetworkReply *reply = manager.get(*request);

        connect(reply, SIGNAL(finished()), loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
        connect(reply, &QNetworkReply::downloadProgress, loop, [=](qint64 bytesReceived, qint64 bytesTotal){
            if (bytesReceived > 0)
            {
                // qInfo() << "原始地址可直接下载";
                recordUrl = url;
                loop->quit();
            }
        });
        loop->exec(); //开启子事件循环
        loop->deleteLater();

        // B站下载会有302重定向的，需要获取headers里面的Location值
        QString forward = reply->rawHeader("location").trimmed();
        if (recordUrl.isEmpty() || !forward.isEmpty())
        {
            recordUrl = forward;
            qInfo() << "录播实际地址：" << recordUrl;
        }
        delete request;
        reply->deleteLater();
        if (recordUrl.isEmpty())
        {
            qWarning() << "无法获取下载地址" << forward;
            return ;
        }

        // 开始下载文件
        startRecordUrl(recordUrl); */

        startRecordUrl(url);
    });
}

void MainWindow::startRecordUrl(QString url)
{
    qInfo() << "开始录播:" << url << QDateTime::currentDateTime();
    qint64 startSecond = QDateTime::currentSecsSinceEpoch();
    QDir dir(rt->dataPath);
    dir.mkpath("record");
    dir.cd("record");
    QString fileName = ac->roomId + "_" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh.mm.ss");
    QString suffix = ".flv";
    if (url.contains(".m3u8"))
        suffix = ".ts";
    QString path = QFileInfo(dir.absoluteFilePath(fileName + suffix)).absoluteFilePath();
    recordLastPath = path;

    ui->recordCheck->setText("录制中...");
    recordTimer->start();
    startRecordTime = QDateTime::currentMSecsSinceEpoch();

    // 开始下载
    m3u8Downloader.start(url, path);

    return ;
    /* recordLoop = new QEventLoop;
    QNetworkAccessManager manager;
    QNetworkRequest* request = new QNetworkRequest(QUrl(url));
    request->setHeader(QNetworkRequest::CookieHeader, ac->userCookies);
    QNetworkReply *reply = manager.get(*request);
    // 写入文件
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Append))
    {
        qCritical() << "写入文件失败" << path;
        reply->deleteLater();
        return ;
    }
    QObject::connect(reply, SIGNAL(finished()), recordLoop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环

    // 下载进度：progress=下载总大小（单位Byte）, total=-1
    connect(reply, &QNetworkReply::downloadProgress, this, [=](qint64 progress, qint64 total){
        qint64 size = progress / 1024 / 1024; // 单位：M
        ui->recordCheck->setToolTip("文件大小：" + snum(size) + " M");
    });
    // 实时保存到缓冲区
    connect(reply, &QNetworkReply::readyRead, this, [&]{
        QByteArray data = reply->read(1024*1024*1024);
        qDebug() << "保存大小：" << data.size();
        file.write(data);
        qint64 second = QDateTime::currentSecsSinceEpoch() - startSecond;
        qint64 hour = second / 60 / 60;
        qint64 mint = second / 60 % 60;
        qint64 secd = second % 60;
        QString timeStr;
        if (hour > 0)
        timeStr = QString("录制中：%1:%2:%3").arg(hour, 2, 10, QLatin1Char('0'))
                .arg(mint, 2, 10, QLatin1Char('0'))
                .arg(secd, 2, 10, QLatin1Char('0'));
        else
            timeStr = QString("录制中：%2:%3").arg(mint, 2, 10, QLatin1Char('0'))
                                             .arg(secd, 2, 10, QLatin1Char('0'));
        ui->recordCheck->setText(timeStr);
    });

    // 开启子事件循环
    recordLoop->exec();
    disconnect(reply, nullptr, recordLoop, nullptr);
    recordLoop->deleteLater();
    recordLoop = nullptr;

    // 刷新缓冲区，永久保存到磁盘
    file.flush();
    qInfo() << "录播完成，保存至：" << path;

    // 发送事件
    LiveDanmaku dmk;
    dmk.setText(path);
    dmk.setNickname(fileName);
    triggerCmdEvent("LIVE_RECORD_SAVED", dmk);

    // 进行格式转换
    if (ui->recordFormatCheck->isChecked())
    {
        QString newPath = QFileInfo(dir.absoluteFilePath(fileName + ".mp4")).absoluteFilePath();

        // 调用 FFmpeg 转换格式
        recordConvertProcess = new QProcess(nullptr);
        QProcess* currPoc = recordConvertProcess;
        QString cmd = QString(rt->ffmpegPath + " -i \"%1\" -c copy \"%2\"").arg(path).arg(newPath);
        connect(currPoc, &QProcess::errorOccurred, this, [=](QProcess::ProcessError error){
            qWarning() << "录播转换出错：" << error << currPoc;
            QString err = snum(error);
            if (rt->ffmpegPath.isEmpty() || !isFileExist(rt->ffmpegPath))
                err = "FFmpeg文件路径出错";
            showError("录播转换出错", err);
            currPoc->close();
            currPoc->deleteLater();
            if (recordConvertProcess == currPoc)
                recordConvertProcess = nullptr;
        });
        connect(currPoc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
            qInfo() << "录播转换格式保存路径：" << newPath;
            LiveDanmaku dmk;
            dmk.setText(newPath);
            dmk.setNickname(fileName);
            triggerCmdEvent("LIVE_RECORD_FORMATTED", dmk);
            currPoc->close();
            currPoc->deleteLater();
            if (recordConvertProcess == currPoc)
                recordConvertProcess = nullptr;
            QTimer::singleShot(1000, [=]{
                deleteFile(path); // 延迟删除旧文件，怕被占用
            });
        });
        currPoc->start(cmd);
    }

    // 删除变量
    reply->deleteLater();
    delete request;
    startRecordTime = 0;
    ui->recordCheck->setText("原画录播");
    recordTimer->stop();

    // 可能是时间结束了，重新下载
    if (ui->recordCheck->isChecked() && isLiving())
    {
        startLiveRecord();
    } */
}

/**
 * 结束录制
 * 可能是点击按钮取消录制
 * 也可能是时间到了，需要重新录制
 */
void MainWindow::finishLiveRecord()
{
    if (m3u8Downloader.isDownloading())
    {
        m3u8Downloader.stop();

        // 可能是时间结束了，重新下载
        if (ui->recordCheck->isChecked() && liveService->isLiving())
        {
            startLiveRecord();
        }
    }

    /* if (!recordLoop)
        return ;
    qInfo() << "结束录播";
    recordLoop->quit();
    recordLoop = nullptr; */
}

void MainWindow::restoreCustomVariant(QString text)
{
    us->customVariant.clear();
    QStringList sl = text.split("\n", QString::SkipEmptyParts);
    bool settedUpname = true;
    foreach (QString s, sl)
    {
        QRegularExpression re("^\\s*(\\S+?)\\s*=\\s?(.*)$");
        QRegularExpressionMatch match;
        if (s.indexOf(re, 0, &match) != -1)
        {
            QString key = match.captured(1);
            QString val = match.captured(2);
            us->customVariant.append(QPair<QString, QString>(key, val));
            if ((key == "%upname%") && val.trimmed().isEmpty())
            {
                settedUpname = false;
            }
            else
            {
                settedUpname = true;
            }
        }
        else
            qCritical() << "自定义变量读取失败：" << s;
    }
    customVarsButton->setBgColor(settedUpname ? Qt::transparent : Qt::white);
}

QString MainWindow::saveCustomVariant()
{
    QStringList sl;
    for (auto it = us->customVariant.begin(); it != us->customVariant.end(); ++it)
    {
        sl << it->first + " = " + it->second;
    }
    return sl.join("\n");
}

void MainWindow::restoreVariantTranslation()
{
    us->variantTranslation.clear();
    QStringList allVariants;

    // 变量
    QString text = readTextFile(":/documents/translation_variables");
    QStringList sl = text.split("\n", QString::SkipEmptyParts);
    QRegularExpression re("^\\s*(\\S+)\\s*=\\s?(.*)$");
    QRegularExpressionMatch match;
    foreach (QString s, sl)
    {
        if (s.indexOf(re, 0, &match) != -1)
        {
            QString key = match.captured(1);
            QString val = match.captured(2);
            key = "%" + key + "%";
            val = "%" + val + "%";
            us->variantTranslation.append(QPair<QString, QString>(key, val));
            allVariants.append(key);
            allVariants.append(val);
        }
        else
            qCritical() << "多语言翻译读取失败：" << s;
    }

    // 方法
    text = readTextFile(":/documents/translation_methods");
    sl = text.split("\n", QString::SkipEmptyParts);
    re = QRegularExpression("^\\s*(\\S+)\\s*=\\s?(.*)$");
    foreach (QString s, sl)
    {
        if (s.indexOf(re, 0, &match) != -1)
        {
            QString key = match.captured(1);
            QString val = match.captured(2);
            key = ">" + key + "(";
            val = ">" + val + "(";
            us->variantTranslation.append(QPair<QString, QString>(key, val));
            allVariants.append(key);
            allVariants.append(val);
        }
        else
            qCritical() << "多语言翻译读取失败：" << s;
    }

    // 函数
    text = readTextFile(":/documents/translation_functions");
    sl = text.split("\n", QString::SkipEmptyParts);
    re = QRegularExpression("^\\s*(\\S+)\\s*=\\s?(.*)$");
    foreach (QString s, sl)
    {
        if (s.indexOf(re, 0, &match) != -1)
        {
            QString key = match.captured(1);
            QString val = match.captured(2);
            key = "%>" + key + "(";
            val = "%>" + val + "(";
            us->variantTranslation.append(QPair<QString, QString>(key, val));
            allVariants.append(key);
            allVariants.append(val);
        }
        else
            qCritical() << "多语言翻译读取失败：" << s;
    }

    // 添加自定义关键词
    allVariants.append(">reject(");
    allVariants.append("%up_name%");
    ConditionEditor::allCompletes = allVariants;
}

void MainWindow::restoreReplaceVariant(QString text)
{
    us->replaceVariant.clear();
    QStringList sl = text.split("\n", QString::SkipEmptyParts);
    foreach (QString s, sl)
    {
        QRegularExpression re("^\\s*(\\S+?)\\s*=\\s?(.*)$");
        QRegularExpressionMatch match;
        if (s.indexOf(re, 0, &match) != -1)
        {
            QString key = match.captured(1);
            QString val = match.captured(2);
            us->replaceVariant.append(QPair<QString, QString>(key, val));
        }
        else
            qCritical() << "替换变量读取失败：" << s;
    }
}

QString MainWindow::saveReplaceVariant()
{
    QStringList sl;
    for (auto it = us->replaceVariant.begin(); it != us->replaceVariant.end(); ++it)
    {
        sl << it->first + " = " + it->second;
    }
    return sl.join("\n");
}

void MainWindow::savePlayingSong()
{
    Song song = musicWindow ? musicWindow->getPlayingSong() : Song();
    QString format = ui->playingSongToFileFormatEdit->text();
    QString text;
    if (song.isValid())
    {
        text = format;
        text = text.replace("{歌名}", song.name)
                .replace("{歌手}", song.artistNames)
                .replace("{用户}", song.addBy)
                .replace("{时长}", snum(song.duration/60) + ":" + snum(song.duration%60))
                .replace("{专辑}", song.album.name);
    }
    else
    {
        text = " ";
    }

    // 获取路径
    QDir dir(webServer->wwwDir.absoluteFilePath("music"));
    dir.mkpath(dir.absolutePath());

    // 保存名字到文件
    QFile file(dir.absoluteFilePath("playing.txt"));
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file);
    stream.setGenerateByteOrderMark(true);
    if (!liveService->externFileCodec.isEmpty())
        stream.setCodec(liveService->externFileCodec.toUtf8());
    stream << text;
    file.flush();
    file.close();

    // 保存封面
    QPixmap cover = musicWindow->getCurrentSongCover();
    cover.save(dir.absoluteFilePath("cover.jpg"));
}

void MainWindow::saveOrderSongs(const SongList &songs)
{
    int count = qMin(songs.size(), ui->orderSongsToFileMaxSpin->value());
    QString format = ui->orderSongsToFileFormatEdit->text();

    Song currentSong = musicWindow ? musicWindow->getPlayingSong() : Song();

    // 组合成长文本
    QStringList sl;
    for (int i = 0; i < count; i++)
    {
        Song song = songs.at(i);
        QString text = format;
        text = text.replace("{序号}", snum(i+1))
                .replace("{歌名}", song.name)
                .replace("{歌手}", song.artistNames)
                .replace("{用户}", song.addBy)
                .replace("{时长}", snum(song.duration/60) + ":" + snum(song.duration%60))
                .replace("{专辑}", song.album.name)
                .replace("{当前歌名}", currentSong.name)
                .replace("{当前歌手}", currentSong.artistNames)
                .replace("{当前用户}", currentSong.addBy);
        sl.append(text);
    }
    if (!sl.size()) // 直播姬会跳过空文本文件，所以需要设置个空格
        sl.append(" ");

    // 获取路径
    QDir dir(webServer->wwwDir.absoluteFilePath("music"));
    dir.mkpath(dir.absolutePath());

    // 保存到文件
    QFile file(dir.absoluteFilePath("songs.txt"));
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file);
    stream.setGenerateByteOrderMark(true);
    if (!liveService->externFileCodec.isEmpty())
        stream.setCodec(liveService->externFileCodec.toUtf8());
    stream << sl.join("\n");
    file.flush();
    file.close();
}

void MainWindow::saveSongLyrics()
{
    QStringList lyrics = musicWindow->getSongLyrics(ui->songLyricsToFileMaxSpin->value());
    if (!lyrics.size()) // 直播姬会跳过空文本文件，所以需要设置个空格
        lyrics.append(" ");

    // 获取路径
    QDir dir(webServer->wwwDir.absoluteFilePath("music"));
    dir.mkpath(dir.absolutePath());

    // 保存到文件
    QFile file(dir.absoluteFilePath("lyrics.txt"));
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file);
    stream.setGenerateByteOrderMark(true);
    if (!liveService->externFileCodec.isEmpty())
        stream.setCodec(liveService->externFileCodec.toUtf8());
    stream << lyrics.join("\n");
    file.flush();
    file.close();
}



void MainWindow::slotUncompressBytes(const QByteArray &body)
{
    splitUncompressedBody(BiliApiUtil::zlibToQtUncompr(body.data(), body.size()+1));
}

void MainWindow::splitUncompressedBody(const QByteArray &unc)
{
    int offset = 0;
    short headerSize = 16;
    while (offset < unc.size() - headerSize)
    {
        int packSize = ((uchar)unc[offset+0] << 24)
                + ((uchar)unc[offset+1] << 16)
                + ((uchar)unc[offset+2] << 8)
                + (uchar)unc[offset+3];
        QByteArray jsonBa = unc.mid(offset + headerSize, packSize - headerSize);
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(jsonBa, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << s8("解析解压后的JSON出错：") << error.errorString();
            qCritical() << s8("包数值：") << offset << packSize << "  解压后大小：" << unc.size();
            qCritical() << s8(">>当前JSON") << jsonBa;
            qCritical() << s8(">>解压正文") << unc;
            return ;
        }
        QJsonObject json = document.object();
        QString cmd = json.value("cmd").toString();
        SOCKET_INF << "解压后获取到CMD：" << cmd;
        if (saveToSqlite && saveCmdToSqlite)
        {
            sqlService.insertCmd(cmd, jsonBa);
        }
        if (cmd != "ROOM_BANNER" && cmd != "ACTIVITY_BANNER_UPDATE_V2" && cmd != "PANEL"
                && cmd != "ONLINERANK")
            SOCKET_INF << "单个JSON消息：" << offset << packSize << QString(jsonBa);
        try {
            handleMessage(json);
        } catch (...) {
            qCritical() << s8("出错啦") << jsonBa;
        }

        offset += packSize;
    }
}

void MainWindow::sendWelcomeIfNotRobot(LiveDanmaku danmaku)
{
    if (judgeRobot != 2)
    {
        sendWelcome(danmaku);
        return ;
    }

    judgeUserRobotByFans(danmaku, [=](LiveDanmaku danmaku){
        sendWelcome(danmaku);
    }, [=](LiveDanmaku danmaku){
        // 实时弹幕显示机器人
        if (danmakuWindow)
            danmakuWindow->markRobot(danmaku.getUid());
    });
}

void MainWindow::sendAttentionThankIfNotRobot(LiveDanmaku danmaku)
{
    judgeUserRobotByFans(danmaku, [=](LiveDanmaku danmaku){
        sendAttentionThans(danmaku);
    }, [=](LiveDanmaku danmaku){
        // 实时弹幕显示机器人
        if (danmakuWindow)
            danmakuWindow->markRobot(danmaku.getUid());
    });
}

void MainWindow::judgeUserRobotByFans(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs)
{
    int val = robotRecord->value("robot/" + snum(danmaku.getUid()), 0).toInt();
    if (val == 1) // 是机器人
    {
        if (ifIs)
            ifIs(danmaku);
        return ;
    }
    else if (val == -1) // 是人
    {
        if (ifNot)
            ifNot(danmaku);
        return ;
    }
    else
    {
        if (danmaku.getMedalLevel() > 0 || danmaku.getLevel() > 1) // 使用等级判断
        {
            robotRecord->setValue("robot/" + snum(danmaku.getUid()), -1);
            if (ifNot)
                ifNot(danmaku);
            return ;
        }
    }

    // 网络判断
    QString url = "https://api.bilibili.com/x/relation/stat?vmid=" + snum(danmaku.getUid());
    get(url, [=](QJsonObject json){
        int code = json.value("code").toInt();
        if (code != 0)
        {
            if (code == 403)
                showError("机器人判断粉丝", "您没有权限");
            else
                showError("机器人判断:粉丝", json.value("message").toString());
            return ;
        }
        QJsonObject obj = json.value("data").toObject();
        int following = obj.value("following").toInt(); // 关注
        int follower = obj.value("follower").toInt(); // 粉丝
        // int whisper = obj.value("whisper").toInt(); // 悄悄关注（自己关注）
        // int black = obj.value("black").toInt(); // 黑名单（自己登录）
        bool robot =  (following >= 100 && follower <= 5) || (follower > 0 && following > follower * 100); // 机器人，或者小号
//        qInfo() << "判断机器人：" << danmaku.getNickname() << "    粉丝数：" << following << follower << robot;
        if (robot)
        {
            // 进一步使用投稿数量判断
            judgeUserRobotByUpstate(danmaku, ifNot, ifIs);
        }
        else // 不是机器人
        {
            robotRecord->setValue("robot/" + snum(danmaku.getUid()), -1);
            if (ifNot)
                ifNot(danmaku);
        }
    });
}

void MainWindow::judgeUserRobotByUpstate(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs)
{
    QString url = "https://api.bilibili.com/x/space/upstat?mid=" + snum(danmaku.getUid());
    get(url, [=](QJsonObject json){
        int code = json.value("code").toInt();
        if (code != 0)
        {
            if (code == 403)
                showError("机器人判断:点击量", "您没有权限");
            else
                showError("机器人判断:点击量", json.value("message").toString());
            return ;
        }
        QJsonObject obj = json.value("data").toObject();
        int achive_view = obj.value("archive").toObject().value("view").toInt();
        int article_view = obj.value("article").toObject().value("view").toInt();
        int article_like = obj.value("article").toObject().value("like").toInt();
        bool robot = (achive_view + article_view + article_like < 10); // 机器人，或者小号
//        qInfo() << "判断机器人：" << danmaku.getNickname() << "    视频播放量：" << achive_view
//                 << "  专栏阅读量：" << article_view << "  专栏点赞数：" << article_like << robot;
        robotRecord->setValue("robot/" + snum(danmaku.getUid()), robot ? 1 : -1);
        if (robot)
        {
            if (ifIs)
                ifIs(danmaku);
        }
        else // 不是机器人
        {
            if (ifNot)
            {
                ifNot(danmaku);
            }
        }
    });
}

void MainWindow::judgeUserRobotByUpload(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs)
{
    QString url = "http://api.vc.bilibili.com/link_draw/v1/doc/upload_count?uid=" + snum(danmaku.getUid());
    get(url, [=](QJsonObject json){
        int code = json.value("code").toInt();
        if (code != 0)
        {
            if (code == 403)
                showError("机器人判断:投稿", "您没有权限");
            else
                showError("机器人判断:投稿", json.value("message").toString());
            return ;
        }
        QJsonObject obj = json.value("data").toObject();
        int allCount = obj.value("all_count").toInt();
        bool robot = (allCount <= 1); // 机器人，或者小号（1是因为有默认成为会员的相簿）
        qInfo() << "判断机器人：" << danmaku.getNickname() << "    投稿数量：" << allCount << robot;
        robotRecord->setValue("robot/" + snum(danmaku.getUid()), robot ? 1 : -1);
        if (robot)
        {
            if (ifIs)
                ifIs(danmaku);
        }
        else // 不是机器人
        {
            if (ifNot)
                ifNot(danmaku);
        }
    });
}

void MainWindow::sendWelcome(LiveDanmaku danmaku)
{
    if (us->notWelcomeUsers.contains(danmaku.getUid())
            || (!ui->sendWelcomeTextCheck->isChecked()
            && !ui->sendWelcomeVoiceCheck->isChecked())) // 不自动欢迎
        return ;
    QStringList words = cr->getEditConditionStringList(ui->autoWelcomeWordsEdit->toPlainText(), danmaku);
    if (!words.size())
    {
        if (us->debugPrint)
            localNotify("[没有可用的欢迎弹幕]");
        return ;
    }
    int r = qrand() % words.size();
    if (us->debugPrint && !(words.size() == 1 && words.first().trimmed().isEmpty()))
        localNotify("[rand " + snum(r) + " in " + snum(words.size()) + "]");
    QString msg = words.at(r);
    if (us->strongNotifyUsers.contains(danmaku.getUid()))
    {
        if (us->debugPrint)
            localNotify("[强提醒]");
        cr->sendCdMsg(msg, danmaku, 2000, NOTIFY_CD_CN,
                  ui->sendWelcomeTextCheck->isChecked(), ui->sendWelcomeVoiceCheck->isChecked(), false);
    }
    else
    {
        cr->sendCdMsg(msg, danmaku, ui->sendWelcomeCDSpin->value() * 1000, WELCOME_CD_CN,
                  ui->sendWelcomeTextCheck->isChecked(), ui->sendWelcomeVoiceCheck->isChecked(), false);
    }
}

void MainWindow::sendAttentionThans(LiveDanmaku danmaku)
{
    QStringList words = cr->getEditConditionStringList(ui->autoAttentionWordsEdit->toPlainText(), danmaku);
    if (!words.size())
    {
        if (us->debugPrint)
            localNotify("[没有可用的感谢关注弹幕]");
        return ;
    }
    int r = qrand() % words.size();
    QString msg = words.at(r);
    cr->sendAttentionMsg(msg, danmaku);
}

void MainWindow::judgeRobotAndMark(LiveDanmaku danmaku)
{
    if (!judgeRobot)
        return ;
    judgeUserRobotByFans(danmaku, nullptr, [=](LiveDanmaku danmaku){
        // 实时弹幕显示机器人
        if (danmakuWindow)
            danmakuWindow->markRobot(danmaku.getUid());
    });
}

void MainWindow::markNotRobot(qint64 uid)
{
    if (!judgeRobot)
        return ;
    int val = robotRecord->value("robot/" + snum(uid), 0).toInt();
    if (val != -1)
        robotRecord->setValue("robot/" + snum(uid), -1);
}

void MainWindow::initTTS()
{
    switch (voiceService->voicePlatform) {
    case VoiceLocal:
#if defined(ENABLE_TEXTTOSPEECH)
        if (!voiceService->tts)
        {
            qInfo() << "初始化TTS语音模块";
            voiceService->tts = new QTextToSpeech(this);
            voiceService->tts->setRate( (voiceService->voiceSpeed = us->value("voice/speed", 50).toInt() - 50) / 50.0 );
            voiceService->tts->setPitch( (voiceService->voicePitch = us->value("voice/pitch", 50).toInt() - 50) / 50.0 );
            voiceService->tts->setVolume( (voiceService->voiceVolume = us->value("voice/volume", 50).toInt()) / 100.0 );
            connect(voiceService->tts, &QTextToSpeech::stateChanged, this, [=](QTextToSpeech::State state){
                if (state == QTextToSpeech::Ready)
                    speakTextQueueNext();
            });
        }
#endif
        break;
    case VoiceXfy:
        if (!voiceService->xfyTTS)
        {
            qInfo() << "初始化讯飞语音模块";
            voiceService->xfyTTS = new XfyTTS(rt->dataPath,
                                us->value("xfytts/appid").toString(),
                                us->value("xfytts/apikey").toString(),
                                us->value("xfytts/apisecret").toString(),
                                this);
            ui->xfyAppIdEdit->setText(us->value("xfytts/appid").toString());
            ui->xfyApiKeyEdit->setText(us->value("xfytts/apikey").toString());
            ui->xfyApiSecretEdit->setText(us->value("xfytts/apisecret").toString());
            voiceService->xfyTTS->setName( voiceService->voiceName = us->value("xfytts/name", "xiaoyan").toString() );
            voiceService->xfyTTS->setPitch( voiceService->voicePitch = us->value("voice/pitch", 50).toInt() );
            voiceService->xfyTTS->setSpeed( voiceService->voiceSpeed = us->value("voice/speed", 50).toInt() );
            voiceService->xfyTTS->setVolume( voiceService->voiceSpeed = us->value("voice/speed", 50).toInt() );
        }
        break;
    case VoiceMS:
        if (!voiceService->msTTS)
        {
            qInfo() << "初始化微软语音模块";
            voiceService->msTTS = new MicrosoftTTS(rt->dataPath,
                                     us->value("mstts/areaCode").toString(),
                                     us->value("mstts/subscriptionKey").toString(),
                                     this);
            connect(voiceService->msTTS, &MicrosoftTTS::signalError, this, [=](QString err) {
                showError("微软语音", err);
            });
            ui->MSAreaCodeEdit->setText(us->value("mstts/areaCode").toString());
            ui->MSSubscriptionKeyEdit->setText(us->value("mstts/subscriptionKey").toString());
            voiceService->msTTSFormat = us->value("mstts/format", DEFAULT_MS_TTS_SSML_FORMAT).toString();
            // TODO: 设置音调等内容
        }
        break;
    case VoiceCustom:
        break;
    }
}

void MainWindow::speakText(QString text)
{
    // 处理特殊字符
    text.replace("_", " ");

    switch (voiceService->voicePlatform) {
    case VoiceLocal:
#if defined(ENABLE_TEXTTOSPEECH)
        if (!voiceService->tts)
            initTTS();
        else if (voiceService->tts->state() != QTextToSpeech::Ready)
        {
            voiceService->ttsQueue.append(text);
            return ;
        }
        voiceService->tts->say(text);
#endif
        break;
    case VoiceXfy:
        if (!voiceService->xfyTTS)
            initTTS();
        voiceService->xfyTTS->speakText(text);
        break;
    case VoiceMS:
    {
        if (!voiceService->msTTS)
            initTTS();

        // 替换文本
        if (!text.startsWith("<speak"))
        {
            QString rpl = voiceService->msTTSFormat;
            text = rpl.replace("%text%", text);
        }
        voiceService->msTTS->speakSSML(text);
    }
        break;
    case VoiceCustom:
        if (voiceService->ttsDownloading || (voiceService->ttsPlayer && voiceService->ttsPlayer->state() == QMediaPlayer::State::PlayingState))
        {
            voiceService->ttsQueue.append(text);
        }
        else
        {
            voiceDownloadAndSpeak(text);
        }
        break;
    }
}

void MainWindow::speakTextQueueNext()
{
    if (!voiceService->ttsQueue.size())
        return ;
    QString text = voiceService->ttsQueue.takeFirst();
    speakText(text);
}

/**
 * 快速设置自定义语音的类型
 */
void MainWindow::on_setCustomVoiceButton_clicked()
{
    QStringList names{"测试",
                     "百度语音"};
    QStringList urls{
        "http://120.24.87.124/cgi-bin/ekho2.pl?cmd=SPEAK&voice=EkhoMandarin&speedDelta=0&pitchDelta=0&volumeDelta=0&text=%url_text%",
        "https://ai.baidu.com/aidemo?type=tns&per=4119&spd=6&pit=5&vol=5&aue=3&tex=%url_text%"
    };
    QStringList descs{
        "",
        "spd：语速，0~15，默认为5中语速\n\
pit：音调，0~15，默认为5中音调\n\
vol：音量，0~15，默认为5中音量（取0时为音量最小值，并非静音）\n\
per：（基础）0 度小美，1 度小宇，3 度逍遥，4 度丫丫，4115 度小贤，4105 度灵儿，4117 度小乔，4100 度小雯\n\
per：（精品）5003 度逍遥，5118 度小鹿，106 度博文，110 度小童，111 度小萌，103 度米朵，5 度小娇\n\
aue：3为mp3（必须）"
    };

    newFacileMenu;
    for (int i = 0; i < names.size(); i++)
    {
        menu->addAction(names.at(i), [=]{
            ui->voiceCustomUrlEdit->setText(urls.at(i));
            on_voiceCustomUrlEdit_editingFinished();
        })->tooltip(descs.at(i));
    }
    menu->addTitle("鼠标悬浮可查看参数");
    menu->exec();
}


/**
 * 针对返回是JSON/音频字节的类型
 * 自动判断
 */
void MainWindow::voiceDownloadAndSpeak(QString text)
{
    QString url = ui->voiceCustomUrlEdit->text();
    if (url.isEmpty())
        return ;
    url = url.replace("%1", text).replace("%text%", text);
    url = url.replace("%url_text%", QString::fromUtf8(text.toUtf8().toPercentEncoding()));
    playNetAudio(url);
}

void MainWindow::playNetAudio(QString url)
{
    qInfo() << "播放网络音频：" << url;
    const QString filePath = rt->dataPath + "tts";
    QDir dir(filePath);
    dir.mkpath(filePath);

    voiceService->ttsDownloading = true;
    get(url, [=](QNetworkReply* reply1){
        voiceService->ttsDownloading = false;
        QByteArray fileData;

        if (reply1->error() != QNetworkReply::NoError)
        {
            qWarning() << "获取网络音频错误：" << reply1->errorString();
            return ;
        }

        auto contentType = reply1->header(QNetworkRequest::ContentTypeHeader).toString();
        if (contentType.contains("json"))
        {
            // https://ai.baidu.com/aidemo?type=tns&per=4119&spd=6&pit=5&vol=5&aue=3&tex=这是一个测试文本
            QByteArray ba = reply1->readAll();
            MyJson json(ba);
            QString s = json.s("data");
            if (s.startsWith("data:audio") && s.contains(","))
            {
                QStringList sl = s.split(",");
                fileData = sl.at(1).toUtf8();
                if (sl.at(0).contains("base64"))
                    fileData = QByteArray::fromBase64(fileData);
            }
            else
            {
                qWarning() << "无法解析的语音返回：" << json;
                showError("在线音频", json.errOrMsg());
                return ;
            }
        }
        else
        {
            // http://120.24.87.124/cgi-bin/ekho2.pl?cmd=SPEAK&voice=EkhoMandarin&speedDelta=0&pitchDelta=0&volumeDelta=0&text=这是一个测试文本
            fileData = reply1->readAll();
        }

        if (fileData.isEmpty())
        {
            qWarning() << "网络音频为空";
            return ;
        }

        // 保存文件
        QString path = filePath + "/" + snum(QDateTime::currentMSecsSinceEpoch()) + ".mp3";
        QFile file(path);
        file.open(QFile::WriteOnly);
        QDataStream stream(&file);
        stream << fileData;
        file.flush();
        file.close();

        // 播放文件
        if (!voiceService->ttsPlayer)
        {
            voiceService->ttsPlayer = new QMediaPlayer(this);
            connect(voiceService->ttsPlayer, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state){
                if (state == QMediaPlayer::StoppedState)
                {
                    QFile file(path);
                    file.remove();
                    speakTextQueueNext();
                }
            });
        }
        voiceService->ttsPlayer->setMedia(QUrl::fromLocalFile(path));

        voiceService->ttsPlayer->play();
    });
}

void MainWindow::showScreenDanmaku(const LiveDanmaku &danmaku)
{
    if (!ui->enableScreenDanmakuCheck->isChecked()) // 不显示所有弹幕
        return ;
    if (!ui->enableScreenMsgCheck->isChecked() && danmaku.getMsgType() != MSG_DANMAKU) // 不显示所有msg
        return ;
    if (_loadingOldDanmakus) // 正在加载旧弹幕
        return ;
    if (danmaku.isPkLink()) // 对面同步过来的弹幕
        return ;

    // 显示动画
    QLabel* label = new QLabel(nullptr);
    label->setWindowFlag(Qt::FramelessWindowHint, true);
    label->setWindowFlag(Qt::Tool, true);
    label->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    label->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明
    label->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    QString disp;
    if (danmaku.getMsgType() == MSG_DANMAKU || !danmaku.getText().isEmpty())
    {
        disp = danmaku.getText();
        if (ui->screenDanmakuWithNameCheck->isChecked())
            disp = danmaku.getNickname() + "：" + disp;
    }
    else
    {
        QString text = danmaku.toString();
        QRegularExpression re("^\\d+:\\d+:\\d+\\s+(.+)$"); // 简化表达式，去掉时间信息
        QRegularExpressionMatch match;
        if (text.indexOf(re, 0, &match) > -1)
            text = match.capturedTexts().at(1);
        disp = text;
    }
    label->setText(disp);
    label->setFont(screenDanmakuFont);

    auto isBlankColor = [=](QString c) -> bool {
        c = c.toLower();
        return c.isEmpty() || c == "#ffffff" || c == "#000000"
                || c == "#ffffffff" || c == "#00000000";
    };
    if (!danmaku.getTextColor().isEmpty() && !isBlankColor(danmaku.getTextColor()))
        label->setStyleSheet("color:" + danmaku.getTextColor() + ";");
    else
        label->setStyleSheet("color:" + QVariant(screenDanmakuColor).toString() + ";");
    label->adjustSize();
    label->show();

    QRect rect = getScreenRect();
    int left = rect.left() + rect.width() * ui->screenDanmakuLeftSpin->value() / 100;
    int right = rect.right() - rect.width() * ui->screenDanmakuRightSpin->value() / 100;
    int top = rect.top() + rect.height() * ui->screenDanmakuTopSpin->value() / 100;
    int bottom = rect.bottom() - rect.height() * ui->screenDanmakuBottomSpin->value() / 100;
    int sx = right, ex = left - label->width();
    if (left > right) // 从左往右
    {
        sx = right - label->width();
        ex = left;
    }

    // 避免重叠
    auto isOverlap = [=](int x, int y, int height){
        foreach (QLabel* label, screenLabels)
        {
            if (label->geometry().contains(x, y)
                    || label->geometry().contains(x, y+height))
                return true;
        }
        return false;
    };
    int forCount = 0;
    int y = 0;
    do {
        int ry = qrand() % (qMax(30, qAbs(bottom - top) - label->height()));
        y = qMin(top, bottom) + label->height() + ry;
        if (++forCount >= 32) // 最多循环32遍（如果弹幕极多，可能会必定叠在一起）
            break;
    } while (isOverlap(sx, y, label->height()));

    label->move(QPoint(sx, y));
    screenLabels.append(label);
    QPropertyAnimation* ani = new QPropertyAnimation(label, "pos");
    ani->setStartValue(QPoint(sx, y));
    ani->setEndValue(QPoint(ex, y));
    ani->setDuration(ui->screenDanmakuSpeedSpin->value() * 1000);
    connect(ani, &QPropertyAnimation::finished, ani, [=]{
        ani->deleteLater();
        screenLabels.removeOne(label);
        label->deleteLater();
    });
    ani->start();
}

/**
 * 合并消息
 * 在添加到消息队列前调用此函数判断
 * 若是，则同步合并实时弹幕中的礼物连击
 */
bool MainWindow::mergeGiftCombo(const LiveDanmaku &danmaku)
{
    if (danmaku.getMsgType() != MSG_GIFT)
        return false;

    // 判断，同人 && 礼物同名 && x秒内
    qint64 uid = danmaku.getUid();
    int giftId = danmaku.getGiftId();
    qint64 time = danmaku.getTimeline().toSecsSinceEpoch();
    LiveDanmaku* merged = nullptr;
    int delayTime = ui->giftComboDelaySpin->value(); // + 1; // 多出的1秒当做网络延迟了

    // 遍历房间弹幕
    for (int i = liveService->roomDanmakus.size()-1; i >= 0; i--)
    {
        LiveDanmaku dm = liveService->roomDanmakus.at(i);
        qint64 t = dm.getTimeline().toSecsSinceEpoch();
        if (t == 0) // 有些是没带时间的
            continue;
        if (t + delayTime < time) // x秒以内
            return false;
        if (dm.getMsgType() != MSG_GIFT
                || dm.getUid() != uid
                || dm.getGiftId() != giftId)
            continue;

        // 是这个没错了
        merged = &liveService->roomDanmakus[i];
        break;
    }
    if (!merged)
        return false;

    // 开始合并
    qInfo() << "合并相同礼物至：" << merged->toString();
    merged->addGift(danmaku.getNumber(), danmaku.getTotalCoin(), danmaku.getTimeline());

    // 合并实时弹幕
    if (danmakuWindow)
        danmakuWindow->mergeGift(danmaku, delayTime);

    return true;
}



void MainWindow::userComeEvent(LiveDanmaku &danmaku)
{
    qint64 uid = danmaku.getUid();

    // [%come_time% > %timestamp%-3600]*%ai_name%，你回来了~ // 一小时内
    // [%come_time%>0, %come_time%<%timestamp%-3600*24]*%ai_name%，你终于来喽！
    int userCome = us->danmakuCounts->value("come/" + snum(uid)).toInt();
    danmaku.setNumber(userCome);
    danmaku.setPrevTimestamp(us->danmakuCounts->value("comeTime/"+snum(uid), 0).toLongLong());

    appendNewLiveDanmaku(danmaku);

    userCome++;
    us->danmakuCounts->setValue("come/"+snum(uid), userCome);
    us->danmakuCounts->setValue("comeTime/"+snum(uid), danmaku.getTimeline().toSecsSinceEpoch());

    liveService->dailyCome++;
    if (liveService->dailySettings)
        liveService->dailySettings->setValue("come", liveService->dailyCome);
    if (danmaku.isOpposite())
    {
        // 加到自己这边来，免得下次误杀（即只提醒一次）
        // 不过不能这么做，否则不会显示“对面”两个字了
        // myAudience.insert(uid);
    }
    else if (liveService->cmAudience.contains(uid))
    {
        if (liveService->cmAudience.value(uid) > 0)
        {
            danmaku.setViewReturn(true);
            danmaku.setPrevTimestamp(liveService->cmAudience[uid]);
            liveService->cmAudience[uid] = 0; // 标记为串门回来
            localNotify(danmaku.getNickname() + " 去对面串门回来");
        }
    }

    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    if (!justStart && ui->autoSendWelcomeCheck->isChecked()) // 发送欢迎
    {
        us->userComeTimes[uid] = currentTime;
        sendWelcomeIfNotRobot(danmaku);
    }
    else // 不发送欢迎，只是查看
    {
        us->userComeTimes[uid] = currentTime; // 直接更新了
        if (judgeRobot == 2)
        {
            judgeRobotAndMark(danmaku);
        }
    }
}

bool MainWindow::isInFans(qint64 uid)
{
    foreach (auto fan, liveService->fansList)
    {
        if (fan.mid == uid)
            return true;
    }
    return false;
}

void MainWindow::sendGift(int giftId, int giftNum)
{
    if (ac->roomId.isEmpty() || ac->browserCookie.isEmpty())
    {
        qWarning() << "房间为空，或未登录";
        return ;
    }

    if (us->localMode)
    {
        localNotify("赠送礼物 -> " + snum(giftId) + " x " + snum(giftNum));
        return ;
    }


    // 设置数据（JSON的ByteArray）
    QStringList datas;
    datas << "uid=" + ac->cookieUid;
    datas << "gift_id=" + snum(giftId);
    datas << "ruid=" + ac->upUid;
    datas << "send_ruid=0";
    datas << "gift_num=" + snum(giftNum);
    datas << "coin_type=gold";
    datas << "bag_id=0";
    datas << "platform=pc";
    datas << "biz_code=live";
    datas << "biz_id=" + ac->roomId;
    datas << "rnd=" + snum(QDateTime::currentSecsSinceEpoch());
    datas << "storm_beat_id=0";
    datas << "metadata=";
    datas << "price=0";
    datas << "csrf_token=" + ac->csrf_token;
    datas << "csrf=" + ac->csrf_token;
    datas << "visit_id=";

    QByteArray ba(datas.join("&").toStdString().data());

    QString url("https://api.live.bilibili.com/gift/v2/Live/send");
    post(url, ba, [=](QJsonObject json){
        QString message = json.value("message").toString();
        statusLabel->setText("");
        if (message != "success")
        {
            showError("赠送礼物", message);
        }
    });
}

void MainWindow::sendBagGift(int giftId, int giftNum, qint64 bagId)
{
    if (ac->roomId.isEmpty() || ac->browserCookie.isEmpty())
    {
        qWarning() << "房间为空，或未登录";
        return ;
    }

    if (us->localMode)
    {
        localNotify("赠送包裹礼物 -> " + snum(giftId) + " x " + snum(giftNum));
        return ;
    }

    // 设置数据（JSON的ByteArray）
    QStringList datas;
    datas << "uid=" + ac->cookieUid;
    datas << "gift_id=" + snum(giftId);
    datas << "ruid=" + ac->upUid;
    datas << "send_ruid=0";
    datas << "gift_num=" + snum(giftNum);
    datas << "bag_id=" + snum(bagId);
    datas << "platform=pc";
    datas << "biz_code=live";
    datas << "biz_id=" + ac->roomId;
    datas << "rnd=" + snum(QDateTime::currentSecsSinceEpoch());
    datas << "storm_beat_id=0";
    datas << "metadata=";
    datas << "price=0";
    datas << "csrf_token=" + ac->csrf_token;
    datas << "csrf=" + ac->csrf_token;
    datas << "visit_id=";

    QByteArray ba(datas.join("&").toStdString().data());

    QString url("https://api.live.bilibili.com/gift/v2/live/bag_send");
    post(url, ba, [=](QJsonObject json){
        QString message = json.value("message").toString();
        statusLabel->setText("");
        if (message != "success")
        {
            showError("赠送包裹礼物失败", message);
            qCritical() << s8("warning: 发送失败：") << message << datas.join("&");
        }
    });
}

void MainWindow::getRoomLiveVideoUrl(StringFunc func)
{
    if (ac->roomId.isEmpty())
        return ;
    /*QString url = "https://api.live.bilibili.com/room/v1/Room/playUrl?cid=" + ac->roomId
            + "&quality=4&qn=10000&platform=web&otype=json";
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取视频URL返回结果不为0：") << json.value("message").toString();
            return ;
        }

        // 获取链接
        QJsonArray array = json.value("data").toObject().value("durl").toArray();
        /*"url": "http://d1--cn-gotcha04.bilivideo.com/live-bvc/521719/live_688893202_7694436.flv?cdn=cn-gotcha04\\u0026expires=1607505058\\u0026len=0\\u0026oi=1944862322\\u0026pt=\\u0026qn=10000\\u0026trid=40938d869aca4cb39041730f74ff9051\\u0026sigparams=cdn,expires,len,oi,pt,qn,trid\\u0026sign=ac701ba60c346bf3a173e56bcb14b7b9\\u0026ptype=0\\u0026src=9\\u0026sl=1\\u0026order=1",
          "length": 0,
          "order": 1,
          "stream_type": 0,
          "p2p_type": 0*-/
        QString url = array.first().toObject().value("url").toString();
        if (func)
            func(url);
    });*/

    QString full_url = "https://api.live.bilibili.com/xlive/web-room/v2/index/getRoomPlayInfo?room_id=" + ac->roomId
            + "&no_playurl=0&mask=1&qn=10000&platform=web&protocol=0,1&format=0,1,2&codec=0,1&dolby=5&panorama=1";
    get(full_url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qWarning() << ("返回结果不为0：") << json.value("message").toString();
            return ;
        }

        QJsonObject play_url = json.value("data").toObject().value("playurl_info").toObject().value("playurl").toObject();
        QJsonArray streams = play_url.value("stream").toArray();
        QString url;

        // 先抓取m3u8的视频
        foreach (auto _stream, streams)
        {
            auto stream = _stream.toObject();
            // m3u8:http_hls, flv:http_stream
            if (stream.value("protocol_name").toString() != "http_hls")
                continue;

            auto formats = stream.value("format").toArray(); // 这里有多个可播放的格式
            foreach (auto _format, formats)
            {
                auto format = _format.toObject();
                auto codecs = format.value("codec").toArray();
                foreach (auto _codec, codecs)
                {
                    auto codec = _codec.toObject();
                    QString codec_name = codec.value("codec_name").toString();
                    QString base_url = codec.value("base_url").toString();
                    auto url_infos = codec.value("url_info").toArray();
                    foreach (auto _url_info, url_infos)
                    {
                        auto url_info = _url_info.toObject();
                        QString host = url_info.value("host").toString();
                        QString extra = url_info.value("extra").toString();
                        qint64 stream_ttl = url_info.value("stream_ttl").toInt();

                        url = host + base_url;

                        if (!url.isEmpty())
                            break;
                    }
                    if (!url.isEmpty())
                        break;
                }
                if (!url.isEmpty())
                    break;
            }
            if (!url.isEmpty())
                break;
        }

        if (url.isEmpty())
        {
            qWarning() << "录播无法获取下载地址：" << full_url;
            return ;
        }
        if (url.endsWith("?"))
            url = url.left(url.length() - 1);
        if (func)
            func(url);
    });
}

/**
 * 触发进入直播间事件
 * 偶然看到的，未经测试
 */
void MainWindow::roomEntryAction()
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/roomEntryAction?room_id="
            + ac->roomId + "&platform=pc&csrf_token=" + ac->csrf_token + "&csrf=" + ac->csrf_token + "&visit_id=";
    post(url, QByteArray(), [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("进入直播间返回结果不为0：") << json.value("message").toString();
            return ;
        }
    });
}

void MainWindow::sendExpireGift()
{
    getBagList(24 * 3600 * 2); // 默认赠送两天内过期的
}

void MainWindow::getBagList(qint64 sendExpire)
{
    if (ac->roomId.isEmpty() || ac->browserCookie.isEmpty())
    {
        qWarning() << "房间为空，或未登录";
        return ;
    }
    /*{
        "code": 0,
        "message": "0",
        "ttl": 1,
        "data": {
            "list": [
                {
                    "bag_id": 232151929,
                    "gift_id": 30607,
                    "gift_name": "小心心",
                    "gift_num": 12,
                    "gift_type": 5,             // 0 = 普通, 6 = 礼盒, 2 = 加分卡
                    "expire_at": 1612972800,
                    "corner_mark": "4天",
                    "corner_color": "",
                    "count_map": [
                        {
                            "num": 1,
                            "text": ""
                        },
                        {
                            "num": 12,
                            "text": "全部"
                        }
                    ],
                    "bind_roomid": 0,
                    "bind_room_text": "",
                    "type": 1,
                    "card_image": "",
                    "card_gif": "",
                    "card_id": 0,
                    "card_record_id": 0,
                    "is_show_send": false
                },
                ......
    }*/

    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/gift/bag_list?t=1612663775421&room_id=" + ac->roomId;
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取包裹返回结果不为0：") << json.value("message").toString();
            return ;
        }

        QJsonArray bagArray = json.value("data").toObject().value("list").toArray();

        if (true)
        {
            foreach (QJsonValue val, bagArray)
            {
                QJsonObject bag = val.toObject();
                // 赠送礼物
                QString giftName = bag.value("gift_name").toString();
                int giftId = bag.value("gift_id").toInt();
                int giftNum = bag.value("gift_num").toInt();
                qint64 bagId = qint64(bag.value("bag_id").toDouble());
                QString cornerMark = bag.value("corner_mark").toString();
                // qInfo() << "当前礼物：" << giftName << "×" << giftNum << cornerMark << giftId <<bagId;
            }
        }

        if (sendExpire) // 赠送过期礼物
        {
            QList<int> whiteList{1, 6, 30607}; // 辣条、亿圆、小心心

            qint64 timestamp = QDateTime::currentSecsSinceEpoch();
            qint64 expireTs = timestamp + sendExpire; // 超过这个时间的都送
            foreach (QJsonValue val, bagArray)
            {
                QJsonObject bag = val.toObject();
                qint64 expire = qint64(bag.value("expire_at").toDouble());
                int giftId = bag.value("gift_id").toInt();
                if (!whiteList.contains(giftId) || expire == 0 || expire > expireTs)
                    continue ;

                // 赠送礼物
                QString giftName = bag.value("gift_name").toString();
                int giftNum = bag.value("gift_num").toInt();
                qint64 bagId = qint64(bag.value("bag_id").toDouble());
                QString cornerMark = bag.value("corner_mark").toString();
                qInfo() << "赠送过期礼物：" << giftId << giftName << "×" << giftNum << cornerMark << (expire-timestamp)/3600 << "小时";
                sendBagGift(giftId, giftNum, bagId);
            }
        }
    });
}

void MainWindow::updateOnlineRankGUI()
{
    if (us->closeGui)
        return ;

    // 获取头像
    for (int i = 0; i < liveService->onlineGoldRank.size(); i++)
    {
        const LiveDanmaku &danmaku = liveService->onlineGoldRank.at(i);
        qint64 uid = danmaku.getUid();
        if (!pl->userHeaders.contains(uid))
        {
            QString url = danmaku.extraJson.value("face").toString();
            // qInfo() << "获取头像：" << uid << url;
            get(url, [=](QNetworkReply* reply){
                if (pl->userHeaders.contains(uid)) // 可能是多线程冲突了
                    return ;
                QByteArray jpegData = reply->readAll();
                QPixmap pixmap;
                pixmap.loadFromData(jpegData);
                if (pixmap.isNull())
                {
                    showError("获取头像图片出错");
                    qWarning() << "头像地址：" << url;
                    pixmap = QPixmap(":/icons/header1");
                }

                pl->userHeaders[uid] = pixmap;
                updateOnlineRankGUI();
            });
            return ;
        }
    }

    // 找到不同点，提升性能（前几应该都不变的）
    int diff = 0;
    for (diff = 0; diff < liveService->onlineGoldRank.size() && diff < ui->onlineRankListWidget->count(); diff++)
    {
        qint64 id1 = liveService->onlineGoldRank.at(diff).getUid();
        qint64 id2 = ui->onlineRankListWidget->item(diff)->data(Qt::UserRole).toLongLong();
        if (id1 != id2)
            break;
    }
    while (ui->onlineRankListWidget->count() > diff)
    {
        auto item = ui->onlineRankListWidget->item(diff);
        auto widget = ui->onlineRankListWidget->itemWidget(item);
        ui->onlineRankListWidget->takeItem(diff);
        delete item;
        widget->deleteLater();
    }

    // 放到GUI
    const int headerRadius = 24;
    int miniHeight = 0;
    for (int i = diff; i < liveService->onlineGoldRank.size(); i++)
    {
        auto& danmaku = liveService->onlineGoldRank.at(i);
        qint64 uid = danmaku.getUid();
        if (pl->userHeaders.contains(uid))
        {
            // 创建widget
            InteractiveButtonBase* card = new InteractiveButtonBase(ui->onlineRankListWidget);
            card->show();
            card->setRadius(rt->fluentRadius);
            // card->setCursor(Qt::PointingHandCursor);

            RoundedPixmapLabel* imgLabel = new RoundedPixmapLabel(card);
            imgLabel->setRadius(headerRadius);
            imgLabel->setFixedSize(headerRadius * 2, headerRadius * 2);
            imgLabel->show();
            imgLabel->setPixmap(pl->userHeaders.value(uid));

            QLabel* unameLabel = new QLabel(danmaku.getNickname(), card);
            unameLabel->show();
            // unameLabel->setFixedWidth(unameLabel->sizeHint().width());
            unameLabel->setAlignment(Qt::AlignHCenter);
            QLabel* scoreLabel = new QLabel(snum(danmaku.getTotalCoin()), card);
            scoreLabel->show();
            scoreLabel->setAlignment(Qt::AlignHCenter);
            unameLabel->setToolTip(danmaku.getNickname() + "  " + snum(danmaku.getTotalCoin()));

            QVBoxLayout* vlayout = new QVBoxLayout(card);
            vlayout->setAlignment(Qt::AlignHCenter);
            vlayout->addWidget(imgLabel);
            vlayout->addWidget(unameLabel);
            vlayout->addWidget(scoreLabel);
            vlayout->setSpacing(vlayout->spacing() / 2);
            // vlayout->setMargin(vlayout->margin() / 2);
            card->setLayout(vlayout);
            card->setFixedSize(imgLabel->width() + vlayout->margin() * 2,
                               imgLabel->height() + unameLabel->height() + scoreLabel->height() + vlayout->spacing()*2 + vlayout->margin() * 2);

            if (unameLabel->sizeHint().width() > imgLabel->width()) // 超出长度的
                unameLabel->setAlignment(Qt::AlignLeft);
            if (scoreLabel->sizeHint().width() > imgLabel->width())
                scoreLabel->setAlignment(Qt::AlignLeft);
            if (!miniHeight)
                miniHeight = card->height();

            // 创建item
            auto item = new QListWidgetItem();
            item->setData(Qt::UserRole, uid);
            ui->onlineRankListWidget->addItem(item);
            item->setSizeHint(card->size());
            ui->onlineRankListWidget->setItemWidget(item, card);

            // 点击事件
            connect(card, &InteractiveButtonBase::clicked, card, [=]{
                // TODO: 点击查看信息
            });
            connect(card, &InteractiveButtonBase::rightClicked, card, [=]{
                newFacileMenu;
                menu->addAction("打开首页", [=]{
                    QDesktopServices::openUrl(QUrl("https://space.bilibili.com/" + snum(uid)));
                });
                menu->exec();
            });
        }
    }
    if (miniHeight)
        ui->onlineRankListWidget->setFixedHeight(miniHeight);
    // ui->onlineRankDescLabel->setText(onlineGoldRank.size() ? "高\n能\n榜" : "");
    ui->onlineRankDescLabel->setPixmap(liveService->onlineGoldRank.size() ? QPixmap(":/icons/rank") : QPixmap());
    ui->onlineRankDescLabel->setMaximumSize(headerRadius * 4 / 3, headerRadius * 2);
}

/**
 * 添加本次直播的金瓜子礼物
 */
void MainWindow::appendLiveGift(const LiveDanmaku &danmaku)
{
    if (danmaku.getTotalCoin() == 0)
    {
        qWarning() << "添加礼物到liveGift错误：" << danmaku.toString();
        return ;
    }
    for (int i = 0; i < liveService->liveAllGifts.size(); i++)
    {
        auto his = liveService->liveAllGifts.at(i);
        if (his.getUid() == danmaku.getUid()
                && his.getGiftId() == danmaku.getGiftId())
        {
            liveService->liveAllGifts[i].addGift(danmaku.getNumber(), danmaku.getTotalCoin(), danmaku.getTimeline());
            return ;
        }
    }

    // 新建一个
    liveService->liveAllGifts.append(danmaku);
}

void MainWindow::appendLiveGuard(const LiveDanmaku &danmaku)
{
    if (!danmaku.is(MSG_GUARD_BUY))
    {
        qWarning() << "添加上船到liveGuard错误：" << danmaku.toString();
        return ;
    }
    for (int i = 0; i < liveService->liveAllGuards.size(); i++)
    {
        auto his = liveService->liveAllGuards.at(i);
        if (his.getUid() == danmaku.getUid()
                && his.getGiftId() == danmaku.getGiftId())
        {
            liveService->liveAllGuards[i].addGift(danmaku.getNumber(), danmaku.getTotalCoin(), danmaku.getTimeline());
            return ;
        }
    }

    // 新建一个
    liveService->liveAllGuards.append(danmaku);
}

void MainWindow::getPkMatchInfo()
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByRoom?room_id=" + liveService->pkRoomId;
    get(url, [=](MyJson json) {
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取PK匹配返回结果不为0：") << json.value("message").toString();
            return ;
        }

        MyJson data = json.data();
        LiveDanmaku danmaku;
        danmaku.with(data);

        // 计算高能榜综合
        qint64 sum = 0;
        data.o("online_gold_rank_info_v2").each("list", [&](MyJson usr){
            sum += usr.s("score").toLongLong(); // 这是String类型， 不是int
        });
        danmaku.setTotalCoin(sum); // 高能榜总积分
        danmaku.setNumber(data.o("online_gold_rank_info_v2").a("list").size()); // 高能榜总人数
        qInfo() << "对面高能榜积分总和：" << sum;
        // qDebug() << data.o("online_gold_rank_info_v2").a("list");
        triggerCmdEvent("PK_MATCH_INFO", danmaku, true);
    });
}

void MainWindow::getPkOnlineGuardPage(int page)
{
    static int guard1 = 0, guard2 = 0, guard3 = 0;
    if (page == 0)
    {
        page = 1;
        guard1 = guard2 = guard3 = 0;
    }

    auto addCount = [=](MyJson user) {
        int alive = user.i("is_alive");
        if (!alive)
            return ;
        QString username = user.s("username");
        int guard_level = user.i("guard_level");
        if (!alive)
            return ;
        if (guard_level == 1)
            guard1++;
        else if (guard_level == 2)
            guard2++;
        else
            guard3++;
    };

    QString url = "https://api.live.bilibili.com/xlive/app-room/v2/guardTab/topList?actionKey=appkey&appkey=27eb53fc9058f8c3&roomid=" + liveService->pkRoomId
            +"&page=" + QString::number(page) + "&ruid=" + liveService->pkUid + "&page_size=30";
    get(url, [=](MyJson json){
        MyJson data = json.data();
        // top3
        if (page == 1)
        {
            QJsonArray top3 = data.value("top3").toArray();
            foreach (QJsonValue val, top3)
                addCount(val.toObject());
        }

        // list
        QJsonArray list = data.value("list").toArray();
        foreach (QJsonValue val, list)
            addCount(val.toObject());

        // 下一页
        QJsonObject info = data.value("info").toObject();
        int page = info.value("page").toInt();
        int now = info.value("now").toInt();
        if (now < page)
            getPkOnlineGuardPage(now+1);
        else // 全部完成了
        {
            qInfo() << "舰长数量：" << guard1 << guard2 << guard3;
            LiveDanmaku danmaku;
            danmaku.setNumber(guard1 + guard2 + guard3);
            danmaku.extraJson.insert("guard1", guard1);
            danmaku.extraJson.insert("guard2", guard2);
            danmaku.extraJson.insert("guard3", guard3);
            triggerCmdEvent("PK_MATCH_ONLINE_GUARD", danmaku, true);
        }
    });
}

void MainWindow::setRoomDescription(QString roomDescription)
{
    ui->roomDescriptionBrowser->setText(roomDescription);
    QString text = ui->roomDescriptionBrowser->toPlainText();
    QTextCursor cursor = ui->roomDescriptionBrowser->textCursor();

    // 删除前面空白
    int count = 0;
    int len = text.length();
    while (count < len && QString("\n\r ").contains(QString(text.mid(count, 1))))
        count++;
    if (count)
    {
        cursor.setPosition(0);
        cursor.setPosition(count, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    // 删除后面空白
    text = ui->roomDescriptionBrowser->toPlainText();
    count = 0;
    len = text.length();
    while (count < len && QString("\n\r ").contains(QString(text.mid(len - count - 1, 1))))
        count++;
    if (count)
    {
        cursor.setPosition(len);
        cursor.setPosition(len - count, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    if (ui->roomDescriptionBrowser->toPlainText().isEmpty())
        ui->roomDescriptionBrowser->hide();
    else
        ui->roomDescriptionBrowser->show();
}

void MainWindow::updateWinningStreak(bool emitWinningStreak)
{

}

void MainWindow::on_autoSendWelcomeCheck_stateChanged(int arg1)
{
    us->setValue("danmaku/sendWelcome", ui->autoSendWelcomeCheck->isChecked());
    ui->sendWelcomeTextCheck->setEnabled(arg1);
    ui->sendWelcomeVoiceCheck->setEnabled(arg1);
}

void MainWindow::on_autoSendGiftCheck_stateChanged(int arg1)
{
    us->setValue("danmaku/sendGift", ui->autoSendGiftCheck->isChecked());
    ui->sendGiftTextCheck->setEnabled(arg1);
    ui->sendGiftVoiceCheck->setEnabled(arg1);
}

void MainWindow::on_autoWelcomeWordsEdit_textChanged()
{
    us->setValue("danmaku/autoWelcomeWords", ui->autoWelcomeWordsEdit->toPlainText());
}

void MainWindow::on_autoThankWordsEdit_textChanged()
{
    us->setValue("danmaku/autoThankWords", ui->autoThankWordsEdit->toPlainText());
}

void MainWindow::on_startLiveWordsEdit_editingFinished()
{
    us->setValue("live/startWords", ui->startLiveWordsEdit->text());
}

void MainWindow::on_endLiveWordsEdit_editingFinished()
{
    us->setValue("live/endWords", ui->endLiveWordsEdit->text());
}

void MainWindow::on_startLiveSendCheck_stateChanged(int arg1)
{
    us->setValue("live/startSend", ui->startLiveSendCheck->isChecked());
}

void MainWindow::on_autoSendAttentionCheck_stateChanged(int arg1)
{
    us->setValue("danmaku/sendAttention", ui->autoSendAttentionCheck->isChecked());
    ui->sendAttentionTextCheck->setEnabled(arg1);
    ui->sendAttentionVoiceCheck->setEnabled(arg1);
}

void MainWindow::on_autoAttentionWordsEdit_textChanged()
{
    us->setValue("danmaku/autoAttentionWords", ui->autoAttentionWordsEdit->toPlainText());
}

void MainWindow::on_sendWelcomeCDSpin_valueChanged(int arg1)
{
    us->setValue("danmaku/sendWelcomeCD", arg1);
}

void MainWindow::on_sendGiftCDSpin_valueChanged(int arg1)
{
    us->setValue("danmaku/sendGiftCD", arg1);
}

void MainWindow::on_sendAttentionCDSpin_valueChanged(int arg1)
{
    us->setValue("danmaku/sendAttentionCD", arg1);
}

void MainWindow::showDiangeHistory()
{
    QStringList list;
    int first = qMax(0, diangeHistory.size() - 10);
    for (int i = diangeHistory.size()-1; i >= first; i--)
    {
        Diange dg = diangeHistory.at(i);
        list << dg.name + "  -  " + dg.nickname + "  " + dg.time.toString("hh:mm:ss");
    }
    QString text = list.size() ? list.join("\n") : "没有点歌记录";
    QMessageBox::information(this, "点歌历史", text);
}

void MainWindow::eternalBlockUser(qint64 uid, QString uname, QString msg)
{
    if (us->eternalBlockUsers.contains(EternalBlockUser(uid, ac->roomId.toLongLong(), msg)))
    {
        localNotify("该用户已经在永久禁言中");
        return ;
    }

    liveService->addBlockUser(uid, 720, msg);

    us->eternalBlockUsers.append(EternalBlockUser(uid, ac->roomId.toLongLong(), uname, ac->upName, ac->roomTitle, QDateTime::currentSecsSinceEpoch(), msg));
    saveEternalBlockUsers();
    qInfo() << "添加永久禁言：" << uname << "    当前人数：" << us->eternalBlockUsers.size();
}

void MainWindow::cancelEternalBlockUser(qint64 uid)
{
    cancelEternalBlockUser(uid, ac->roomId.toLongLong());
}

void MainWindow::cancelEternalBlockUser(qint64 uid, qint64 roomId)
{
    EternalBlockUser user(uid, roomId, "");
    if (!us->eternalBlockUsers.contains(user))
        return ;

    us->eternalBlockUsers.removeOne(user);
    saveEternalBlockUsers();
    qInfo() << "移除永久禁言：" << uid << "    当前人数：" << us->eternalBlockUsers.size();
}

void MainWindow::cancelEternalBlockUserAndUnblock(qint64 uid)
{
    cancelEternalBlockUserAndUnblock(uid, ac->roomId.toLongLong());
}

void MainWindow::cancelEternalBlockUserAndUnblock(qint64 uid, qint64 roomId)
{
    cancelEternalBlockUser(uid, roomId);

    liveService->delBlockUser(uid, snum(roomId));
}

void MainWindow::saveEternalBlockUsers()
{
    QJsonArray array;
    int size = us->eternalBlockUsers.size();
    for (int i = 0; i < size; i++)
        array.append(us->eternalBlockUsers.at(i).toJson());
    us->setValue("danmaku/eternalBlockUsers", array);
    qInfo() << "保存永久禁言，当前人数：" << us->eternalBlockUsers.size();
}

/**
 * 检测需要重新禁言的用户，越前面时间越早
 */
void MainWindow::detectEternalBlockUsers()
{
    qint64 currentSecond = QDateTime::currentSecsSinceEpoch();
    const int MAX_BLOCK_HOUR = 720;
    qint64 maxBlockSecond = MAX_BLOCK_HOUR * 3600;
    const int netDelay = 5; // 5秒的屏蔽时长
    bool blocked = false;
    for (int i = 0; i < us->eternalBlockUsers.size(); i++)
    {
        EternalBlockUser user = us->eternalBlockUsers.first();
        if (user.time + maxBlockSecond + netDelay >= currentSecond) // 仍在冷却中
            break;

        // 该补上禁言啦
        blocked = true;
        qInfo() << "永久禁言：重新禁言用户" << user.uid << user.uname << user.time << "->" << currentSecond;
        liveService->addBlockUser(user.uid, snum(user.roomId), MAX_BLOCK_HOUR, user.msg);

        user.time = currentSecond;
        us->eternalBlockUsers.removeFirst();
        us->eternalBlockUsers.append(user);
    }
    if (blocked)
        saveEternalBlockUsers();
}

void MainWindow::on_enableBlockCheck_clicked()
{
    bool enable = ui->enableBlockCheck->isChecked();
    us->setValue("block/enableBlock", enable);
    if (danmakuWindow)
        danmakuWindow->setEnableBlock(enable);

    if (enable)
        liveService->refreshBlockList();
}


void MainWindow::on_newbieTipCheck_clicked()
{
    bool enable = ui->newbieTipCheck->isChecked();
    us->setValue("block/newbieTip", enable);
    if (danmakuWindow)
        danmakuWindow->setNewbieTip(enable);
}

void MainWindow::on_autoBlockNewbieCheck_clicked()
{
    us->setValue("block/autoBlockNewbie", ui->autoBlockNewbieCheck->isChecked());
    ui->autoBlockNewbieNotifyCheck->setEnabled(ui->autoBlockNewbieCheck->isChecked());
}

void MainWindow::on_autoBlockNewbieKeysEdit_textChanged()
{
    us->setValue("block/autoBlockNewbieKeys", ui->autoBlockNewbieKeysEdit->toPlainText());
}

void MainWindow::on_autoBlockNewbieNotifyCheck_clicked()
{
    us->setValue("block/autoBlockNewbieNotify", ui->autoBlockNewbieNotifyCheck->isChecked());
}

void MainWindow::on_autoBlockNewbieNotifyWordsEdit_textChanged()
{
    us->setValue("block/autoBlockNewbieNotifyWords", ui->autoBlockNewbieNotifyWordsEdit->toPlainText());
}

void MainWindow::on_saveDanmakuToFileCheck_clicked()
{
    us->saveDanmakuToFile = ui->saveDanmakuToFileCheck->isChecked();
    us->setValue("danmaku/saveDanmakuToFile", us->saveDanmakuToFile);
    if (us->saveDanmakuToFile)
        liveService->startSaveDanmakuToFile();
    else
        liveService->finishSaveDanmuToFile();
}

void MainWindow::on_promptBlockNewbieCheck_clicked()
{
    us->setValue("block/promptBlockNewbie", ui->promptBlockNewbieCheck->isChecked());
}

void MainWindow::on_promptBlockNewbieKeysEdit_textChanged()
{
    us->setValue("block/promptBlockNewbieKeys", ui->promptBlockNewbieKeysEdit->toPlainText());
}

void MainWindow::on_timerConnectServerCheck_clicked()
{
    bool enable = ui->timerConnectServerCheck->isChecked();
    us->timerConnectServer = enable;
    us->setValue("live/timerConnectServer", enable);
    if (!liveService->isLiving() && enable)
        startConnectRoom();
    else if (!enable && (!liveService->liveSocket || liveService->liveSocket->state() == QAbstractSocket::UnconnectedState))
        startConnectRoom();
}

void MainWindow::on_startLiveHourSpin_valueChanged(int arg1)
{
    us->startLiveHour = ui->startLiveHourSpin->value();
    us->setValue("live/startLiveHour", us->startLiveHour);
    if (!justStart && us->timerConnectServer && liveService->connectServerTimer && !liveService->connectServerTimer->isActive())
        liveService->connectServerTimer->start();
}

void MainWindow::on_endLiveHourSpin_valueChanged(int arg1)
{
    us->endLiveHour = ui->endLiveHourSpin->value();
    us->setValue("live/endLiveHour", us->endLiveHour);
    if (!justStart && us->timerConnectServer && liveService->connectServerTimer && !liveService->connectServerTimer->isActive())
        liveService->connectServerTimer->start();
}

void MainWindow::on_calculateDailyDataCheck_clicked()
{
    us->calculateDailyData = ui->calculateDailyDataCheck->isChecked();
    us->setValue("live/calculateDaliyData", us->calculateDailyData);
    if (us->calculateDailyData)
        liveService->startCalculateDailyData();
}

void MainWindow::on_pushButton_clicked()
{
    QString text = QDateTime::currentDateTime().toString("yyyy-MM-dd\n");
    text += "\n进来人次：" + snum(liveService->dailyCome);
    text += "\n观众人数：" + snum(us->userComeTimes.count());
    text += "\n弹幕数量：" + snum(liveService->dailyDanmaku);
    text += "\n新人弹幕：" + snum(liveService->dailyNewbieMsg);
    text += "\n新增关注：" + snum(liveService->dailyNewFans);
    text += "\n银瓜子数：" + snum(liveService->dailyGiftSilver);
    text += "\n金瓜子数：" + snum(liveService->dailyGiftGold);
    text += "\n上船次数：" + snum(liveService->dailyGuard);
    text += "\n最高人气：" + snum(liveService->dailyMaxPopul);
    text += "\n平均人气：" + snum(liveService->dailyAvePopul);

    text += "\n\n累计粉丝：" + snum(ac->currentFans);
    QMessageBox::information(this, "今日数据", text);
}

void MainWindow::on_removeDanmakuTipIntervalSpin_valueChanged(int arg1)
{
    this->removeDanmakuTipInterval = arg1 * 1000;
    us->setValue("danmaku/removeTipInterval", arg1);
}

void MainWindow::on_doveCheck_clicked()
{
    // 这里不做保存，重启失效
    us->liveDove = ui->doveCheck->isChecked();
}

void MainWindow::on_notOnlyNewbieCheck_clicked()
{
    bool enable = ui->notOnlyNewbieCheck->isChecked();
    us->setValue("block/notOnlyNewbie", enable);
}

void MainWindow::on_pkAutoMelonCheck_clicked()
{
    bool enable = ui->pkAutoMelonCheck->isChecked();
    us->setValue("pk/autoMelon", enable);
}

void MainWindow::on_pkMaxGoldButton_clicked()
{
    bool ok = false;
    // 默认最大设置的是1000元，有钱人……
    int v = QInputDialog::getInt(this, "自动偷塔礼物上限", "单次PK偷塔赠送的金瓜子上限\n1元=1000金瓜子=100或10乱斗值（按B站规则会变）\n注意：每次偷塔、反偷塔时都单独判断上限，而非一次大乱斗的累加", liveService->pkMaxGold, 0, 1000000, 100, &ok);
    if (!ok)
        return ;

    us->setValue("pk/maxGold", liveService->pkMaxGold = v);
}

void MainWindow::on_pkJudgeEarlyButton_clicked()
{
    bool ok = false;
    int v = QInputDialog::getInt(this, "自动偷塔提前判断", "每次偷塔结束前n毫秒判断，根据网速酌情设置\n1秒=1000毫秒，下次大乱斗生效", liveService->pkJudgeEarly, 0, 5000, 500, &ok);
    if (!ok)
        return ;

    us->setValue("pk/judgeEarly", liveService->pkJudgeEarly = v);
}

void MainWindow::on_roomIdEdit_returnPressed()
{
    if (liveService->liveSocket->state() == QAbstractSocket::UnconnectedState)
    {
        startConnectRoom();
    }
}

void MainWindow::on_actionData_Path_triggered()
{
    QDesktopServices::openUrl(QUrl("file:///" + rt->dataPath, QUrl::TolerantMode));
}

/**
 * 显示实时弹幕
 */
void MainWindow::on_actionShow_Live_Danmaku_triggered()
{
    if (!danmakuWindow)
    {
        danmakuWindow = new LiveDanmakuWindow(this);
        danmakuWindow->setChatService(this->chatService);

        connect(this, &MainWindow::signalNewDanmaku, danmakuWindow, [=](const LiveDanmaku &danmaku) {
            if (danmaku.is(MSG_DANMAKU))
            {
                if (cr->isFilterRejected("FILTER_DANMAKU_MSG", danmaku))
                    return ;
            }
            else if (danmaku.is(MSG_WELCOME) || danmaku.is(MSG_WELCOME_GUARD))
            {
                if (cr->isFilterRejected("FILTER_DANMAKU_COME", danmaku))
                    return ;
            }
            else if (danmaku.is(MSG_GIFT) || danmaku.is(MSG_GUARD_BUY))
            {
                if (cr->isFilterRejected("FILTER_DANMAKU_GIFT", danmaku))
                    return ;
            }
            else if (danmaku.is(MSG_ATTENTION))
            {
                if (cr->isFilterRejected("FILTER_DANMAKU_ATTENTION", danmaku))
                    return ;
            }

            danmakuWindow->slotNewLiveDanmaku(danmaku);
        });

        connect(this, SIGNAL(signalRemoveDanmaku(LiveDanmaku)), danmakuWindow, SLOT(slotOldLiveDanmakuRemoved(LiveDanmaku)));
        connect(danmakuWindow, SIGNAL(signalSendMsg(QString)), liveService, SLOT(sendMsg(QString)));
        connect(danmakuWindow, SIGNAL(signalAddBlockUser(qint64, int, QString)), liveService, SLOT(addBlockUser(qint64, int, QString)));
        connect(danmakuWindow, SIGNAL(signalDelBlockUser(qint64)), liveService, SLOT(delBlockUser(qint64)));
        connect(danmakuWindow, SIGNAL(signalEternalBlockUser(qint64,QString,QString)), this, SLOT(eternalBlockUser(qint64,QString,QString)));
        connect(danmakuWindow, SIGNAL(signalCancelEternalBlockUser(qint64)), this, SLOT(cancelEternalBlockUser(qint64)));
        connect(danmakuWindow, SIGNAL(signalAIReplyed(QString, qint64)), this, SLOT(slotAIReplyed(QString, qint64)));
        connect(danmakuWindow, SIGNAL(signalShowPkVideo()), this, SLOT(on_actionShow_PK_Video_triggered()));
        connect(danmakuWindow, &LiveDanmakuWindow::signalChangeWindowMode, this, [=]{
            danmakuWindow->deleteLater();
            danmakuWindow = nullptr;
            on_actionShow_Live_Danmaku_triggered(); // 重新加载
        });
        connect(danmakuWindow, &LiveDanmakuWindow::signalSendMsgToPk, this, [=](QString msg){
            if (!liveService->pking || liveService->pkRoomId.isEmpty())
                return ;
            qInfo() << "发送PK对面消息：" << liveService->pkRoomId << msg;
            liveService->sendRoomMsg(liveService->pkRoomId, msg);
        });
        connect(danmakuWindow, &LiveDanmakuWindow::signalMarkUser, this, [=](qint64 uid){
            if (judgeRobot)
                markNotRobot(uid);
        });
        connect(danmakuWindow, &LiveDanmakuWindow::signalTransMouse, this, [=](bool enabled){
            if (enabled)
                ui->closeTransMouseButton->show();
            else
                ui->closeTransMouseButton->hide();
        });
        connect(danmakuWindow, &LiveDanmakuWindow::signalAddCloudShieldKeyword, this, &MainWindow::addCloudShieldKeyword);
        connect(danmakuWindow, SIGNAL(signalAppointAdmin(qint64)), liveService, SLOT(appointAdmin(qint64)));
        connect(danmakuWindow, SIGNAL(signalDismissAdmin(qint64)), liveService, SLOT(dismissAdmin(qint64)));
        danmakuWindow->setEnableBlock(ui->enableBlockCheck->isChecked());
        danmakuWindow->setNewbieTip(ui->newbieTipCheck->isChecked());
        danmakuWindow->setIds(ac->upUid.toLongLong(), ac->roomId.toLongLong());
        danmakuWindow->setWindowIcon(this->windowIcon());
        danmakuWindow->setWindowTitle(this->windowTitle());
        danmakuWindow->hide();

        // PK中
        if (liveService->pkBattleType)
        {
            danmakuWindow->setPkStatus(1, liveService->pkRoomId.toLongLong(), liveService->pkUid.toLongLong(), liveService->pkUname);
        }

        // 添加弹幕
        QTimer::singleShot(0, [=]{
            danmakuWindow->removeAll();
            if (liveService->roomDanmakus.size())
            {
                for (int i = 0; i < liveService->roomDanmakus.size(); i++)
                    danmakuWindow->slotNewLiveDanmaku(liveService->roomDanmakus.at(i));
            }
            else // 没有之前的弹幕，从API重新pull下来
            {
                pullLiveDanmaku();
            }
            danmakuWindow->setAutoTranslate(ui->languageAutoTranslateCheck->isChecked());
            danmakuWindow->setAIReply(ui->AIReplyCheck->isChecked());

            if (liveService->pking)
            {
                danmakuWindow->setIds(ac->upUid.toLongLong(), ac->roomId.toLongLong());
            }
        });
    }

    bool hidding = danmakuWindow->isHidden();

    if (hidding)
    {
        danmakuWindow->show();
    }
    else
    {
        danmakuWindow->hide();
    }
    us->setValue("danmaku/liveWindow", hidding);
}

void MainWindow::on_actionSet_Cookie_triggered()
{
    bool ok = false;
    QString s = QInputDialog::getText(this, "设置Cookie", "设置用户登录的cookie", QLineEdit::Normal, ac->browserCookie, &ok);
    if (!ok)
        return ;

    if (!s.contains("SESSDATA="))
    {
        QMessageBox::warning(this, "设置Cookie", "设置Cookie失败，这不是Bilibili的浏览器Cookie");
        return ;
    }

    if (s.toLower().startsWith("cookie:"))
        s = s.replace(0, 7, "").trimmed();

    autoSetCookie(s);
}

void MainWindow::on_actionSet_Danmaku_Data_Format_triggered()
{
    bool ok = false;
    QString s = QInputDialog::getText(this, "设置Data", "设置弹幕的data\n自动从cookie中提取，可不用设置", QLineEdit::Normal, ac->browserData, &ok);
    if (!ok)
        return ;

    us->setValue("danmaku/browserData", ac->browserData = s);
    int posl = ac->browserData.indexOf("csrf_token=") + 9;
    int posr = ac->browserData.indexOf("&", posl);
    if (posr == -1) posr = ac->browserData.length();
    ac->csrf_token = ac->browserData.mid(posl, posr - posl);
}

void MainWindow::on_actionCookie_Help_triggered()
{
    QString steps = "发送弹幕前需按以下步骤注入登录信息：\n\n";
    steps += "步骤一：\n浏览器登录bilibili账号，按下F12（开发者调试工具）\n\n";
    steps += "步骤二：\n找到右边顶部的“Network”项，选择它下面的XHR\n\n";
    steps += "步骤三：\n刷新B站页面，中间多出一排列表，点其中任意一个，看右边“Headers”中的代码\n\n";
    steps += "步骤四：\n复制“Request Headers”下的“cookie”冒号后的一长串内容，粘贴到本程序“设置Cookie”中\n\n";
    steps += "设置好直播间ID、要发送的内容，即可发送弹幕！\n";
    steps += "注意：请勿过于频繁发送，容易被临时拉黑！";

    /*steps += "\n\n变量列表：\n";
    steps += "\\n：分成多条弹幕发送，间隔1.5秒";
    steps += "\n%hour%：根据时间替换为“早上”、“中午”、“晚上”等";
    steps += "\n%all_greet%：根据时间替换为“你好啊”、“早上好呀”、“晚饭吃了吗”、“还没睡呀”等";
    steps += "\n%greet%：根据时间替换为“你好”、“早上好”、“中午好”等";
    steps += "\n%tone%：随机替换为“啊”、“呀”";
    steps += "\n%punc%：随机替换为“~”、“！”";
    steps += "\n%tone/punc%：随机替换为“啊”、“呀”、“~”、“！”";*/
    QMessageBox::information(this, "定时弹幕", steps);
}

void MainWindow::on_actionCreate_Video_LRC_triggered()
{
    VideoLyricsCreator* vlc = new VideoLyricsCreator(us, nullptr);
    vlc->show();
}

void MainWindow::on_actionShow_Order_Player_Window_triggered()
{
    if (!musicWindow)
    {
        musicWindow = new OrderPlayerWindow(rt->dataPath, nullptr);
        cr->setMusicWindow(musicWindow);
        connect(musicWindow, &OrderPlayerWindow::signalOrderSongSucceed, this, [=](Song song, qint64 latency, int waiting){
            qInfo() << "点歌成功" << song.simpleString() << latency;

            // 获取UID
            qint64 uid = 0;
            for (int i = liveService->roomDanmakus.size()-1; i >= 0; i--)
            {
                const LiveDanmaku danmaku = liveService->roomDanmakus.at(i);
                if (!danmaku.is(MSG_DANMAKU))
                    continue;

                QString nick = danmaku.getNickname();
                if (nick == song.addBy)
                {
                    uid = danmaku.getUid();
                    break;
                }
            }

            LiveDanmaku danmaku(uid, song.addBy, song.name);
            danmaku.setPrevTimestamp(latency / 1000); // 毫秒转秒
            danmaku.setFirst(waiting); // 当前歌曲在第几首，例如1，则是下一首
            danmaku.with(song.toJson());

            triggerCmdEvent("ORDER_SONG_SUCCEED", danmaku);
            triggerCmdEvent("ORDER_SONG_SUCCEED_OVERRIDE", danmaku);
            if (hasEvent("ORDER_SONG_SUCCEED_OVERRIDE"))
                return ;

            if (latency < 180000) // 小于3分钟
            {
                QString tip = "成功点歌：【" + song.simpleString() + "】";
                localNotify(tip);
                if (ui->diangeReplyCheck->isChecked())
                {
                    if (waiting == 1 && latency > 20000)
                        cr->sendNotifyMsg("成功点歌，下一首播放");
                    else if (waiting == 2 && latency > 20000)
                        cr->sendNotifyMsg("成功点歌，下两首播放");
                    else // 多首队列
                        cr->sendNotifyMsg("成功点歌");
                }
            }
            else // 超过3分钟
            {
                int minute = (latency+20000) / 60000;
                localNotify(snum(minute) + "分钟后播放【" + song.simpleString() + "】");
                if (ui->diangeReplyCheck->isChecked())
                    cr->sendNotifyMsg("成功点歌，" + snum(minute) + "分钟后播放");
            }
        });
        connect(musicWindow, &OrderPlayerWindow::signalOrderSongPlayed, this, [=](Song song){
            localNotify("开始播放：" + song.simpleString());
            LiveDanmaku danmaku(song.id, song.addBy, song.name);
            danmaku.setPrevTimestamp(song.addTime);
            danmaku.with(song.toJson());
            triggerCmdEvent("ORDER_SONG_PLAY", danmaku);
        });
        connect(musicWindow, &OrderPlayerWindow::signalCurrentSongChanged, this, [=](Song song){
            if (webServer->sendCurrentSongToSockets)
                sendJsonToSockets("CURRENT_SONG", song.toJson());

            LiveDanmaku danmaku(song.id, song.addBy, song.name);
            danmaku.setPrevTimestamp(song.addTime);
            danmaku.with(song.toJson());
            triggerCmdEvent("CURRENT_SONG_CHANGED", danmaku);

            if (ui->playingSongToFileCheck->isChecked())
            {
                savePlayingSong();
            }
        });

        connect(musicWindow, &OrderPlayerWindow::signalSongPlayFinished, this, [=](Song song){
            LiveDanmaku danmaku(song.id, song.addBy, song.name);
            danmaku.setPrevTimestamp(song.addTime);
            danmaku.with(song.toJson());
            triggerCmdEvent("SONG_PLAY_FINISHED", danmaku);
        });
        connect(musicWindow, &OrderPlayerWindow::signalOrderSongImproved, this, [=](Song song, int prev, int curr){
            localNotify("提升歌曲：" + song.name + " : " + snum(prev+1) + "->" + snum(curr));
            LiveDanmaku danmaku(song.id, song.addBy, song.name);
            danmaku.setNumber(curr+1);
            danmaku.with(song.toJson());
            triggerCmdEvent("ORDER_SONG_IMPROVED", danmaku);
        });
        connect(musicWindow, &OrderPlayerWindow::signalOrderSongCutted, this, [=](Song song){
            // localNotify("已切歌");
            triggerCmdEvent("ORDER_SONG_CUTTED", LiveDanmaku(song.id, song.addBy, song.name).with(song.toJson()));
        });
        connect(musicWindow, &OrderPlayerWindow::signalOrderSongNotFound, this, [=](QString key){
            localNotify("未找到歌曲：" + key);
            triggerCmdEvent("ORDER_SONG_NOT_FOUND", LiveDanmaku(key));
        });
        connect(musicWindow, &OrderPlayerWindow::signalOrderSongModified, this, [=](const SongList& songs){
            if (ui->orderSongsToFileCheck->isChecked())
            {
                saveOrderSongs(songs);
            }
            if (webServer->sendSongListToSockets)
            {
                sendMusicList(songs);
            }
        });
        connect(musicWindow, &OrderPlayerWindow::signalLyricChanged, this, [=](){
            if (ui->songLyricsToFileCheck->isChecked())
            {
                saveSongLyrics();
            }
            if (webServer->sendLyricListToSockets)
            {
                sendLyricList();
            }
        });
        auto simulateMusicKey = [=]{
            if (!ui->autoPauseOuterMusicCheck->isChecked())
                return ;
#if defined (Q_OS_WIN)
            cr->simulateKeys(ui->outerMusicKeyEdit->text());
#endif
        };
        connect(musicWindow, &OrderPlayerWindow::signalOrderSongStarted, this, [=]{
            simulateMusicKey();
        });
        connect(musicWindow, &OrderPlayerWindow::signalOrderSongEnded, this, [=]{
            simulateMusicKey();
        });
        connect(musicWindow, &OrderPlayerWindow::signalWindowClosed, this, [=]{
            QTimer::singleShot(5000, this, [=]{ // 延迟5秒，避免程序关闭时先把点歌姬关了，但下次还是需要显示的
                us->setValue("danmaku/playerWindow", false);
            });
        });
    }

    bool hidding = musicWindow->isHidden();

    if (hidding)
    {
        musicWindow->show();
    }
    else
    {
        musicWindow->hide();
    }
    us->setValue("danmaku/playerWindow", hidding);
}

void MainWindow::on_diangeReplyCheck_clicked()
{
    us->setValue("danmaku/diangeReply", ui->diangeReplyCheck->isChecked());
}

void MainWindow::on_actionAbout_triggered()
{
    QString appVersion = GetFileVertion(QApplication::applicationFilePath()).trimmed();
    if (!appVersion.startsWith("v") && !appVersion.startsWith("V"))
        appVersion.insert(0, "v");

    QString text;
    text += QApplication::applicationName() + " " + appVersion;
    text += "\n\n本程序由心乂独立开发，参考多个开源项目实现。\n仅供个人学习、研究之用，禁止用于商业用途。\n\n";
    text += "QQ群：1038738410\n欢迎大家一起来交流反馈&功能研发&闲聊&搞事情&拯救地球\n\n";
    text += "GitHub: https://github.com/iwxyi";
    QMessageBox::information(this, "关于", text);
}

void MainWindow::on_actionGitHub_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/iwxyi/Bilibili-MagicalDanmaku"));
}

void MainWindow::on_actionCustom_Variant_triggered()
{
    QString text = saveCustomVariant();
    bool ok;
    text = TextInputDialog::getText(this, "自定义变量", "请输入自定义变量，可在答谢、定时中使用：\n示例格式：%var%=val", text, &ok);
    if (!ok)
        return ;
    us->setValue("danmaku/customVariant", text);

    restoreCustomVariant(text);
}

void MainWindow::on_actionVariant_Translation_triggered()
{
}

void MainWindow::on_actionSend_Long_Text_triggered()
{
    bool ok;
    QString text = QInputDialog::getText(this, "发送长文本", "请输入长文本（支持变量），分割为每次20字发送\n注意带有敏感词或特殊字符的部分将无法发送", QLineEdit::Normal, "", &ok);
    text = text.trimmed();
    if (!ok || text.isEmpty())
        return ;

    liveService->sendLongText(text);
}

void MainWindow::on_actionShow_Lucky_Draw_triggered()
{
    if (!luckyDrawWindow)
    {
        luckyDrawWindow = new LuckyDrawWindow(nullptr);
        connect(this, SIGNAL(signalNewDanmaku(const LiveDanmaku&)), luckyDrawWindow, SLOT(slotNewDanmaku(const LiveDanmaku&)));
    }
    luckyDrawWindow->show();
}

void MainWindow::on_actionGet_Play_Url_triggered()
{
    getRoomLiveVideoUrl([=](QString url) {
        QApplication::clipboard()->setText(url);
    });
}

void MainWindow::on_actionShow_Live_Video_triggered()
{
    if (ac->roomId.isEmpty())
        return ;
    if (!hasPermission())
    {
        on_actionBuy_VIP_triggered();
        return ;
    }

    LiveVideoPlayer* player = new LiveVideoPlayer(us, nullptr);
    connect(this, SIGNAL(signalLiveStart(QString)), player, SLOT(slotLiveStart(QString))); // 重新开播，需要刷新URL
    connect(player, SIGNAL(signalRestart()), this, SLOT(on_actionShow_Live_Video_triggered()));
    player->setAttribute(Qt::WA_DeleteOnClose, true);
    player->setRoomId(ac->roomId);
    player->setWindowTitle(ac->roomTitle + " - " + ac->upName);
    player->setWindowIcon(liveService->upFace);
    player->show();
}

void MainWindow::on_actionShow_PK_Video_triggered()
{
    if (liveService->pkRoomId.isEmpty())
        return ;

    LiveVideoPlayer* player = new LiveVideoPlayer(us, nullptr);
    player->setAttribute(Qt::WA_DeleteOnClose, true);
    player->setRoomId(liveService->pkRoomId);
    player->show();
}

void MainWindow::on_pkChuanmenCheck_clicked()
{
    liveService->pkChuanmenEnable = ui->pkChuanmenCheck->isChecked();
    us->setValue("pk/chuanmen", liveService->pkChuanmenEnable);

    if (liveService->pkChuanmenEnable)
    {
        liveService->connectPkRoom();
    }
}

void MainWindow::on_pkMsgSyncCheck_clicked()
{
    if (liveService->pkMsgSync == 0)
    {
        liveService->pkMsgSync = 1;
        ui->pkMsgSyncCheck->setCheckState(Qt::PartiallyChecked);
    }
    else if (liveService->pkMsgSync == 1)
    {
        liveService->pkMsgSync = 2;
        ui->pkMsgSyncCheck->setCheckState(Qt::Checked);
    }
    else if (liveService->pkMsgSync == 2)
    {
        liveService->pkMsgSync = 0;
        ui->pkMsgSyncCheck->setCheckState(Qt::Unchecked);
    }
    ui->pkMsgSyncCheck->setText(liveService->pkMsgSync == 1 ? "PK同步消息(仅视频)" : "PK同步消息");
    us->setValue("pk/msgSync", liveService->pkMsgSync);
}

void MainWindow::pkPre(QJsonObject json)
{
    /*{
        "cmd": "PK_BATTLE_PRE",
        "pk_status": 101,
        "pk_id": 100970480,
        "timestamp": 1607763991,
        "data": {
            "battle_type": 1, // 自己开始匹配？
            "uname": "SLe\\u4e36\\u82cf\\u4e50",
            "face": "http:\\/\\/i2.hdslb.com\\/bfs\\/face\\/4636d48aeefa1a177bc2bdfb595892d3648b80b1.jpg",
            "uid": 13330958,
            "room_id": 271270,
            "season_id": 30,
            "pre_timer": 10,
            "pk_votes_name": "\\u4e71\\u6597\\u503c",
            "end_win_task": null
        },
        "roomid": 22532956
    }*/

    /*{ 对面匹配过来的情况
        "cmd": "PK_BATTLE_PRE",
        "pk_status": 101,
        "pk_id": 100970387,
        "timestamp": 1607763565,
        "data": {
            "battle_type": 2, // 对面开始匹配？
            "uname": "\\u519c\\u6751\\u9493\\u9c7c\\u5c0f\\u6b66\\u5929\\u5929\\u76f4\\u64ad",
            "face": "http:\\/\\/i0.hdslb.com\\/bfs\\/face\\/fbaa9cfbc214164236cdbe79a77bcaae5334e9ef.jpg",
            "uid": 199775659, // 对面用户ID
            "room_id": 12298098, // 对面房间ID
            "season_id": 30,
            "pre_timer": 10,
            "pk_votes_name": "\\u4e71\\u6597\\u503c", // 乱斗值
            "end_win_task": null
        },
        "roomid": 22532956
    }*/

    // 大乱斗信息
    QJsonObject data = json.value("data").toObject();
    QString uname = data.value("uname").toString();
    QString uid = QString::number(static_cast<qint64>(data.value("uid").toDouble()));
    QString room_id = QString::number(static_cast<qint64>(data.value("room_id").toDouble()));
    liveService->pkUname = uname;
    liveService->pkRoomId = room_id;
    liveService->pkUid = uid;

    // 大乱斗类型
    liveService->pkBattleType = data.value("battle_type").toInt();
    if (liveService->pkBattleType == 1) // 普通大乱斗
        liveService->pkVideo = false;
    else if (liveService->pkBattleType == 2) // 视频大乱斗
        liveService->pkVideo = true;
    else
        liveService->pkVideo = false;

    qInfo() << "准备大乱斗，已匹配：" << static_cast<qint64>(json.value("pk_id").toDouble())  << "    类型：" << liveService->pkBattleType;
    if (danmakuWindow)
    {
        if (uname.isEmpty())
            danmakuWindow->setStatusText("大乱斗匹配中...");
        else if (!liveService->pkRoomId.isEmpty())
        {
            int pkCount = us->danmakuCounts->value("pk/" + liveService->pkRoomId, 0).toInt();
            QString text = "匹配：" + uname;
            if(pkCount > 0)
                text += "[" + QString::number(pkCount) + "]";
            danmakuWindow->setStatusText(text);
            qInfo() << "主播：" << uname << liveService->pkUid << liveService->pkRoomId;
            danmakuWindow->setPkStatus(1, liveService->pkRoomId.toLongLong(), liveService->pkUid.toLongLong(), liveService->pkUname);
        }
    }
    liveService->pkToLive = QDateTime::currentSecsSinceEpoch();

    if (liveService->pkChuanmenEnable /*&& battle_type == 2*/)
    {
        liveService->connectPkRoom();
    }
    ui->actionShow_PK_Video->setEnabled(true);
    ui->actionJoin_Battle->setEnabled(false);
    liveService->pkGifts.clear();

    // 处理PK对面直播间事件
    if (hasEvent("PK_MATCH_INFO"))
    {
        getPkMatchInfo();
    }
}

void MainWindow::pkStart(QJsonObject json)
{
    /*{
        "cmd": "PK_BATTLE_START",
        "data": {
            "battle_type": 1, // 不知道其他类型是啥
            "final_hit_votes": 0,
            "pk_end_time": 1605748342,
            "pk_frozen_time": 1605748332,
            "pk_start_time": 1605748032,
            "pk_votes_add": 0,
            "pk_votes_name": "乱斗值",
            "pk_votes_type": 0
        },
        "pk_id": 100729281,
        "pk_status": 201,
        "timestamp": 1605748032
    }*/

    QJsonObject data = json.value("data").toObject();
    liveService->pking = true;
    qint64 startTime = static_cast<qint64>(data.value("pk_start_time").toDouble());
    // qint64 endTime = static_cast<qint64>(data.value("pk_end_time").toDouble());
    liveService->pkEndTime = startTime + 300; // 因为endTime要延迟10秒，还是用startTime来判断吧
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    qint64 deltaEnd = liveService->pkEndTime - currentTime;
    QString roomId = ac->roomId;
    liveService->oppositeTouta = 0;
    liveService->cmAudience.clear();
    liveService->pkBattleType = data.value("battle_type").toInt();
    if (liveService->pkBattleType == 1) // 普通大乱斗
        liveService->pkVideo = false;
    else if (liveService->pkBattleType == 2) // 视频大乱斗
        liveService->pkVideo = true;
    else
        liveService->pkVideo = false;
    if (liveService->pkVideo)
    {
        liveService->pkToLive = currentTime;
    }

    // 结束前x秒
    liveService->pkEndingTimer->start(deltaEnd*1000 - liveService->pkJudgeEarly);

    liveService->pkTimer->start();
    if (danmakuWindow)
    {
        danmakuWindow->showStatusText();
        danmakuWindow->setToolTip(liveService->pkUname);
        danmakuWindow->setPkStatus(1, liveService->pkRoomId.toLongLong(), liveService->pkUid.toLongLong(), liveService->pkUname);
    }
    qint64 pkid = static_cast<qint64>(json.value("pk_id").toDouble());
    qInfo() << "开启大乱斗, id =" << pkid << "  room=" << liveService->pkRoomId << "  user=" << liveService->pkUid << "   battle_type=" << liveService->pkBattleType;

    // 保存PK信息
    int pkCount = us->danmakuCounts->value("pk/" + liveService->pkRoomId, 0).toInt();
    us->danmakuCounts->setValue("pk/" + liveService->pkRoomId, pkCount+1);
    qInfo() << "保存匹配次数：" << liveService->pkRoomId << liveService->pkUname << (pkCount+1);

    // PK提示
    QString text = "开启大乱斗：" + liveService->pkUname;
    if (pkCount)
        text += "  PK过" + QString::number(pkCount) + "次";
    else
        text += "  初次匹配";
    localNotify(text, liveService->pkUid.toLongLong());

    // 处理对面在线舰长界面
    if (hasEvent("PK_MATCH_ONLINE_GUARD"))
    {
        getPkOnlineGuardPage(0);
    }
}

void MainWindow::pkProcess(QJsonObject json)
{
    /*{
        "cmd": "PK_BATTLE_PROCESS",
        "data": {
            "battle_type": 1,
            "init_info": {
                "best_uname": "我今天超可爱0",
                "room_id": 22532956,
                "votes": 132
            },
            "match_info": {
                "best_uname": "银河的偶尔限定女友粉",
                "room_id": 21398069,
                "votes": 156
            }
        },
        "pk_id": 100729411,
        "pk_status": 201,
        "timestamp": 1605749908
    }*/
    QJsonObject data = json.value("data").toObject();
    int prevMyVotes = liveService->myVotes;
    int prevMatchVotes = liveService->matchVotes;
    if (snum(static_cast<qint64>(data.value("init_info").toObject().value("room_id").toDouble())) == ac->roomId)
    {
        liveService->myVotes = data.value("init_info").toObject().value("votes").toInt();
        liveService->matchVotes = data.value("match_info").toObject().value("votes").toInt();
    }
    else
    {
        liveService->myVotes = data.value("match_info").toObject().value("votes").toInt();
        liveService->matchVotes = data.value("init_info").toObject().value("votes").toInt();
    }

    if (!liveService->pkTimer->isActive())
        liveService->pkTimer->start();

    if (liveService->pkEnding)
    {
        qInfo() << "大乱斗进度(偷塔阶段)：" << liveService->myVotes << liveService->matchVotes << "   等待送到：" << liveService->pkVoting;

        // 显示偷塔情况
        if (prevMyVotes < liveService->myVotes)
        {
            int delta = liveService->myVotes - prevMyVotes;
            localNotify("[己方偷塔] + " + snum(delta));

            // B站返回的规则改了，偷塔的时候获取不到礼物了
            liveService->pkVoting -= delta;
            if (liveService->pkVoting < 0)
                liveService->pkVoting = 0;
        }
        if (prevMatchVotes < liveService->matchVotes)
        {
            liveService->oppositeTouta++;
            localNotify("[对方偷塔] + " + snum(liveService->matchVotes - prevMatchVotes));
            {
                // qInfo() << "pk偷塔信息：" << s;
                if (liveService->danmuLogStream)
                {
                    /* int melon = 100 / goldTransPk; // 单个吃瓜有多少乱斗值
                    int num = static_cast<int>((matchVotes-myVotes-pkVoting+melon)/melon);
                    QString s = QString("myVotes:%1, pkVoting:%2, matchVotes:%3, maxGold:%4, goldTransPk:%5, oppositeTouta:%6, need:%7")
                                .arg(myVotes).arg(pkVoting).arg(matchVotes).arg(getPkMaxGold(qMax(myVotes, matchVotes))).arg(goldTransPk).arg(oppositeTouta)
                                .arg(num);
                    (*danmuLogStream) << s << "\n";
                    (*danmuLogStream).flush(); // 立刻刷新到文件里 */
                }
            }
        }

        // 反偷塔，防止对方也在最后几秒刷礼物
        execTouta();
    }
    else
    {
        qInfo() << "大乱斗进度：" << liveService->myVotes << liveService->matchVotes;
    }
}

void MainWindow::pkEnd(QJsonObject json)
{
    /*{
        "cmd": "PK_BATTLE_END",
        "data": {
            "battle_type": 1,
            "init_info": {
                "best_uname": "我今天超可爱0",
                "room_id": 22532956,
                "votes": 10,
                "winner_type": 2
            },
            "match_info": {
                "best_uname": "",
                "room_id": 22195813,
                "votes": 0,
                "winner_type": -1
            },
            "timer": 10
        },
        "pk_id": "100729259",
        "pk_status": 401,
        "timestamp": 1605748006
    }*/
    // winner_type: 2赢，-1输，两边2平局

    QJsonObject data = json.value("data").toObject();
    if (liveService->pkVideo)
        liveService->pkToLive = QDateTime::currentSecsSinceEpoch();
    int winnerType1 = data.value("init_info").toObject().value("winner_type").toInt();
    int winnerType2 = data.value("match_info").toObject().value("winner_type").toInt();
    qint64 thisRoomId = static_cast<qint64>(data.value("init_info").toObject().value("room_id").toDouble());
    if (liveService->pkTimer)
        liveService->pkTimer->stop();
    if (liveService->pkEndingTimer)
        liveService->pkEndingTimer->stop();
    if (danmakuWindow)
    {
        danmakuWindow->hideStatusText();
        danmakuWindow->setToolTip("");
        danmakuWindow->setPkStatus(0, 0, 0, "");
    }
    QString bestName = "";
    int winnerType = 0;
    if (snum(thisRoomId) == ac->roomId) // init是自己
    {
        liveService->myVotes = data.value("init_info").toObject().value("votes").toInt();
        liveService->matchVotes = data.value("match_info").toObject().value("votes").toInt();
        bestName = data.value("init_info").toObject().value("best_uname").toString();
        winnerType = winnerType1;
    }
    else // match是自己
    {
        liveService->matchVotes = data.value("init_info").toObject().value("votes").toInt();
        liveService->myVotes = data.value("match_info").toObject().value("votes").toInt();
        bestName = data.value("match_info").toObject().value("best_uname").toString();
        winnerType = winnerType2;
    }

    bool ping = winnerType1 == winnerType2;
    bool result = winnerType > 0;

    qint64 bestUid = 0;
    int winCode = 0;
    if (!ping)
        winCode = winnerType;
    if (liveService->myVotes > 0)
    {
        for (int i = liveService->pkGifts.size()-1; i >= 0; i--)
            if (liveService->pkGifts.at(i).getNickname() == bestName)
            {
                bestUid = liveService->pkGifts.at(i).getUid();
                break;
            }
        LiveDanmaku danmaku(bestName, bestUid, winCode, liveService->myVotes);
        triggerCmdEvent("PK_BEST_UNAME", danmaku.with(data), true);
    }
    triggerCmdEvent("PK_END", LiveDanmaku(bestName, bestUid, winCode, liveService->myVotes).with(data), true);

    localNotify(QString("大乱斗 %1：%2 vs %3")
                                     .arg(ping ? "平局" : (result ? "胜利" : "失败"))
                                     .arg(liveService->myVotes)
                                     .arg(liveService->matchVotes));
    qInfo() << "大乱斗结束，结果：" << (ping ? "平局" : (result ? "胜利" : "失败")) << liveService->myVotes << liveService->matchVotes;
    liveService->myVotes = 0;
    liveService->matchVotes = 0;
    QTimer::singleShot(60000, [=]{
        if (liveService->pking) // 下一把PK，已经清空了
            return ;
        liveService->cmAudience.clear();
    });

    // 保存对面偷塔次数
    if (liveService->oppositeTouta && !liveService->pkUname.isEmpty())
    {
        int count = us->danmakuCounts->value("touta/" + liveService->pkRoomId, 0).toInt();
        us->danmakuCounts->setValue("touta/" + liveService->pkRoomId, count+1);
    }

    // 清空大乱斗数据
    liveService->pking = false;
    liveService->pkEnding = false;
    liveService->pkVoting = 0;
    liveService->pkEndTime = 0;
    liveService->pkUname = "";
    liveService->pkUid = "";
    liveService->pkRoomId = "";
    liveService->myAudience.clear();
    liveService->oppositeAudience.clear();
    liveService->pkVideo = false;
    ui->actionShow_PK_Video->setEnabled(false);
    liveService->pkBattleType = 0;

    if (ac->cookieUid == ac->upUid)
        ui->actionJoin_Battle->setEnabled(true);

    if (liveService->pkLiveSocket)
    {
        try {
            if (liveService->pkLiveSocket->state() == QAbstractSocket::ConnectedState)
                liveService->pkLiveSocket->close(); // 会自动deleterLater
            // pkSocket->deleteLater();
        } catch (...) {
            qCritical() << "delete pkSocket failed";
        }
        liveService->pkLiveSocket = nullptr;
    }
}

void MainWindow::pkSettle(QJsonObject json)
{
    /*{
        "cmd": "PK_BATTLE_SETTLE_NEW",
        "pk_id": 200933662,
        "pk_status": 601,
        "timestamp": 1613959764,
        "data": {
            "pk_id": 200933662,
            "pk_status": 601,
            "settle_status": 1,
            "punish_end_time": 1613959944,
            "timestamp": 1613959764,
            "battle_type": 6,
            "init_info": {
                "room_id": 7259049,
                "result_type": -1,
                "votes": 0,
                "assist_info": []
            },
            "match_info": {
                "room_id": 21839758,
                "result_type": 2,
                "votes": 3,
                "assist_info": [
                    {
                        "rank": 1,
                        "uid": 412357310,
                        "face": "http:\\/\\/i0.hdslb.com\\/bfs\\/face\\/e97fbf0e412b936763033055821e1ff5df56565a.jpg",
                        "uname": "\\u6cab\\u58a8\\u58a8\\u58a8\\u58a8\\u58a8\\u58a8\\u58a8\\u58a8"
                    }
                ]
            },
            "dm_conf": {
                "font_color": "#FFE10B",
                "bg_color": "#72C5E2"
            }
        }
    }*/

    QJsonObject data = json.value("data").toObject();
    if (liveService->pkVideo)
        liveService->pkToLive = QDateTime::currentSecsSinceEpoch();
    int winnerType1 = data.value("init_info").toObject().value("result_type").toInt();
    int winnerType2 = data.value("match_info").toObject().value("result_type").toInt();
    qint64 thisRoomId = static_cast<qint64>(data.value("init_info").toObject().value("room_id").toDouble());
    if (liveService->pkTimer)
        liveService->pkTimer->stop();
    if (liveService->pkEndingTimer)
        liveService->pkEndingTimer->stop();
    if (danmakuWindow)
    {
        danmakuWindow->hideStatusText();
        danmakuWindow->setToolTip("");
        danmakuWindow->setPkStatus(0, 0, 0, "");
    }
    QString bestName = "";
    int winCode = 0;
    if (snum(thisRoomId) == ac->roomId) // init是自己
    {
        liveService->myVotes = data.value("init_info").toObject().value("votes").toInt();
        liveService->matchVotes = data.value("match_info").toObject().value("votes").toInt();
        bestName = data.value("init_info").toObject().value("best_uname").toString();
        winCode = winnerType1;
    }
    else // match是自己
    {
        liveService->matchVotes = data.value("init_info").toObject().value("votes").toInt();
        liveService->myVotes = data.value("match_info").toObject().value("votes").toInt();
        bestName = data.value("match_info").toObject().value("best_uname").toString();
        winCode = winnerType2;
    }

    qint64 bestUid = 0;
    if (liveService->myVotes > 0)
    {
        for (int i = liveService->pkGifts.size()-1; i >= 0; i--)
            if (liveService->pkGifts.at(i).getNickname() == bestName)
            {
                bestUid = liveService->pkGifts.at(i).getUid();
                break;
            }
        LiveDanmaku danmaku(bestName, bestUid, winCode, liveService->myVotes);
        triggerCmdEvent("PK_BEST_UNAME", danmaku.with(data), true);
    }
    triggerCmdEvent("PK_END", LiveDanmaku(bestName, bestUid, winCode, liveService->myVotes).with(data), true);

    localNotify(QString("大乱斗 %1：%2 vs %3")
                                     .arg(winCode == 0 ? "平局" : (winCode > 0 ? "胜利" : "失败"))
                                     .arg(liveService->myVotes)
                                     .arg(liveService->matchVotes));
    qInfo() << "大乱斗结束，结果：" << (winCode == 0 ? "平局" : (winCode > 0 ? "胜利" : "失败")) << liveService->myVotes << liveService->matchVotes;
    liveService->myVotes = 0;
    liveService->matchVotes = 0;
    QTimer::singleShot(60000, [=]{
        if (liveService->pking) // 下一把PK，已经清空了
            return ;
        liveService->cmAudience.clear();
    });

    // 保存对面偷塔次数
    if (liveService->oppositeTouta && !liveService->pkUname.isEmpty())
    {
        int count = us->danmakuCounts->value("touta/" + liveService->pkRoomId, 0).toInt();
        us->danmakuCounts->setValue("touta/" + liveService->pkRoomId, count+1);
    }

    // 清空大乱斗数据
    liveService->pking = false;
    liveService->pkEnding = false;
    liveService->pkVoting = 0;
    liveService->pkEndTime = 0;
    liveService->pkUname = "";
    liveService->pkUid = "";
    liveService->pkRoomId = "";
    liveService->myAudience.clear();
    liveService->oppositeAudience.clear();
    liveService->pkVideo = false;
    ui->actionShow_PK_Video->setEnabled(false);

    if (ac->cookieUid == ac->upUid)
        ui->actionJoin_Battle->setEnabled(true);

    if (liveService->pkLiveSocket)
    {
        try {
            if (liveService->pkLiveSocket->state() == QAbstractSocket::ConnectedState)
                liveService->pkLiveSocket->close(); // 会自动deleterLater
            // pkSocket->deleteLater();
        } catch (...) {
            qCritical() << "delete pkSocket failed";
        }
        liveService->pkLiveSocket = nullptr;
    }
}

void MainWindow::slotPkEndingTimeout()
{
    if (!liveService->pking) // 比如换房间了
    {
        qInfo() << "大乱斗结束前，逻辑不正确" << liveService->pking << ac->roomId
                 << QDateTime::currentSecsSinceEpoch() << liveService->pkEndTime;
        return ;
    }

    liveService->pkEndingTimer->stop();

    slotPkEnding();
}

int MainWindow::getPkMaxGold(int votes)
{
    if (!ui->pkAutoMaxGoldCheck->isChecked())
        return liveService->pkMaxGold;
    int money = qMax(0, votes / (1000 / liveService->goldTransPk) - liveService->pkMaxGold / liveService->goldTransPk); // 用于计算的价格
    double prop = pow(money, 1.0/3); // 自动调整的倍率，未限制
    double maxProp = 10.0 / qMax(1, liveService->pkMaxGold / 1000); // 限制最大是10倍；10元及以上则不使用倍率
    maxProp = qMax(1.0, maxProp);
    prop = qMin(qMax(1.0, prop), maxProp); // 限制倍率是 1 ~ 10 倍之间
    if (ui->pkAutoMelonCheck->isChecked() && us->debugPrint)
        qInfo() << ("[偷塔上限 " + snum(votes) + " => " + snum(int(liveService->pkMaxGold * prop)) + "金瓜子, "
                    +QString::number(pow(money, 1.0/3), 'f', 1)+"倍]");
    return int(liveService->pkMaxGold * prop);
}

bool MainWindow::execTouta()
{
    int maxGold = getPkMaxGold(qMax(liveService->myVotes, liveService->matchVotes));
    if (ui->pkAutoMelonCheck->isChecked()
            && liveService->myVotes + liveService->pkVoting <= liveService->matchVotes
            && liveService->myVotes + liveService->pkVoting + maxGold/liveService->goldTransPk > liveService->matchVotes
            /* && oppositeTouta < 6 // 对面之前未连续偷塔（允许被偷塔五次）（可能是连刷，这时候几个吃瓜偷塔没用） */
            && !liveService->toutaBlankList.contains(liveService->pkRoomId)
            && !liveService->magicalRooms.contains(liveService->pkRoomId))
    {
        // 调用送礼
        int giftId = 0, giftNum = 0;
        QString giftName;
        int giftUnit = 0, giftVote = 0; // 单价和单乱斗值
        int needVote = liveService->matchVotes - liveService->myVotes - liveService->pkVoting; // 至少需要多少积分才能不输给对面

        if (!ui->toutaGiftCheck->isChecked()) // 赠送吃瓜（比对面多1分）
        {
            giftId = 20004;
            giftUnit = 100;
            giftVote = giftUnit / liveService->goldTransPk; // 单个吃瓜有多少乱斗值
            giftNum = needVote / giftVote + 1; // 需要多少个礼物

            liveService->chiguaCount += giftNum;
        }
        else // 按类型赠送礼物
        {
            int minCost = 0x3f3f3f3f;
            foreach (auto gift, liveService->toutaGifts)
            {
                int unit = gift.getTotalCoin(); // 单价
                int vote = unit / liveService->goldTransPk; // 相当于多少乱斗值
                int num = needVote / vote + 1; // 起码要这个数量
                int cost = unit * num; // 总价
                if (cost > maxGold) // 总价超过上限
                    continue;

                if (liveService->toutaGiftCounts.size()) // 数量白名单
                {
                    int index = -1;
                    for (int i = 0; i < liveService->toutaGiftCounts.size(); i++)
                    {
                        int count = liveService->toutaGiftCounts.at(i);
                        if (count >= num) // 这个数量可以赢
                        {
                            int cos = unit * count;
                            if (cos > maxGold) // 总价超过上限
                                continue;

                            // 这个可以使用
                            index = i;
                            num = count;
                            cost = cos;
                            break;
                        }
                    }
                    if (index == -1) // 没有合适的数量
                        continue;
                }

                if (cost < minCost || (cost == minCost && num < giftNum)) // 以价格低、数量少的优先
                {
                    giftId = gift.getGiftId();
                    giftName = gift.getGiftName();
                    giftUnit = unit;
                    giftVote = vote;
                    giftNum = num;
                    minCost = cost;
                }
            }
        }

        if (!giftId)
        {
            qWarning() << "没有可赠送的合适礼物：" << needVote << "积分";
            return false;
        }

        sendGift(giftId, giftNum);
        localNotify(QString("[") + (liveService->oppositeTouta ? "反" : "") + "偷塔] " + snum(liveService->matchVotes-liveService->myVotes-liveService->pkVoting+1) + "，赠送 " + snum(giftNum) + " 个" + giftName);
        qInfo() << "大乱斗赠送" << giftNum << "个" << giftName << "：" << liveService->myVotes << "vs" << liveService->matchVotes;
        if (!us->localMode)
            liveService->pkVoting += giftVote * giftNum; // 正在赠送中

        liveService->toutaCount++;
        liveService->toutaGold += giftUnit * giftNum;
        saveTouta();
        return true;
    }
    return false;
}

void MainWindow::trayAction(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::Trigger:
        if (this->isHidden())
        {
            this->showNormal();
            this->activateWindow();
        }
        else
        {
            this->hide();
        }
        break;
    case QSystemTrayIcon::MiddleClick:
        on_actionShow_Live_Danmaku_triggered();
        break;
    case QSystemTrayIcon::Context:
    {
        newFacileMenu;
        menu->addAction(QIcon(":/icons/star"), "主界面", [=]{
            this->show();
            this->activateWindow();
        })->check(this->isVisible());
        menu->split()->addAction(QIcon(":/icons/danmu"), "弹幕姬", [=]{
            on_actionShow_Live_Danmaku_triggered();
        })->check(danmakuWindow && danmakuWindow->isVisible());
        menu->addAction(QIcon(":/icons/order_song"), "点歌姬", [=]{
            on_actionShow_Order_Player_Window_triggered();
        })->check(musicWindow && musicWindow->isVisible());
        menu->addAction(QIcon(":/icons/live"), "视频流", [=]{
            on_actionShow_Live_Video_triggered();
        });
        if (us->localMode || us->debugPrint)
        {
            menu->split();
            menu->addAction(ui->actionLocal_Mode)->hide(!us->localMode);
            menu->addAction(ui->actionDebug_Mode)->hide(!us->debugPrint);
        }

        menu->split();
        if (!hasPermission())
        {
            menu->addAction(QIcon(":/icons/crown"), "免费版", [=]{
                on_actionBuy_VIP_triggered();
            });
        }
        menu->addAction(QIcon(":/icons/cry"), "退出", [=]{
            menu->close();
            prepareQuit();
        })->text(ui->autoUpdateCheck->isChecked() && !rt->appNewVersion.isEmpty(), "中断更新并退出")
          ->text(isFileExist(QApplication::applicationDirPath() + "/update.zip"), "退出并更新");
        menu->exec();
    }
        break;
    default:
        break;
    }
}

void MainWindow::saveMonthGuard()
{
    QDir dir(rt->dataPath + "guard_month");
    dir.mkpath(dir.absolutePath());
    QDate date = QDate::currentDate();
    QString fileName = QString("%1_%2-%3.csv").arg(ac->roomId).arg(date.year()).arg(date.month());
    QString filePath = dir.absoluteFilePath(dir.absoluteFilePath(fileName));

    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file);
    stream.setGenerateByteOrderMark(true);
    if (!liveService->recordFileCodec.isEmpty())
        stream.setCodec(liveService->recordFileCodec.toUtf8());

    stream << QString("UID,昵称,级别,备注\n").toUtf8();
    auto getGuardName = [=](int level) {
        if (level == 1)
            return QString("总督").toUtf8();
        else if (level == 2)
            return QString("提督").toUtf8();
        return QString("舰长").toUtf8();
    };
    for (int i = 0; i < liveService->guardInfos.size(); i++)
    {
        LiveDanmaku danmaku = liveService->guardInfos.at(i);
        stream << danmaku.getUid() << ","
               << danmaku.getNickname() << ","
               << getGuardName(danmaku.getGuard()) << "\n";
    }

    file.close();
}

void MainWindow::saveEveryGuard(LiveDanmaku danmaku)
{
    QDir dir(rt->dataPath + "guard_histories");
    dir.mkpath(dir.absolutePath());
    QString filePath = dir.absoluteFilePath(ac->roomId + ".csv");

    QFile file(filePath);
    bool exists = file.exists();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        qWarning() << "打开上船记录文件失败：" << filePath;
    QTextStream stream(&file);
    stream.setGenerateByteOrderMark(true);
    if (!liveService->recordFileCodec.isEmpty())
        stream.setCodec(liveService->recordFileCodec.toUtf8());

    if (!exists)
        stream << QString("日期,时间,昵称,礼物,数量,累计,UID,备注\n").toUtf8();

    stream << danmaku.getTimeline().toString("yyyy-MM-dd") << ","
           << danmaku.getTimeline().toString("hh:mm") << ","
           << danmaku.getNickname() << ","
           << danmaku.getGiftName() << ","
           << danmaku.getNumber() << ","
           << us->danmakuCounts->value("guard/" + snum(danmaku.getUid()), 0).toInt() << ","
           << danmaku.getUid() << ","
           << us->userMarks->value("base/" + snum(danmaku.getUid()), "").toString() << "\n";

    file.close();
}

void MainWindow::saveEveryGift(LiveDanmaku danmaku)
{
    QDir dir(rt->dataPath + "gift_histories");
    dir.mkpath(dir.absolutePath());
    QDate date = QDate::currentDate();
    QString fileName = QString("%1_%2-%3.csv").arg(ac->roomId).arg(date.year()).arg(date.month());
    QString filePath = dir.absoluteFilePath(fileName);

    QFile file(filePath);
    bool exists = file.exists();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        qWarning() << "打开礼物记录文件失败：" << filePath;
    QTextStream stream(&file);
    stream.setGenerateByteOrderMark(true);
    if (!liveService->recordFileCodec.isEmpty())
        stream.setCodec(liveService->recordFileCodec.toUtf8());

    if (!exists)
        stream << QString("日期,时间,昵称,礼物,数量,金瓜子,UID\n").toUtf8();

    stream << danmaku.getTimeline().toString("yyyy-MM-dd") << ","
           << danmaku.getTimeline().toString("hh:mm") << ","
           << danmaku.getNickname() << ","
           << danmaku.getGiftName() << ","
           << danmaku.getNumber() << ","
           << (danmaku.isGoldCoin() ? danmaku.getTotalCoin() : 0) << ","
           << danmaku.getUid() << "\n";

    file.close();
}

void MainWindow::appendFileLine(QString filePath, QString format, LiveDanmaku danmaku)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        qWarning() << "打开文件失败：" << filePath;
    QTextStream stream(&file);
    stream.setGenerateByteOrderMark(true);
    if (!liveService->codeFileCodec.isEmpty())
        stream.setCodec(liveService->codeFileCodec.toUtf8());
    stream << cr->processDanmakuVariants(format, danmaku) << "\n";
    file.close();
}

void MainWindow::releaseLiveData(bool prepare)
{
    if (!prepare) // 切换房间或者断开连接
    {
        ui->guardCountLabel->setText("0");
        ui->fansCountLabel->setText("0");
        ui->fansClubCountLabel->setText("0");
        ui->roomRankLabel->setText("0");
        ui->roomRankLabel->setToolTip("");
        ui->roomRankTextLabel->setText("热门榜");
        ui->popularityLabel->setText("0");
        ui->danmuCountLabel->setText("0");

        liveService->pking = false;
        liveService->pkUid = "";
        liveService->pkUname = "";
        liveService->pkRoomId = "";
        liveService->pkEnding = false;
        liveService->pkVoting = 0;
        liveService->pkVideo = false;
        liveService->pkTimer->stop();
        liveService->pkEndTime = 0;
        liveService->pkBattleType = 0;
        liveService->pkEndingTimer->stop();
        liveService->myAudience.clear();
        liveService->oppositeAudience.clear();
        liveService->fansList.clear();
        liveService->guardInfos.clear();
        liveService->onlineGoldRank.clear();
        liveService->medalUpgradeWaiting.clear();
        ac->currentFans = 0;
        ac->currentFansClub = 0;
        ac->currentGuards.clear();

        cr->releaseData();
        liveService->roomDanmakus.clear();
        liveService->pkGifts.clear();

        liveService->finishSaveDanmuToFile();

        if (ui->saveRecvCmdsCheck->isChecked())
        {
            ui->saveRecvCmdsCheck->setChecked(false);
            on_saveRecvCmdsCheck_clicked();
        }

        if (pushCmdsFile)
        {
            on_pushRecvCmdsButton_clicked();
        }

        ac->cookieGuardLevel = 0;
        if (ui->adjustDanmakuLongestCheck->isChecked())
            liveService->adjustDanmakuLongest();

        for (int i = 0; i < ui->giftListWidget->count(); i++)
        {
            auto item = ui->giftListWidget->item(i);
            auto widget = ui->giftListWidget->itemWidget(item);
            widget->deleteLater();
        }
        ui->giftListWidget->clear();
        ui->onlineRankListWidget->clear();
        pl->allGiftMap.clear();
    }
    else // 下播，依旧保持连接
    {
    }

    liveService->danmuPopulQueue.clear();
    liveService->minuteDanmuPopul = 0;
    liveService->danmuPopulValue = 0;

    diangeHistory.clear();
    ui->diangeHistoryListWidget->clear();

    statusLabel->setText("");
    liveService->popularVal = 0;

    // 本次直播数据
    liveService->liveAllGifts.clear();
    liveService->liveAllGuards.clear();

    if (danmakuWindow)
    {
        danmakuWindow->hideStatusText();
        danmakuWindow->setIds(0, 0);
        danmakuWindow->releaseLiveData(prepare);
    }

    if (liveService->pkLiveSocket)
    {
        if (liveService->pkLiveSocket->state() == QAbstractSocket::ConnectedState)
            liveService->pkLiveSocket->close();
        liveService->pkLiveSocket = nullptr;
    }

    // 结束游戏
    if (liveOpenService->isPlaying())
    {
        qInfo() << "退出前结束互动玩法...";
        QEventLoop loop;
        connect(liveOpenService, SIGNAL(signalEnd(bool)), &loop, SLOT(quit()));
        liveOpenService->end();
        loop.exec();
    }

    ui->actionShow_Live_Video->setEnabled(false);
    ui->actionShow_PK_Video->setEnabled(false);
    ui->actionJoin_Battle->setEnabled(false);

    finishLiveRecord();
    liveService->saveCalculateDailyData();

    QPixmap face = ac->roomId.isEmpty() ? QPixmap() : PixmapUtil::toCirclePixmap(liveService->upFace);
    setWindowIcon(face);
    tray->setIcon(face);

    // 清理一周没来的用户
    int day = ui->autoClearComeIntervalSpin->value();
    us->danmakuCounts->beginGroup("comeTime");
    QStringList removedKeys;
    auto keys = us->danmakuCounts->allKeys();
    qint64 week = QDateTime::currentSecsSinceEpoch() - day * 24 * 3600;
    foreach (auto key, keys)
    {
        qint64 value = us->danmakuCounts->value(key).toLongLong();
        if (value < week)
        {
            removedKeys.append(key);
        }
    }
    us->danmakuCounts->endGroup();

    us->danmakuCounts->beginGroup("comeTime");
    foreach (auto key, removedKeys)
    {
        us->danmakuCounts->remove(key);
    }
    us->danmakuCounts->endGroup();

    us->danmakuCounts->beginGroup("come");
    foreach (auto key, removedKeys)
    {
        us->danmakuCounts->remove(key);
    }
    us->danmakuCounts->endGroup();
    us->danmakuCounts->sync();
}

QRect MainWindow::getScreenRect()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenRect = screen->availableVirtualGeometry();
    return screenRect;
}

void MainWindow::getPositiveVote()
{
    get("https://api.live.bilibili.com/xlive/virtual-interface/v1/app/detail?app_id=1653383145397", [=](MyJson json) {
        if (json.code() != 0)
        {
            qWarning() << "获取饭贩商店应用失败：" << json;
            return ;
        }
        MyJson data = json.data();
        _fanfanOwn = data.b("own");
        _fanfanLike = data.b("is_like");
        _fanfanLikeCount = data.i("like_count");
        ui->positiveVoteCheck->setChecked(_fanfanLike);
        ui->positiveVoteCheck->setText("好评(" + snum(_fanfanLikeCount) + ")");

        // 获取到的数值可能是有问题的
        if (_fanfanLikeCount == 0)
            return ;

        // 没有评价过的老用户进行提示好评
        if ((QDateTime::currentSecsSinceEpoch() - us->l("runtime/first_use_time") > 3 * 24 * 3600)
                && !us->value("ask/positiveVote", false).toBool()
                && _fanfanLikeCount > 0)
        {
            QTimer::singleShot(3000, [=]{
                if (!ac->cookieUid.isEmpty() && !_fanfanLike)
                {
                    auto noti = new NotificationEntry("positiveVote", "好评", "您是否觉得神奇弹幕好用？");
                    noti->setBtn(1, "好评", "god");
                    noti->setBtn(2, "吐槽", "bad");
                    connect(noti, &NotificationEntry::signalCardClicked, this, [=] {
                        qInfo() << "点击好评卡片";
                        positiveVote();
                    });
                    connect(noti, &NotificationEntry::signalBtnClicked, this, [=](int i) {
                        qInfo() << "点击评价按钮" << i;
                        if (i != 2)
                        {
                            us->setValue("ask/positiveVote", true);
                            positiveVote();
                        }
                        else
                            openLink("http://live.lyixi.com/");
                    });
                    connect(noti, &NotificationEntry::signalTimeout, this, [=] {
                        qInfo() << "默认好评";
                        positiveVote();
                    });
                    noti->setDefaultBtn(1);
                    tip_box->createTipCard(noti);
                }
            });
        }

        // 添加到自己的库中
        if (!_fanfanOwn)
        {
            qInfo() << "饭贩商店获取本应用";
            fanfanAddOwn();
        }
    });
}

/**
 * 饭贩好评
 * 需要 Cookie 带有 PEA_AU 字段
 */
void MainWindow::positiveVote()
{
    QString url = "https://api.live.bilibili.com/xlive/virtual-interface/v1/app/like";
    MyJson json;
    json.insert("app_id", (qint64)1653383145397);
    json.insert("csrf_token", ac->csrf_token);
    json.insert("csrf", ac->csrf_token);
    json.insert("visit_id", "");

    postJson(url, json.toBa(), [=](QNetworkReply* reply){
        QByteArray ba(reply->readAll());
        MyJson json(ba);
        if (json.code() != 0)
        {
            qWarning() << "饭贩好评失败";
            QDesktopServices::openUrl(QUrl("https://play-live.bilibili.com/details/1653383145397"));
        }

        QTimer::singleShot(0, [=]{
            getPositiveVote();
        });
    });
}

void MainWindow::fanfanLogin()
{
    post("https://api.live.bilibili.com/xlive/virtual-interface/v1/user/login", "", [=](QNetworkReply* reply){
        QVariant variantCookies = reply->header(QNetworkRequest::SetCookieHeader);
        QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie> >(variantCookies);
        if (!cookies.size())
        {
            qWarning() << "login 没有返回 set cookie。" << variantCookies;
            return ;
        }
        QNetworkCookie cookie = cookies.at(0);
        QString DataAsString =cookie.toRawForm(); // 转换为QByteArray
        qInfo() << "饭贩连接：" << DataAsString;
        autoSetCookie(ac->browserCookie + "; " + DataAsString);

        QTimer::singleShot(3000, [=]{
            if (!_fanfanLike)
            {
                positiveVote();
            }
        });
    });
}

void MainWindow::fanfanAddOwn()
{
    QString url = "https://api.live.bilibili.com/xlive/virtual-interface/v1/app/addOwn";
    MyJson json;
    json.insert("app_id", (qint64)1653383145397);
    json.insert("csrf_token", ac->csrf_token);
    json.insert("csrf", ac->csrf_token);
    json.insert("visit_id", "");

    postJson(url, json.toBa(), [=](QNetworkReply* reply){
        QByteArray ba(reply->readAll());
        MyJson json(ba);
        if (json.code() == 1666501)
        {
            qInfo() << "饭贩已拥有，无需重复添加";
        }
        else if (json.code() != 0)
        {
            qWarning() << "饭贩添加失败：" << json.msg();
        }
    });
}

void MainWindow::startSplash()
{
#ifndef Q_OS_ANDROID
    if (!ui->startupAnimationCheck->isChecked())
        return ;
    RoundedAnimationLabel* label = new RoundedAnimationLabel(this);
    QMovie* movie = new QMovie(":/icons/star_gif");
    movie->setBackgroundColor(Qt::white);
    label->setMovie(movie);
    label->setStyleSheet("background-color: white;");
    label->setGeometry(this->rect());
    label->setAlignment(Qt::AlignCenter);
    int minSize = qMin(label->width(), label->height());
    movie->setScaledSize(QSize(minSize, minSize));
    int lastFrame = movie->frameCount()-1;
    connect(movie, &QMovie::frameChanged, label, [=](int frameNumber){
        if (frameNumber < lastFrame)
            return ;

        movie->stop();
        QPixmap pixmap(label->size());
        label->render(&pixmap);

        // 开始隐藏
        label->startHide();
        movie->deleteLater();
    });
    label->show();
    movie->start();
#endif
}

void MainWindow::loadWebExtensionList()
{
    /* if (rt->asPlugin)
        return ; */

    // 清空旧的列表
    for (int i = 0; i < ui->extensionListWidget->count(); i++)
    {
        auto item = ui->extensionListWidget->item(i);
        auto widget = ui->extensionListWidget->itemWidget(item);
        widget->deleteLater();
    }
    ui->extensionListWidget->clear();

    // 设置界面值
    QFontMetrics fm(font());
    const int lineHeight = fm.lineSpacing();
    const int btnSize = lineHeight * 3 / 2;
    QFont titleFont;
    titleFont.setPointSize(titleFont.pointSize() * 5 / 4);
    titleFont.setBold(true);

    // 加载url列表，允许一键复制
    QList<QFileInfo> dirs = QDir(webServer->wwwDir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (auto info, dirs)
    {
        QString infoPath = QDir(info.absoluteFilePath()).absoluteFilePath("package.json");
        if (!isFileExist(infoPath))
            infoPath = QDir(info.absoluteFilePath()).absoluteFilePath("info.json");
        if (!isFileExist(infoPath))
            continue;
        QString str = readTextFileAutoCodec(infoPath);
        MyJson json(str.toUtf8());
        QString extName = json.s("name");
        QString authorAll = json.s("author");
        QString descAll = json.s("desc");
        QString minVersion = json.s("min_version");
        if (!minVersion.isEmpty() && rt->appVersion < minVersion)
        {
            showError("扩展不可用", "【" + extName + "】要求版本：v" + minVersion);
        }
        json.each("list", [=](MyJson inf){
            /// 读取数值
            QString name = inf.s("name");
            QString author= inf.s("author");
            QString urlR = inf.s("url"); // 如果是 /开头，则是相对于www的路径，否则相对于当前文件夹的路径
            QString stsR = inf.s("config");
            QString cssR = inf.s("css");
            QString cssC = inf.s("css_custom");
            QString dirR = inf.s("dir");
            QString fileR = inf.s("file");
            QString desc = inf.s("desc");
            QStringList cmds = inf.ss("cmds");
            QJsonValue code = inf.value("code");
            QString coverPath = inf.s("cover");
            QString homepage = inf.s("homepage"); // TODO
            QString reward = inf.s("reward"); // TODO
            if (coverPath.isEmpty())
            {
                if (isFileExist(QDir(info.absoluteFilePath()).absoluteFilePath(coverPath = "cover.png"))
                        || isFileExist(QDir(info.absoluteFilePath()).absoluteFilePath(coverPath = "cover.jpg")))
                    coverPath = QDir(info.absoluteFilePath()).absoluteFilePath(coverPath);
                else
                    coverPath = ":/icons/gray_cover";
            }
            else
            {
                if (isFileExist(coverPath = QDir(info.absoluteFilePath()).absoluteFilePath(coverPath)))
                    ;
                else
                    coverPath = ":/icons/gray_cover";
            }
            if (name.isEmpty())
                name = extName;
            if (author.isEmpty())
                author = authorAll;
            if (desc.isEmpty())
                desc = descAll;
            QString dirName = info.fileName();
            if (!urlR.isEmpty() && !urlR.startsWith("/")) // "/"开头以www为根目录，否则以扩展目录为根目录
                urlR = "/" + dirName + "/" + urlR;
            if (!cssR.isEmpty() && !cssR.startsWith("/"))
                cssR = "/" + dirName + "/" + cssR;
            if (!cssC.isEmpty() && !cssC.startsWith("/"))
                cssC = "/" + dirName + "/" + cssC;
            else if (cssC.isEmpty())
                cssC = cssR;
            if (!dirR.isEmpty() && !dirR.startsWith("/"))
                dirR = "/" + dirName + "/" + dirR;
            if (!fileR.isEmpty() && !fileR.startsWith("/"))
                fileR = "/" + dirName + "/" + fileR;
            if (!stsR.isEmpty() && !stsR.startsWith("/"))
                stsR = "/" + dirName + "/" + stsR;

            if (name.isEmpty() && urlR.isEmpty())
                return;

            /// 创建控件
            const int labelCount = 4;
            const int cardHeight = (lineHeight + 9) * labelCount;
            const int coverSize = lineHeight * 4;
            // 主卡片
            auto widget = new InteractiveButtonBase(ui->serverUrlsCard);
            widget->setPaddings(8);
            widget->setAlign(Qt::AlignLeft);
            widget->setRadius(rt->fluentRadius);
            widget->setFixedForePos();
            if (!desc.isEmpty())
                widget->setToolTip(desc);
            widget->setCursor(Qt::PointingHandCursor);

            // 封面
            auto coverVLayout = new QVBoxLayout;
            QLabel* coverLabel = new RoundedPixmapLabel(widget);
            coverLabel->setFixedSize(coverSize, coverSize);
            coverVLayout->addWidget(coverLabel);
            coverLabel->setPixmap(QPixmap(coverPath));
            coverLabel->setScaledContents(true);

            if (!author.isEmpty())
            {
                QLabel* label = new QLabel("by " + author, widget);
                label->setAlignment(Qt::AlignCenter);
                label->setStyleSheet("color: grey;");
                coverVLayout->addWidget(label);
            }

            // 信息
            auto infoVLayout = new QVBoxLayout;
            QLabel* nameLabel = new QLabel(name, widget);
            nameLabel->setFont(titleFont);
            infoVLayout->addWidget(nameLabel);
            if (!desc.isEmpty())
            {
                QLabel* label = new QLabel(desc, widget);
                label->setStyleSheet("color: grey;");
                infoVLayout->addWidget(label);
            }
            if (!urlR.isEmpty())
            {
                QLabel* label = new QLabel(urlR, widget);
                label->setStyleSheet("color: grey;");
                infoVLayout->addWidget(label);
            }
            infoVLayout->setSpacing(4);

            // 布局
            auto mainHLayout = new QHBoxLayout(widget);
            widget->setLayout(mainHLayout);
            mainHLayout->addLayout(coverVLayout);
            mainHLayout->addLayout(infoVLayout);
            mainHLayout->setSpacing(12);
            mainHLayout->setMargin(9);
            mainHLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            mainHLayout->setStretch(0, 0);
            mainHLayout->setStretch(1, 1);
            widget->setFixedHeight(cardHeight);

            /// 交互按钮
            auto btnHLayout = new QHBoxLayout;
            infoVLayout->addLayout(btnHLayout);

            // 占位符
            auto placehold = new QWidget(widget);
            placehold->setFixedSize(1, btnSize);
            btnHLayout->addWidget(placehold);

            // 右键菜单
            connect(widget, &InteractiveButtonBase::rightClicked, this, [=]{
                newFacileMenu;
                menu->addAction(QIcon(":/icons/directory"), "打开文件夹", [=]{
                    QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()));
                });

                QString url = inf.s("homepage");
                menu->split()->addAction(QIcon(":/icons/room"), "插件主页", [=]{
                    openLink(url);
                })->disable(url.isEmpty());

                url = inf.s("contact");
                menu->addAction(QIcon(":/icons/music_reply"), "联系作者", [=]{
                    openLink(url);
                })->disable(url.isEmpty());

                url = inf.s("reward");
                menu->addAction(QIcon(":/icons/candy"), "打赏", [=]{
                    openLink(url);
                })->disable(url.isEmpty());

                // 自定义菜单
                QJsonArray customMenus = inf.a("menus");
                if (customMenus.size())
                    menu->split();
                for (QJsonValue cm: customMenus)
                {
                    MyJson mn = cm.toObject();
                    QString name = mn.s("name");
                    QString url = mn.s("url");
                    QString icon = mn.s("icon");
                    QString code = mn.s("code");
                    if (!icon.startsWith(":"))
                        icon = QDir(info.absoluteFilePath()).absoluteFilePath(icon);
                    menu->addAction(QIcon(icon), name, [=]{
                        if (!code.isEmpty())
                        {
                            cr->sendVariantMsg(code, LiveDanmaku(), NOTIFY_CD_CN, true);
                        }
                        if (!url.isEmpty())
                            openLink(url);
                    });
                }

                menu->exec();
            });

            // URL
            if (!urlR.isEmpty())
            {
                connect(widget, &InteractiveButtonBase::clicked, this, [=]{
                    if (!ui->serverCheck->isChecked())
                    {
                        shakeWidget(ui->serverCheck);
                        return showError("未开启网络服务", "不可使用" + name);
                    }
                    if (urlR.endsWith(".exe") || urlR.endsWith(".vbs") || urlR.endsWith(".bat"))
                    {
                        qInfo() << "启动程序：" << QDir(webServer->wwwDir).absolutePath() + urlR;
                        QProcess process;
                        process.startDetached(QDir(webServer->wwwDir).absolutePath() + urlR);
                    }
                    else
                    {
                        qInfo() << "打开网址：" << getDomainPort() + urlR;
                        QDesktopServices::openUrl(getDomainPort() + urlR);
                    }
                });

                auto btn = new WaterCircleButton(QIcon(":/icons/copy"), widget);
                btnHLayout->addWidget(btn);
                btn->setFixedSize(btnSize, btnSize);
                btn->setSquareSize();
                btn->setCursor(Qt::PointingHandCursor);
                btn->setFixedForePos();

                if (urlR.endsWith(".exe") || urlR.endsWith(".vbs") || urlR.endsWith(".bat"))
                {
                    btn->setToolTip("本扩展为应用程序，可单独运行");
                    connect(btn, &InteractiveButtonBase::clicked, this, [=]{
                        QProcess process;
                        process.startDetached(QDir(webServer->wwwDir).absolutePath() + urlR);
                    });
                }
                else
                {
                    btn->setToolTip("复制URL，可粘贴到直播姬/OBS的“浏览器”中");
                    connect(btn, &InteractiveButtonBase::clicked, this, [=]{
                        QApplication::clipboard()->setText(getDomainPort() + urlR);
                    });
                }
            }

            // 配置
            if (!stsR.isEmpty())
            {
                auto btn = new WaterCircleButton(QIcon(":/icons/generate"), widget);
                btnHLayout->addWidget(btn);
                btn->setFixedSize(btnSize, btnSize);
                btn->setSquareSize();
                btn->setCursor(Qt::PointingHandCursor);
                btn->setFixedForePos();
                btn->setToolTip("打开扩展的基础配置\n更深层次的设置需要修改源代码");
                // btn->hide();
                // connect(widget, SIGNAL(signalMouseEnter()), btn, SLOT(show()));
                // connect(widget, SIGNAL(signalMouseLeave()), btn, SLOT(hide()));
                connect(btn, &InteractiveButtonBase::clicked, this, [=]{
                    QDesktopServices::openUrl(getDomainPort() + stsR);
                });
            }

            // 导入代码
            if (inf.contains("code") && code.isArray())
            {
                auto btn = new WaterCircleButton(QIcon(":/icons/code"), widget);
                btnHLayout->addWidget(btn);
                btn->setFixedSize(btnSize, btnSize);
                btn->setSquareSize();
                btn->setCursor(Qt::PointingHandCursor);
                btn->setFixedForePos();
                btn->setToolTip("一键导入要用到的代码\n如果已经存在，可能会重复导入");
                // btn->hide();
                // connect(widget, SIGNAL(signalMouseEnter()), btn, SLOT(show()));
                // connect(widget, SIGNAL(signalMouseLeave()), btn, SLOT(hide()));
                connect(btn, &InteractiveButtonBase::clicked, this, [=]{
                    qInfo() << "导入网页小程序代码";
                    QApplication::clipboard()->setText(QJsonDocument(code.toArray()).toJson());
                    on_actionPaste_Code_triggered();
                });
            }

            // 编辑CSS
            if (!cssR.isEmpty())
            {
                if (cssR.startsWith("/"))
                    cssR = cssR.right(cssR.length() - 1);
                if (cssC.startsWith("/"))
                    cssC = cssC.right(cssC.length() - 1);
                auto btn = new WaterCircleButton(QIcon(":/icons/modify"), widget);
                btnHLayout->addWidget(btn);
                btn->setFixedSize(btnSize, btnSize);
                btn->setSquareSize();
                btn->setCursor(Qt::PointingHandCursor);
                btn->setFixedForePos();
                btn->setToolTip("修改或者导入CSS样式");
                // btn->hide();
                // connect(widget, SIGNAL(signalMouseEnter()), btn, SLOT(show()));
                // connect(widget, SIGNAL(signalMouseLeave()), btn, SLOT(hide()));
                connect(btn, &InteractiveButtonBase::clicked, this, [=]{
                    QString path = webServer->wwwDir.absoluteFilePath(cssR);
                    QString pathC = webServer->wwwDir.absoluteFilePath(cssC);
                    qInfo() << "编辑页面：" << path;
                    QString content;
                    if (isFileExist(pathC))
                        content = readTextFile(pathC);
                    else
                        content = readTextFileIfExist(path);
                    bool ok;
                    content = QssEditDialog::getText(this, "设置样式：" + name, "支持CSS样式，修改后刷新页面生效", content, &ok);
                    if (!ok)
                        return ;
                    writeTextFile(pathC, content);
                });
            }

            // 打开目录
            if (!dirR.isEmpty())
            {
                if (dirR.startsWith("/"))
                    dirR = dirR.right(dirR.length() - 1);
                auto btn = new WaterCircleButton(QIcon(":/icons/dir"), widget);
                btnHLayout->addWidget(btn);
                btn->setFixedSize(btnSize, btnSize);
                btn->setSquareSize();
                btn->setCursor(Qt::PointingHandCursor);
                btn->setFixedForePos();
                btn->setToolTip("打开扩展指定目录，修改相关资源");
                // btn->hide();
                // connect(widget, SIGNAL(signalMouseEnter()), btn, SLOT(show()));
                // connect(widget, SIGNAL(signalMouseLeave()), btn, SLOT(hide()));
                connect(btn, &InteractiveButtonBase::clicked, this, [=]{
                    QString path = webServer->wwwDir.absoluteFilePath(dirR);
                    QDesktopServices::openUrl(QUrl(path));
                });
            }

            // 打开文件
            if (!fileR.isEmpty())
            {
                if (fileR.startsWith("/"))
                    fileR = fileR.right(fileR.length() - 1);
                auto btn = new WaterCircleButton(QIcon(":/icons/file"), widget);
                btnHLayout->addWidget(btn);
                btn->setFixedSize(btnSize, btnSize);
                btn->setSquareSize();
                btn->setCursor(Qt::PointingHandCursor);
                btn->setFixedForePos();
                btn->setToolTip("打开扩展指定文件，如抽奖结果");
                // btn->hide();
                // connect(widget, SIGNAL(signalMouseEnter()), btn, SLOT(show()));
                // connect(widget, SIGNAL(signalMouseLeave()), btn, SLOT(hide()));
                connect(btn, &InteractiveButtonBase::clicked, this, [=]{
                    QString path = webServer->wwwDir.absoluteFilePath(fileR);
                    QDesktopServices::openUrl(QUrl(path));
                });
            }

            btnHLayout->addStretch(1);

            auto item = new QListWidgetItem(ui->extensionListWidget);
            ui->extensionListWidget->setItemWidget(item, widget);
            widget->adjustSize();
            item->setSizeHint(widget->size());
        });
    }
}

void MainWindow::shakeWidget(QWidget *widget)
{
    static QList<QWidget*> shakingWidgets;

    if (shakingWidgets.contains(widget))
        return ;

    int nX = widget->x();
    int nY = widget->y();
    QPropertyAnimation *ani = new QPropertyAnimation(widget, "geometry");
    ani->setEasingCurve(QEasingCurve::InOutSine);
    ani->setDuration(500);
    ani->setStartValue(QRect(QPoint(nX,nY), widget->size()));

    int range = 5; // 抖动距离
    int nShakeCount = 20; //抖动次数
    double nStep = 1.0 / nShakeCount;
    for(int i = 1; i < nShakeCount; i++){
        range = i & 1 ? -range : range;
        ani->setKeyValueAt(nStep * i, QRect(QPoint(nX + range, nY), widget->size()));
    }

    ani->setEndValue(QRect(QPoint(nX,nY), widget->size()));
    ani->start(QAbstractAnimation::DeleteWhenStopped);

    shakingWidgets.append(widget);
    connect(ani, &QPropertyAnimation::stateChanged, this, [=](QAbstractAnimation::State state){
        if (state == QPropertyAnimation::State::Stopped)
        {
            shakingWidgets.removeOne(widget);
        }
    });
}

void MainWindow::saveGameNumbers(int channel)
{
    auto list = cr->gameNumberLists[channel];
    QStringList sl;
    foreach (qint64 val, list)
        sl << snum(val);
    cr->heaps->setValue("game_numbers/r" + snum(channel), sl.join(";"));
}

void MainWindow::restoreGameNumbers()
{
    for (int i = 0; i < CHANNEL_COUNT; i++)
    {
        if (!cr->heaps->contains("game_numbers/r" + snum(i)))
            continue;

        QStringList sl = cr->heaps->value("game_numbers/r" + snum(i)).toString().split(";");
        auto& list = cr->gameNumberLists[i];
        list.clear();
        foreach (QString s, sl)
            list << s.toLongLong();
    }
}

void MainWindow::saveGameTexts(int channel)
{
    auto list = cr->gameTextLists[channel];
    cr->heaps->setValue("game_texts/r" + snum(channel), list.join(MAGICAL_SPLIT_CHAR));
}

void MainWindow::restoreGameTexts()
{
    for (int i = 0; i < CHANNEL_COUNT; i++)
    {
        if (!cr->heaps->contains("game_texts/r" + snum(i)))
            continue;

        QStringList sl = cr->heaps->value("game_texts/r" + snum(i)).toString().split(MAGICAL_SPLIT_CHAR);
        cr->gameTextLists[i] = sl;
    }
}

void MainWindow::setUrlCookie(const QString &url, QNetworkRequest *request)
{
    if (url.contains("bilibili.com") && !ac->browserCookie.isEmpty())
        request->setHeader(QNetworkRequest::CookieHeader, ac->userCookies);
    if (url.contains("ai.baidu.com"))
        request->setRawHeader("Referer", "https://ai.baidu.com/tech/speech/tts_online");
}

void MainWindow::openLink(QString link)
{
    if (!link.startsWith("this://"))
    {
        QDesktopServices::openUrl(link);
        return ;
    }

    link = link.right(link.length() - 7);

    if (link == "buy_vip") // 购买VIP
    {
        on_actionBuy_VIP_triggered();
    }
    else if (link == "capture_manager") // 截图管理
    {
        on_actionPicture_Browser_triggered();
    }
    else if (link == "start_pk") // 开始大乱斗
    {
        on_actionJoin_Battle_triggered();
    }
    else if (link == "guard_catch") // 黑听舰长捕捉
    {
        on_actionGuard_Online_triggered();
    }
    else if (link == "catch_you") // 跑骚捕捉
    {
        on_actionCatch_You_Online_triggered();
    }
    else if (link == "live_status") // 直播间状态查询
    {
        on_actionRoom_Status_triggered();
    }
    else if (link == "lyrics") // 发送歌词
    {
        on_actionCreate_Video_LRC_triggered();
    }
    else if (link == "video_stream_url") // 拷贝视频流地址
    {
        on_actionGet_Play_Url_triggered();
    }
    else if (link == "donation") // 捐赠
    {
        on_actionSponsor_triggered();
    }
    else if (link == "copy_qq") // 复制QQ群
    {
        QApplication::clipboard()->setText("1038738410");
    }
    else if (link == "live_danmaku")
    {
        on_actionShow_Live_Danmaku_triggered();
    }
    else if (link == "live_video")
    {
        on_actionShow_Live_Video_triggered();
    }
    else if (link == "pk_live_video")
    {
        on_actionShow_PK_Video_triggered();
    }
    else if (link == "music")
    {
        on_actionShow_Order_Player_Window_triggered();
    }
    else if (link == "send_long_text")
    {
        on_actionSend_Long_Text_triggered();
    }
    else if (link == "qq_qrcode")
    {
        // 直接把下面的图片设置成这个二维码的
        ui->label_39->setStyleSheet("border-image:url(:/documents/qq_qrcode);\
                                    background-position:center;\
                                    background-repeat:none;");
    }
    else if (link == "gift_list")
    {
        on_actionShow_Gift_List_triggered();
    }
}

/// 显示礼物在界面上
/// 只显示超过一定金额的礼物
void MainWindow::addGuiGiftList(const LiveDanmaku &danmaku)
{
    if (us->closeGui)
        return ;

    // 设置下限
    if (!danmaku.isGoldCoin() || danmaku.getTotalCoin() < 1000) // 超过1元才算
        return ;

    // 测试代码：
    // addGuiGiftList(LiveDanmaku("测试用户", 30607, "测试心心", qrand() % 20, 123456, QDateTime::currentDateTime(), "gold", 10000));

    auto addToList = [=](const QPixmap& pixmap){
        // 创建控件
        InteractiveButtonBase* card = new InteractiveButtonBase(this);
        QVBoxLayout* layout = new QVBoxLayout(card);
        QLabel* imgLabel = new QLabel(card);
        QLabel* giftNameLabel = new QLabel(danmaku.getGiftName(), card);
        QLabel* userNameLabel = new QLabel(danmaku.getNickname(), card);
        layout->addWidget(imgLabel);
        layout->addWidget(giftNameLabel);
        layout->addWidget(userNameLabel);
        card->setLayout(layout);
        card->show();

        layout->setSpacing(1);
        layout->setMargin(3);
        card->setObjectName("giftCard");
        imgLabel->setFixedHeight(rt->giftImgSize);
        imgLabel->setAlignment(Qt::AlignCenter);
        giftNameLabel->setAlignment(Qt::AlignCenter);
        userNameLabel->setAlignment(Qt::AlignCenter);
        if (danmaku.getNumber() > 1)
        {
            giftNameLabel->setText(danmaku.getGiftName() + "×" + snum(danmaku.getNumber()));
            giftNameLabel->adjustSize();
        }
        card->setToolTip("价值：" + snum(danmaku.getTotalCoin() / 1000) + "元");

        // 获取图片
        imgLabel->setPixmap(pixmap.scaled(rt->giftImgSize, rt->giftImgSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        // 添加到列表
        auto item = new QListWidgetItem();
        ui->giftListWidget->insertItem(0, item);
        ui->giftListWidget->setItemWidget(item, card);

        // 设置样式
        /* card->setStyleSheet("#giftCard { background: transparent;"
                            "border: 0px solid lightgray; "
                            "border-radius: " + snum(rt->fluentRadius) + "px; }"
                            "#giftCard:hover { border: 1px solid lightgray; }"); */
        giftNameLabel->setStyleSheet("font-size: 14px;");
        userNameLabel->setStyleSheet("color: gray;");

        // layout->activate();
        card->setRadius(rt->fluentRadius);
        // card->setFixedSize(card->sizeHint());
        imgLabel->adjustSize();
        giftNameLabel->adjustSize();
        userNameLabel->adjustSize();
        int height = imgLabel->height() + giftNameLabel->height() + userNameLabel->height() + layout->spacing() * 2 + layout->margin() * 2;
        card->setFixedSize(imgLabel->width() + layout->margin() * 2, height);
        item->setSizeHint(card->size());
        if (giftNameLabel->sizeHint().width() > imgLabel->width())
            giftNameLabel->setAlignment(Qt::AlignLeft);
        if (userNameLabel->sizeHint().width() > imgLabel->width())
            userNameLabel->setAlignment(Qt::AlignLeft);
        if (ui->giftListWidget->height() < card->height())
            ui->giftListWidget->setFixedHeight(card->height());
    };

    // 移除过多的
    int giftListMax = 100; // 最大保留几个（过多可能会占内存）
    if (ui->giftListWidget->count() >= giftListMax)
    {
        auto item = ui->giftListWidget->item(ui->giftListWidget->count() - 1);
        auto widget = ui->giftListWidget->itemWidget(item);
        widget->deleteLater();
        ui->giftListWidget->takeItem(ui->giftListWidget->count() - 1);
    }

    // 本地已有，直接设置
    qint64 id = danmaku.getGiftId();
    if (pl->giftPixmaps.contains(id))
    {
        addToList(pl->giftPixmaps[id]);
        return ;
    }

    // 先下载图片，然后进行设置
    // TODO: 这个URL的图片有些是不准确的
    QString url;
    if (pl->allGiftMap.contains(id))
        url = pl->allGiftMap.value(id).getFaceUrl();
    if (url.isEmpty())
    {
        url = "http://openapi.zbmate.com/gift_icons/b/";
        qWarning() << "无法获取礼物图片：" << id << "   使用接口" << url << "代替";
    }
    if (danmaku.isGuard()) // 舰长礼物的话，是静态的图片
    {
        int guard = danmaku.getGuard();
        QString name = "jz";
        if (guard == 2)
            name = "td";
        else if (guard == 1)
            name = "zd";
        url += name + ".png";
    }
    else
    {
        if (pl->allGiftMap.contains(id))
            url = pl->allGiftMap.value(id).extraJson.value("img_basic").toString();
        else
            url += snum(id) + ".gif";
    }
    get(url, [=](QNetworkReply* reply){
        QByteArray jpegData = reply->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(jpegData);
        if (pixmap.isNull())
        {
            showError("获取礼物图片出错", danmaku.getGiftName() + " " + snum(danmaku.getGiftId()));
            qWarning() << "礼物地址：" << url;
            return ;
        }

        pl->giftPixmaps[id] = pixmap;
        addToList(pixmap);
    });
}

QString MainWindow::GetFileVertion(QString fullName)
{
#if defined(Q_OS_WIN)
    DWORD dwLen = 0;
    char* lpData=NULL;
    LPCWSTR  str_path;
    str_path=fullName.toStdWString().c_str();
    BOOL bSuccess = FALSE;
    QString fileInfomation;
    DWORD vHandle=0;
    //获得文件基础信息
    //--------------------------------------------------------
    dwLen = GetFileVersionInfoSize( str_path, &vHandle);
    if (0 == dwLen)
    {
        qCritical()<<"获取版本字节信息失败!";
        return "";
    }

    lpData =(char*)malloc(dwLen+1);
    if (NULL == lpData)
    {
        qCritical()<<"分配内存失败";
        return "";
    }
    bSuccess = GetFileVersionInfo( fullName.toStdWString().c_str(),0, dwLen+1, lpData);
    if (!bSuccess)
    {
        qCritical()<<"获取文件版本信息错误!";
        return "";
    }
    LPVOID lpBuffer = NULL;
    UINT uLen = 0;

    //获得语言和代码页(language and code page),规定，套用即可
    //---------------------------------------------------
    bSuccess = VerQueryValue( lpData,
                              (TEXT("\\VarFileInfo\\Translation")),
                              &lpBuffer,
                              &uLen);
    QString strTranslation,str1,str2;
    unsigned short int *p =(unsigned short int *)lpBuffer;
    str1.setNum(*p,16);
    str1="000"+ str1;
    strTranslation+= str1.mid(str1.size()-4,4);
    str2.setNum(*(++p),16);
    str2="000"+ str2;
    strTranslation+= str2.mid(str2.size()-4,4);

    QString str_value;
    QString code;
    //以上步骤需按序进行，以下步骤可根据需要增删或者调整

    //获得产品版本信息：ProductVersion
    code ="\\StringFileInfo\\"+ strTranslation +"\\ProductVersion";
    bSuccess = VerQueryValue(lpData,
                             (code.toStdWString().c_str()),
                             &lpBuffer,
                             &uLen);
    if (!bSuccess)
    {
        qCritical()<<"获取产品版本信息错误!";
    }
    else
    {
        str_value=QString::fromUtf16((const unsigned short int *)lpBuffer)+"\n";
    }
    return str_value;
#else
    QString text = readTextFileAutoCodec(":/documents/resource.rc");
    QString key = "VALUE \"ProductVersion\",";
    int pos = text.indexOf(key);
    if (pos != -1)
    {
        int right = text.indexOf("\n", pos);
        if (right == -1)
            right = text.length();
        QString version = text.mid(pos + key.length(), right - pos - key.length()).trimmed();
        if (version.startsWith("\"") && version.endsWith("\""))
            version = version.mid(1, version.length() - 2);
        return version;
    }
    return "";
#endif
}

/**
 * 升级版本所所需要修改的数据
 */
void MainWindow::upgradeVersionToLastest(QString oldVersion)
{
    if (oldVersion.startsWith("v") || oldVersion.startsWith("V"))
        oldVersion.replace(0, 1, "");
    QStringList versions = {
        "3.6.3",
        "4.6.0",
        "4.8.2",
        rt->appVersion // 最后一个一定是最新版本
    };
    int index = 0;
    while (index < versions.size())
    {
        if (versions.at(index) < oldVersion)
            index++;
        else
            break;
    }
    for (int i = index; i < versions.size() - 1; i++)
    {
        QString ver = versions.at(i);
        qInfo() << "数据升级：" << ver << " -> " << versions.at(i+1);
        upgradeOneVersionData(ver);
        sqlService.upgradeDb(versions.at(i+1));
    }
}

void MainWindow::upgradeOneVersionData(QString beforeVersion)
{
    if (beforeVersion == "3.6.3")
    {
        us->beginGroup("heaps");
        cr->heaps->beginGroup("heaps");
        auto keys = us->allKeys();
        for (int i = 0; i < keys.size(); i++)
        {
            QString key = keys.at(i);
            cr->heaps->setValue(key, us->value(key));
            us->remove(key);
        }
        cr->heaps->endGroup();
        us->endGroup();
    }
    else if (beforeVersion == "4.6.0")
    {
        us->setValue("runtime/first_use_time", QDateTime::currentSecsSinceEpoch());
    }
    else if (beforeVersion == "4.8.2")
    {
        deleteDir(rt->dataPath + "musics");
    }
}

bool MainWindow::hasInstallVC2015()
{
#ifdef Q_OS_WINDOWS
    QString header = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
    QSettings reg(header,QSettings::NativeFormat);
    QMap<QString,QString> m_data;
    QStringList sum = reg.allKeys();
    for(int m  = 0 ; m < sum.size();++m){
        QString id = sum.at(m);
        int end = id.indexOf("}");
        if(end > 0){
            id = id.mid(0,end+1);

            if(!m_data.keys().contains(id)){
                QSettings gt(header + id,QSettings::NativeFormat);
                QString name = gt.value("DisplayName").toString();
                if(!name.isEmpty() && name.contains("Microsoft Visual C++")){
                    m_data[id] = name;
                }

            }

        }
    }

    QMap<QString,QString>::const_iterator it = m_data.constBegin();
    while (it != m_data.constEnd()) {
        if(it.value().contains(QRegExp("Microsoft Visual C\\+\\+ 20(1[56789]|[2-9])")))
            return true;
        ++it;

    }
#endif
    return false;
}

void MainWindow::generateDefaultCode(QString path)
{
    MyJson json;
    json.insert("welcome", ui->autoWelcomeWordsEdit->toPlainText());
    json.insert("gift", ui->autoThankWordsEdit->toPlainText());
    json.insert("attention", ui->autoAttentionWordsEdit->toPlainText());

    QJsonArray array;
    for (int row = 0; row < ui->taskListWidget->count(); row++)
        array.append(static_cast<TaskWidget*>(ui->taskListWidget->itemWidget(ui->taskListWidget->item(row)))->toJson());
    json.insert("timer_task", array);

    array = QJsonArray();
    for (int row = 0; row < ui->replyListWidget->count(); row++)
        array.append(static_cast<ReplyWidget*>(ui->replyListWidget->itemWidget(ui->replyListWidget->item(row)))->toJson());
    json.insert("auto_reply", array);

    array = QJsonArray();
    for (int row = 0; row < ui->eventListWidget->count(); row++)
        array.append(static_cast<EventWidget*>(ui->eventListWidget->itemWidget(ui->eventListWidget->item(row)))->toJson());
    json.insert("event_action", array);

    json.insert("block_keys", ui->autoBlockNewbieKeysEdit->toPlainText());
    json.insert("block_notify", ui->autoBlockNewbieNotifyWordsEdit->toPlainText());
    json.insert("block_tip", ui->promptBlockNewbieKeysEdit->toPlainText());

    if (path.isEmpty())
        path = QApplication::applicationDirPath() + "/default_code.json";
    writeTextFile(path, QString::fromUtf8(json.toBa()));
}

void MainWindow::readDefaultCode(QString path)
{
    if (path.isEmpty())
    {
        path = QApplication::applicationDirPath() + "/default_code.json";
        if (!QFileInfo(path).exists())
            path = ":/documents/default_code";
    }

    QString text = readTextFileIfExist(path);
    MyJson json(text.toUtf8());
    if (json.contains("welcome"))
    {
        ui->autoWelcomeWordsEdit->setPlainText(json.s("welcome"));
    }
    if (json.contains("gift"))
    {
        ui->autoThankWordsEdit->setPlainText(json.s("gift"));
    }
    if (json.contains("attention"))
    {
        ui->autoAttentionWordsEdit->setPlainText(json.s("attention"));
    }
    if (json.contains("timer_task"))
    {
        json.each("timer_task", [=](MyJson obj){
            addTimerTask(obj);
        });
        saveTaskList();
    }
    if (json.contains("auto_reply"))
    {
        json.each("auto_reply", [=](MyJson obj){
            addAutoReply(obj);
        });
        saveReplyList();
    }
    if (json.contains("event_action"))
    {
        json.each("event_action", [=](MyJson obj){
            addEventAction(obj);
        });
        saveEventList();
    }
    if (json.contains("block_keys"))
    {
        ui->autoBlockNewbieKeysEdit->setPlainText(json.s("block_keys"));
    }
    if (json.contains("block_notify"))
    {
        ui->autoBlockNewbieNotifyWordsEdit->setPlainText(json.s("block_notify"));
    }
    if (json.contains("block_tip"))
    {
        ui->promptBlockNewbieKeysEdit->setPlainText(json.s("block_tip"));
    }
}

void MainWindow::showError(QString title, QString s) const
{
    statusLabel->setText(title + ": " + s);
    qWarning() << title << ":" << s;
    tip_box->createTipCard(new NotificationEntry("", title, s));
}

void MainWindow::showError(QString s) const
{
    statusLabel->setText(s);
    qWarning() << s;
    tip_box->createTipCard(new NotificationEntry("", "", s));
}

void MainWindow::showNotify(QString title, QString s) const
{
    statusLabel->setText(title + ": " + s);
    qInfo() << title << ":" << s;
    tip_box->createTipCard(new NotificationEntry("", title, s));
}

void MainWindow::showNotify(QString s) const
{
    statusLabel->setText(s);
    qInfo() << s;
    tip_box->createTipCard(new NotificationEntry("", "", s));
}

void MainWindow::initLiveOpenService()
{
    if (liveOpenService)
        return ;

    // 初始化
    liveOpenService = new BiliLiveOpenService(this);
}

void MainWindow::initDbService()
{
    // TODO: 可更换路径
    sqlService.setDbPath(rt->dataPath + "database.db");
    if (saveToSqlite)
        sqlService.open();

    connect(this, &MainWindow::signalNewDanmaku, this, [=](const LiveDanmaku &danmaku){
        if (danmaku.isPkLink()) // 不包含PK同步的弹幕
            return ;
        if (danmaku.isNoReply() && !danmaku.isAutoSend()) // 不包含自动发送的弹幕
            return ;
        if (saveToSqlite)
            sqlService.insertDanmaku(danmaku);
    });
    connect(&sqlService, &SqlService::signalError, this, [=](const QString& err) {
        showError("数据库", err);
    });
}

void MainWindow::showSqlQueryResult(QString sql)
{
    DBBrowser* dbb = new DBBrowser(&sqlService, us, nullptr);
    dbb->setGeometry(this->geometry());
    connect(dbb, &DBBrowser::signalProcessVariant, this, [=](QString& code) {
        code = cr->processDanmakuVariants(code, LiveDanmaku());
    });
    if (!sql.isEmpty())
    {
        dbb->showQueryResult(sql);
    }
    dbb->show();
}

void MainWindow::on_actionMany_Robots_triggered()
{
    if (!liveService->hostList.size()) // 未连接
        return ;

    HostInfo hostServer = liveService->hostList.at(0);
    QString host = QString("wss://%1:%2/sub").arg(hostServer.host).arg(hostServer.wss_port);

    QSslConfiguration config = liveService->liveSocket->sslConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setProtocol(QSsl::TlsV1SslV3);

    for (int i = 0; i < 1000; i++)
    {
        QWebSocket* socket = new QWebSocket();
        connect(socket, &QWebSocket::connected, this, [=]{
            qInfo() << "rSocket" << i << "connected";
            // 5秒内发送认证包
            liveService->sendVeriPacket(socket, liveService->pkRoomId, liveService->pkToken);
        });
        connect(socket, &QWebSocket::disconnected, this, [=]{
            robots_sockets.removeOne(socket);
            socket->deleteLater();
        });
        socket->setSslConfiguration(config);
        socket->open(host);
        robots_sockets.append(socket);
    }
}

void MainWindow::on_judgeRobotCheck_clicked()
{
    judgeRobot = (judgeRobot + 1) % 3;
    if (judgeRobot == 0)
        ui->judgeRobotCheck->setCheckState(Qt::Unchecked);
    else if (judgeRobot == 1)
        ui->judgeRobotCheck->setCheckState(Qt::PartiallyChecked);
    else if (judgeRobot == 2)
        ui->judgeRobotCheck->setCheckState(Qt::Checked);
    ui->judgeRobotCheck->setText(judgeRobot == 1 ? "机器人判断(仅关注)" : "机器人判断");

    us->setValue("danmaku/judgeRobot", judgeRobot);
}

void MainWindow::on_actionAdd_Room_To_List_triggered()
{
    if (ac->roomId.isEmpty())
        return ;

    QStringList list = us->value("custom/rooms", "").toString().split(";", QString::SkipEmptyParts);
    for (int i = 0; i < list.size(); i++)
    {
        QStringList texts = list.at(i).split(",", QString::SkipEmptyParts);
        if (texts.size() < 1)
            continue ;
        QString id = texts.first();
        QString name = texts.size() >= 2 ? texts.at(1) : id;
        if (id == ac->roomId) // 找到这个
        {
            ui->menu_3->removeAction(ui->menu_3->actions().at(ui->menu_3->actions().size() - (list.size() - i)));
            list.removeAt(i);
            us->setValue("custom/rooms", list.join(";"));
            return ;
        }
    }

    QString id = ac->roomId; // 不能用成员变量，否则无效（lambda中值会变，自己试试就知道了）
    QString name = ac->upName;

    list.append(id + "," + name.replace(";","").replace(",",""));
    us->setValue("custom/rooms", list.join(";"));

    QAction* action = new QAction(name, this);
    ui->menu_3->addAction(action);
    connect(action, &QAction::triggered, this, [=]{
        ui->roomIdEdit->setText(id);
        on_roomIdEdit_editingFinished();
    });
}

void MainWindow::on_recordCheck_clicked()
{
    bool check = ui->recordCheck->isChecked();
    us->setValue("record/enabled", check);

    if (check)
    {
        if (!ac->roomId.isEmpty() && liveService->isLiving())
            startLiveRecord();
    }
    else
        finishLiveRecord();
}

void MainWindow::on_recordSplitSpin_valueChanged(int arg1)
{
    us->setValue("record/split", arg1);
    if (recordTimer)
        recordTimer->setInterval(arg1 * 60000);
}

void MainWindow::on_sendWelcomeTextCheck_clicked()
{
    us->setValue("danmaku/sendWelcomeText", ui->sendWelcomeTextCheck->isChecked());
}

void MainWindow::on_sendWelcomeVoiceCheck_clicked()
{
    us->setValue("danmaku/sendWelcomeVoice", ui->sendWelcomeVoiceCheck->isChecked());
#if defined(ENABLE_TEXTTOSPEECH)
    if (!voiceService->tts && ui->sendWelcomeVoiceCheck->isChecked())
        initTTS();
#endif
}

void MainWindow::on_sendGiftTextCheck_clicked()
{
    us->setValue("danmaku/sendGiftText", ui->sendGiftTextCheck->isChecked());
}

void MainWindow::on_sendGiftVoiceCheck_clicked()
{
    us->setValue("danmaku/sendGiftVoice", ui->sendGiftVoiceCheck->isChecked());
#if defined(ENABLE_TEXTTOSPEECH)
    if (!voiceService->tts && ui->sendGiftVoiceCheck->isChecked())
        initTTS();
#endif
}

void MainWindow::on_sendAttentionTextCheck_clicked()
{
    us->setValue("danmaku/sendAttentionText", ui->sendAttentionTextCheck->isChecked());
}

void MainWindow::on_sendAttentionVoiceCheck_clicked()
{
    us->setValue("danmaku/sendAttentionVoice", ui->sendAttentionVoiceCheck->isChecked());
#if defined(ENABLE_TEXTTOSPEECH)
    if (!voiceService->tts && ui->sendAttentionVoiceCheck->isChecked())
        initTTS();
#endif
}

void MainWindow::on_enableScreenDanmakuCheck_clicked()
{
    us->setValue("screendanmaku/enableDanmaku", ui->enableScreenDanmakuCheck->isChecked());
    ui->enableScreenMsgCheck->setEnabled(ui->enableScreenDanmakuCheck->isChecked());
    ui->screenDanmakuWithNameCheck->setEnabled(ui->enableScreenDanmakuCheck->isChecked());
}

void MainWindow::on_enableScreenMsgCheck_clicked()
{
    us->setValue("screendanmaku/enableMsg", ui->enableScreenMsgCheck->isChecked());
}

void MainWindow::on_screenDanmakuWithNameCheck_clicked()
{
    us->setValue("screendanmaku/showName", ui->screenDanmakuWithNameCheck->isChecked());
}

void MainWindow::on_screenDanmakuLeftSpin_valueChanged(int arg1)
{
    us->setValue("screendanmaku/left", arg1);
}

void MainWindow::on_screenDanmakuRightSpin_valueChanged(int arg1)
{
    us->setValue("screendanmaku/right", arg1);
}

void MainWindow::on_screenDanmakuTopSpin_valueChanged(int arg1)
{
    us->setValue("screendanmaku/top", arg1);
}

void MainWindow::on_screenDanmakuBottomSpin_valueChanged(int arg1)
{
    us->setValue("screendanmaku/bottom", arg1);
}

void MainWindow::on_screenDanmakuSpeedSpin_valueChanged(int arg1)
{
    us->setValue("screendanmaku/speed", arg1);
}

void MainWindow::on_screenDanmakuFontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok,this);
    if (!ok)
        return ;
    this->screenDanmakuFont = font;
    this->setFont(font);
    us->setValue("screendanmaku/font", screenDanmakuFont.toString());
}

void MainWindow::on_screenDanmakuColorButton_clicked()
{
    QColor c = QColorDialog::getColor(screenDanmakuColor, this, "选择昵称颜色", QColorDialog::ShowAlphaChannel);
    if (!c.isValid())
        return ;
    if (c != screenDanmakuColor)
    {
        us->setValue("screendanmaku/color", screenDanmakuColor = c);
    }
}

void MainWindow::on_autoSpeekDanmakuCheck_clicked()
{
    us->setValue("danmaku/autoSpeek", ui->autoSpeekDanmakuCheck->isChecked());
#if defined(ENABLE_TEXTTOSPEECH)
    if (!voiceService->tts && ui->autoSpeekDanmakuCheck->isChecked())
        initTTS();
#endif
}

void MainWindow::on_diangeFormatEdit_textEdited(const QString &text)
{
    diangeFormatString = text;
    us->setValue("danmaku/diangeFormat", diangeFormatString);
}

void MainWindow::on_diangeNeedMedalCheck_clicked()
{
    us->setValue("danmaku/diangeNeedMedal", ui->diangeNeedMedalCheck->isChecked());
}

void MainWindow::on_showOrderPlayerButton_clicked()
{
    on_actionShow_Order_Player_Window_triggered();
}

void MainWindow::on_diangeShuaCheck_clicked()
{
    us->setValue("danmaku/diangeShua", ui->diangeShuaCheck->isChecked());
}

void MainWindow::on_orderSongShuaSpin_editingFinished()
{
    us->setValue("danmaku/diangeShuaCount", ui->orderSongShuaSpin->value());
}

void MainWindow::on_pkMelonValButton_clicked()
{
    bool ok = false;
    int val = QInputDialog::getInt(this, "乱斗值比例", "1乱斗值等于多少金瓜子？10或100？", liveService->goldTransPk, 1, 100, 1, &ok);
    if (!ok)
        return ;
    liveService->goldTransPk = val;
    us->setValue("pk/goldTransPk", liveService->goldTransPk);
}

void MainWindow::slotPkEnding()
{
    qInfo() << "大乱斗结束前情况：" << liveService->myVotes << liveService->matchVotes
             << QDateTime::currentSecsSinceEpoch() << liveService->pkEndTime;

    liveService->pkEnding = true;
    liveService->pkVoting = 0;

    // 几个吃瓜就能解决的……
    {
        QString text = QString("大乱斗尾声：%1 vs %2")
                .arg(liveService->myVotes).arg(liveService->matchVotes);
        if (!liveService->pkRoomId.isEmpty())
        {
            int totalCount = us->danmakuCounts->value("pk/" + liveService->pkRoomId, 0).toInt() - 1;
            int toutaCount = us->danmakuCounts->value("touta/" + liveService->pkRoomId, 0).toInt();
            if (totalCount > 1) // 开始的时候就已经+1了，上面已经-1
                text += QString("  偷塔概率:%1/%2")
                                .arg(toutaCount).arg(totalCount);
        }

        // localNotify(text);
    }
    execTouta();

    triggerCmdEvent("PK_ENDING", LiveDanmaku(), true);
}

/**
 * 机器人开始工作
 * 开播时连接/连接后开播
 * 不包括视频大乱斗的临时开启
 */
void MainWindow::slotStartWork()
{
    // 初始化开播数据
    liveService->liveTimestamp = QDateTime::currentMSecsSinceEpoch();

    // 自动更换勋章
    if (ui->autoSwitchMedalCheck->isChecked())
    {
        // switchMedalToRoom(roomId.toLongLong());
        liveService->switchMedalToUp(ac->upUid);
    }

    ui->actionShow_Live_Video->setEnabled(true);

    // 赠送过期礼物
    if (ui->sendExpireGiftCheck->isChecked())
    {
        sendExpireGift();
    }

    // 延迟操作（好像是有bug）
    QString ri = ac->roomId;
    QTimer::singleShot(5000, [=]{
        if (ri != ac->roomId)
            return ;

        // 挂小心心
        if (ui->acquireHeartCheck->isChecked() && liveService->isLiving())
        {
            liveService->startHeartConnection();
        }
    });

    // 设置直播状态
    QPixmap face = PixmapUtil::toLivingPixmap(PixmapUtil::toCirclePixmap(liveService->upFace));
    setWindowIcon(face);
    tray->setIcon(face);

    // 开启弹幕保存（但是之前没有开启，怕有bug）
    if (us->saveDanmakuToFile && !liveService->danmuLogFile)
        liveService->startSaveDanmakuToFile();

    // 同步所有的使用房间，避免使用神奇弹幕的偷塔误杀
    QString usedRoom = ac->roomId;
    syncTimer->start((qrand() % 10 + 10) * 1000);

    // 本次直播数据
    liveService->liveAllGifts.clear();

    // 获取舰长
    liveService->updateExistGuards(0);

    triggerCmdEvent("START_WORK", LiveDanmaku(), true);

    // 云同步
    pullRoomShieldKeyword();

    // 开始工作
    if (ui->liveOpenCheck->isChecked())
    {
        liveOpenService->start();
    }
}

void MainWindow::on_autoSwitchMedalCheck_clicked()
{
    us->setValue("danmaku/autoSwitchMedal", ui->autoSwitchMedalCheck->isChecked());
    if (!ac->roomId.isEmpty() && liveService->isLiving())
    {
        // switchMedalToRoom(roomId.toLongLong());
        liveService->switchMedalToUp(ac->upUid);
    }
}

void MainWindow::on_sendAutoOnlyLiveCheck_clicked()
{
    us->setValue("danmaku/sendAutoOnlyLive", ui->sendAutoOnlyLiveCheck->isChecked());
}

void MainWindow::on_autoDoSignCheck_clicked()
{
    us->setValue("danmaku/autoDoSign", us->autoDoSign = ui->autoDoSignCheck->isChecked());
}

void MainWindow::on_actionRoom_Status_triggered()
{
    RoomStatusDialog* rsd = new RoomStatusDialog(us, rt->dataPath, nullptr);
    rsd->show();
}

void MainWindow::on_autoLOTCheck_clicked()
{
    us->setValue("danmaku/autoLOT", us->autoJoinLOT = ui->autoLOTCheck->isChecked());
}

void MainWindow::on_blockNotOnlyNewbieCheck_clicked()
{
    bool enable = ui->blockNotOnlyNewbieCheck->isChecked();
    us->setValue("block/blockNotOnlyNewbieCheck", enable);
}

void MainWindow::on_autoBlockTimeSpin_editingFinished()
{
    us->setValue("block/autoTime", ui->autoBlockTimeSpin->value());
}

/**
 * 所有事件的槽都在这里触发
 */
void MainWindow::triggerCmdEvent(QString cmd, LiveDanmaku danmaku, bool debug)
{
    if (debug)
        qInfo() << "触发事件：" << cmd;
    emit signalCmdEvent(cmd, danmaku);

    sendDanmakuToSockets(cmd, danmaku);
}

void MainWindow::on_voiceLocalRadio_toggled(bool checked)
{
    if (checked)
    {
        voiceService->voicePlatform = VoiceLocal;
        us->setValue("voice/platform", voiceService->voicePlatform);
        ui->voiceNameEdit->setText(us->value("voice/localName").toString());

        ui->voiceConfigSettingsCard->show();
        ui->voiceXfySettingsCard->hide();
        ui->voiceMSSettingsCard->hide();
        ui->voiceCustomSettingsCard->hide();
        ui->scrollArea->updateChildWidgets();
        ui->scrollArea->adjustWidgetsPos();
    }
}

void MainWindow::on_voiceXfyRadio_toggled(bool checked)
{
    if (checked)
    {
        voiceService->voicePlatform = VoiceXfy;
        us->setValue("voice/platform", voiceService->voicePlatform);
        ui->voiceNameEdit->setText(us->value("xfytts/name", "xiaoyan").toString());

        ui->voiceConfigSettingsCard->show();
        ui->voiceXfySettingsCard->show();
        ui->voiceMSSettingsCard->hide();
        ui->voiceCustomSettingsCard->hide();
        ui->scrollArea->updateChildWidgets();
        ui->scrollArea->adjustWidgetsPos();
    }
}

void MainWindow::on_voiceMSRadio_toggled(bool checked)
{
    if (checked)
    {
        voiceService->voicePlatform = VoiceMS;
        us->setValue("voice/platform", voiceService->voicePlatform);
        ui->voiceNameEdit->setText(us->value("xfytts/name", "xiaoyan").toString());

        ui->voiceConfigSettingsCard->hide();
        ui->voiceMSSettingsCard->show();
        ui->voiceXfySettingsCard->hide();
        ui->voiceCustomSettingsCard->hide();
        ui->scrollArea->updateChildWidgets();
        ui->scrollArea->adjustWidgetsPos();
    }
}

void MainWindow::on_voiceCustomRadio_toggled(bool checked)
{
    ui->voiceConfigSettingsCard->setVisible(!checked);
    if (checked)
    {
        voiceService->voicePlatform = VoiceCustom;
        us->setValue("voice/platform", voiceService->voicePlatform);
        ui->voiceNameEdit->setText(us->value("voice/customName").toString());

        ui->voiceConfigSettingsCard->show();
        ui->voiceXfySettingsCard->hide();
        ui->voiceMSSettingsCard->hide();
        ui->voiceCustomSettingsCard->show();
        ui->scrollArea->updateChildWidgets();
        ui->scrollArea->adjustWidgetsPos();
    }
}

void MainWindow::on_voiceNameEdit_editingFinished()
{
    voiceService->voiceName = ui->voiceNameEdit->text();
    qInfo() << "设置发音人：" << voiceService->voiceName;
    switch (voiceService->voicePlatform) {
    case VoiceLocal:
        us->setValue("voice/localName", voiceService->voiceName);
        break;
    case VoiceXfy:
        us->setValue("xfytts/name", voiceService->voiceName);
        if (voiceService->xfyTTS)
        {
            voiceService->xfyTTS->setName(voiceService->voiceName);
        }
        break;
    case VoiceCustom:
        us->setValue("voice/customName", voiceService->voiceName);
        break;
    case VoiceMS:
    {
        static bool notified = false;
        if (!notified)
        {
            notified = true;
            showNotify("微软语音", "暂不支持GUI调节");
        }
        break;
    }
    }
}

void MainWindow::on_voiceNameSelectButton_clicked()
{
    if (voiceService->voicePlatform == VoiceXfy)
    {
        QStringList names{"讯飞小燕<xiaoyan>",
                         "讯飞许久<aisjiuxu>",
                         "讯飞小萍<aisxping>",
                         "讯飞小婧<aisjinger>",
                         "讯飞许小宝<aisbabyxu>"};

        newFacileMenu;
        for (int i = 0; i < names.size(); i++)
        {
            menu->addAction(names.at(i), [=]{
                QString text = names.at(i);
                QRegularExpression re("<(.+)>");
                QRegularExpressionMatch match;
                if (text.indexOf(re, 0, &match) > -1)
                    text = match.capturedTexts().at(1);
                this->ui->voiceNameEdit->setText(text);
                on_voiceNameEdit_editingFinished();
            });
        }
        menu->exec();
    }
}

void MainWindow::on_voicePitchSlider_valueChanged(int value)
{
    us->setValue("voice/pitch", voiceService->voicePitch = value);
    ui->voicePitchLabel->setText("音调" + snum(value));

    switch (voiceService->voicePlatform) {
    case VoiceLocal:
#if defined(ENABLE_TEXTTOSPEECH)
        if (voiceService->tts)
        {
            voiceService->tts->setPitch((voiceService->voicePitch - 50) / 50.0);
        }
#endif
        break;
    case VoiceXfy:
        if (voiceService->xfyTTS)
        {
            voiceService->xfyTTS->setPitch(voiceService->voicePitch);
        }
        break;
    case VoiceCustom:
        break;
    case VoiceMS:
    {
//        static bool notified = false;
//        if (!notified)
//        {
//            notified = true;
//            showNotify("微软语音", "暂不支持GUI调节");
//        }
        break;
    }
    }
}

void MainWindow::on_voiceSpeedSlider_valueChanged(int value)
{
    us->setValue("voice/speed", voiceService->voiceSpeed = value);
    ui->voiceSpeedLabel->setText("音速" + snum(value));

    switch (voiceService->voicePlatform) {
    case VoiceLocal:
#if defined(ENABLE_TEXTTOSPEECH)
        if (voiceService->tts)
        {
            voiceService->tts->setRate((voiceService->voiceSpeed - 50) / 50.0);
        }
#endif
        break;
    case VoiceXfy:
        if (voiceService->xfyTTS)
        {
            voiceService->xfyTTS->setSpeed(voiceService->voiceSpeed);
        }
        break;
    case VoiceCustom:
        break;
    case VoiceMS:
    {
//        static bool notified = false;
//        if (!notified)
//        {
//            notified = true;
//            showNotify("微软语音", "暂不支持GUI调节");
//        }
        break;
    }
    }
}

void MainWindow::on_voiceVolumeSlider_valueChanged(int value)
{
    us->setValue("voice/volume", voiceService->voiceVolume = value);
    ui->voiceVolumeLabel->setText("音量" + snum(value));

    switch (voiceService->voicePlatform) {
    case VoiceLocal:
#if defined(ENABLE_TEXTTOSPEECH)
        if (voiceService->tts)
        {
            voiceService->tts->setVolume((voiceService->voiceVolume) / 100.0);
        }
#endif
        break;
    case VoiceXfy:
        if (voiceService->xfyTTS)
        {
            voiceService->xfyTTS->setVolume(voiceService->voiceVolume);
        }
        break;
    case VoiceCustom:
        break;
    case VoiceMS:
    {
//        static bool notified = false;
//        if (!notified)
//        {
//            notified = true;
//            showNotify("微软语音", "暂不支持GUI调节");
//        }
        break;
    }
    }
}

void MainWindow::on_voicePreviewButton_clicked()
{
    speakText("这是一个语音合成示例");
}

void MainWindow::on_voiceLocalRadio_clicked()
{
#if defined(ENABLE_TEXTTOSPEECH)
    QTimer::singleShot(100, [=]{
        if (!voiceService->tts)
        {
            initTTS();
        }
        else
        {
            voiceService->tts->setRate( (voiceService->voiceSpeed = us->value("voice/speed", 50).toInt() - 50) / 50.0 );
            voiceService->tts->setPitch( (voiceService->voicePitch = us->value("voice/pitch", 50).toInt() - 50) / 50.0 );
            voiceService->tts->setVolume( (voiceService->voiceVolume = us->value("voice/volume", 50).toInt()) / 100.0 );
        }
    });
#endif
}

void MainWindow::on_voiceXfyRadio_clicked()
{
    QTimer::singleShot(100, [=]{
        if (!voiceService->xfyTTS)
        {
            initTTS();
        }
        else
        {
            voiceService->xfyTTS->setName( voiceService->voiceName = us->value("xfytts/name", "xiaoyan").toString() );
            voiceService->xfyTTS->setPitch( voiceService->voicePitch = us->value("voice/pitch", 50).toInt() );
            voiceService->xfyTTS->setSpeed( voiceService->voiceSpeed = us->value("voice/speed", 50).toInt() );
            voiceService->xfyTTS->setVolume( voiceService->voiceSpeed = us->value("voice/speed", 50).toInt() );
        }
    });
}

void MainWindow::on_voiceMSRadio_clicked()
{
    QTimer::singleShot(100, [=]{
        if (!voiceService->msTTS)
        {
            initTTS();
        }
        else
        {
            // TODO: 设置音调等内容
            voiceService->msTTS->clearQueue();
            voiceService->msTTS->refreshToken();
        }
    });
}

void MainWindow::on_voiceCustomRadio_clicked()
{
    QTimer::singleShot(100, [=]{
        initTTS();
    });
}

void MainWindow::on_label_10_linkActivated(const QString &link)
{
    QDesktopServices::openUrl(link);
}

void MainWindow::on_xfyAppIdEdit_textEdited(const QString &text)
{
    us->setValue("xfytts/appid", text);
    if (voiceService->xfyTTS)
    {
        voiceService->xfyTTS->setAppId(text);
    }
}

void MainWindow::on_xfyApiSecretEdit_textEdited(const QString &text)
{
    us->setValue("xfytts/apisecret", text);
    if (voiceService->xfyTTS)
    {
        voiceService->xfyTTS->setApiSecret(text);
    }
}

void MainWindow::on_xfyApiKeyEdit_textEdited(const QString &text)
{
    us->setValue("xfytts/apikey", text);
    if (voiceService->xfyTTS)
    {
        voiceService->xfyTTS->setApiKey(text);
    }
}

void MainWindow::on_voiceCustomUrlEdit_editingFinished()
{
    us->setValue("voice/customUrl", ui->voiceCustomUrlEdit->text());
}


void MainWindow::on_eternalBlockListButton_clicked()
{
    EternalBlockDialog* dialog = new EternalBlockDialog(&us->eternalBlockUsers, this);
    connect(dialog, SIGNAL(signalCancelEternalBlock(qint64, qint64)), this, SLOT(cancelEternalBlockUser(qint64, qint64)));
    connect(dialog, SIGNAL(signalCancelBlock(qint64, qint64)), this, SLOT(cancelEternalBlockUserAndUnblock(qint64, qint64)));
    dialog->exec();
}

void MainWindow::on_AIReplyMsgCheck_clicked()
{
    Qt::CheckState state = ui->AIReplyMsgCheck->checkState();
    us->setValue("danmaku/aiReplyMsg", state);
    if (state != Qt::PartiallyChecked)
        ui->AIReplyMsgCheck->setText("回复弹幕");
    else
        ui->AIReplyMsgCheck->setText("回复弹幕(仅单条)");
}

void MainWindow::slotAIReplyed(QString reply, qint64 uid)
{
    if (ui->AIReplyMsgCheck->checkState() != Qt::Unchecked)
    {
        // 机器人自己的不回复（不然自己和自己打起来了）
        if (snum(uid) == ac->cookieUid)
            return ;

        // AI回复长度上限，以及过滤
        if (ui->AIReplyMsgCheck->checkState() == Qt::PartiallyChecked
                && reply.length() > ac->danmuLongest)
            return ;

        // 自动断句
        QStringList sl;
        int len = reply.length();
        const int maxOne = ac->danmuLongest;
        int count = (len + maxOne - 1) / maxOne;
        for (int i = 0; i < count; i++)
        {
            sl << reply.mid(i * maxOne, maxOne);
        }
        cr->sendAutoMsg(sl.join("\\n"), LiveDanmaku());
    }
}

void MainWindow::on_danmuLongestSpin_editingFinished()
{
    ac->danmuLongest = ui->danmuLongestSpin->value();
    us->setValue("danmaku/danmuLongest", ac->danmuLongest);
}


void MainWindow::on_startupAnimationCheck_clicked()
{
    us->setValue("mainwindow/splash", ui->startupAnimationCheck->isChecked());
}

void MainWindow::on_serverCheck_clicked()
{
    bool enabled = ui->serverCheck->isChecked();
    us->setValue("server/enabled", enabled);
    if (enabled)
        openServer();
    else
        closeServer();
}

void MainWindow::on_serverPortSpin_editingFinished()
{
    us->setValue("server/port", ui->serverPortSpin->value());
#if defined(ENABLE_HTTP_SERVER)
    if (webServer->server)
    {
        closeServer();
        QTimer::singleShot(1000, [=]{
            openServer();
        });
    }
#endif
}

void MainWindow::on_autoPauseOuterMusicCheck_clicked()
{
    us->setValue("danmaku/autoPauseOuterMusic", ui->autoPauseOuterMusicCheck->isChecked());
}

void MainWindow::on_outerMusicKeyEdit_textEdited(const QString &arg1)
{
    us->setValue("danmaku/outerMusicPauseKey", arg1);
}

void MainWindow::on_acquireHeartCheck_clicked()
{
    us->setValue("danmaku/acquireHeart", ui->acquireHeartCheck->isChecked());

    if (ui->acquireHeartCheck->isChecked())
    {
        if (liveService->isLiving())
            liveService->startHeartConnection();
    }
    else
    {
        liveService->stopHeartConnection();
    }
}

void MainWindow::on_sendExpireGiftCheck_clicked()
{
    bool enable = ui->sendExpireGiftCheck->isChecked();
    us->setValue("danmaku/sendExpireGift", enable);

    if (enable)
    {
        sendExpireGift();
    }
}

void MainWindow::on_actionPicture_Browser_triggered()
{
    if (!pictureBrowser)
    {
        pictureBrowser = new PictureBrowser(us, nullptr);
    }
    pictureBrowser->show();
    pictureBrowser->readDirectory(rt->dataPath + "captures");
}

void MainWindow::on_orderSongsToFileCheck_clicked()
{
    us->setValue("danmaku/orderSongsToFile", ui->orderSongsToFileCheck->isChecked());

    if (musicWindow && ui->orderSongsToFileCheck->isChecked())
    {
        saveOrderSongs(musicWindow->getOrderSongs());
    }
}

void MainWindow::on_orderSongsToFileFormatEdit_textEdited(const QString &arg1)
{
    us->setValue("danmaku/orderSongsToFileFormat", arg1);

    if (musicWindow && ui->orderSongsToFileCheck->isChecked())
    {
        saveOrderSongs(musicWindow->getOrderSongs());
    }
}

void MainWindow::on_orderSongsToFileMaxSpin_editingFinished()
{
    us->setValue("danmaku/orderSongsToFileMax", ui->orderSongsToFileMaxSpin->value());

    if (musicWindow && ui->orderSongsToFileCheck->isChecked())
    {
        saveOrderSongs(musicWindow->getOrderSongs());
    }
}

void MainWindow::on_playingSongToFileCheck_clicked()
{
    us->setValue("danmaku/playingSongToFile", ui->playingSongToFileCheck->isChecked());

    if (musicWindow && ui->playingSongToFileCheck->isChecked())
    {
        savePlayingSong();
    }
}

void MainWindow::on_playingSongToFileFormatEdit_textEdited(const QString &arg1)
{
    us->setValue("danmaku/playingSongToFileFormat", arg1);

    if (musicWindow && ui->playingSongToFileCheck->isChecked())
    {
        savePlayingSong();
    }
}

void MainWindow::on_actionCatch_You_Online_triggered()
{
    CatchYouWidget* cyw = new CatchYouWidget(us, rt->dataPath, nullptr);
    cyw->show();
    // cyw->catchUser(upUid);
    cyw->setDefaultUser(ac->upUid);
}

void MainWindow::on_pkBlankButton_clicked()
{
    bool ok = false;
    QString text = QInputDialog::getText(this, "自动偷塔黑名单", "设置不启用自动偷塔的房间号列表\n多个房间用任意非数字符号分隔",
                                         QLineEdit::Normal, liveService->toutaBlankList.join(";"), &ok);
    if (!ok)
        return ;
    liveService->toutaBlankList = text.split(QRegExp("[^\\d]+"), QString::SkipEmptyParts);
    us->setValue("pk/blankList", liveService->toutaBlankList.join(";"));
}

void MainWindow::on_actionUpdate_New_Version_triggered()
{
    if (!rt->appDownloadUrl.isEmpty())
    {
        QDesktopServices::openUrl(QUrl(rt->appDownloadUrl));
    }
    else
    {
        syncMagicalRooms();
    }
}

void MainWindow::on_startOnRebootCheck_clicked()
{
    bool enable = ui->startOnRebootCheck->isChecked();
    us->setValue("runtime/startOnReboot", enable);

    QString appName = QApplication::applicationName();
    QString appPath = QDir::toNativeSeparators(QApplication::applicationFilePath());
    QSettings *reg=new QSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString val = reg->value(appName).toString();// 如果此键不存在，则返回的是空字符串
    if (enable)
        reg->setValue(appName, appPath);// 如果移除的话
    else
        reg->remove(appName);
    reg->deleteLater();
}

void MainWindow::on_domainEdit_editingFinished()
{
    webServer->serverDomain = ui->domainEdit->text().trimmed();
    if (webServer->serverDomain.isEmpty())
        webServer->serverDomain = "localhost";
    us->setValue("server/domain", webServer->serverDomain);
}

void MainWindow::prepareQuit()
{
    releaseLiveData();

    QTimer::singleShot(0, [=]{
        if (recordConvertProcess)
        {
            recordConvertProcess->waitForFinished(30000);
            deleteFile(recordLastPath); // 本来是延迟删的，但是怕来不及了
        }

        qApp->quit();
    });
}

void MainWindow::on_giftComboSendCheck_clicked()
{
    us->setValue("danmaku/giftComboSend", ui->giftComboSendCheck->isChecked());
}

void MainWindow::on_giftComboDelaySpin_editingFinished()
{
    us->setValue("danmaku/giftComboDelay", ui->giftComboDelaySpin->value());
}

void MainWindow::on_retryFailedDanmuCheck_clicked()
{
    us->setValue("danmaku/retryFailedDanmu", us->retryFailedDanmaku = ui->retryFailedDanmuCheck->isChecked());
}

void MainWindow::on_songLyricsToFileCheck_clicked()
{
    us->setValue("danmaku/songLyricsToFile", ui->songLyricsToFileCheck->isChecked());

    if (musicWindow && ui->songLyricsToFileCheck->isChecked())
    {
        saveSongLyrics();
    }
    if (musicWindow && webServer->sendLyricListToSockets)
    {
        sendLyricList();
    }
}

void MainWindow::on_songLyricsToFileMaxSpin_editingFinished()
{
    us->setValue("danmaku/songLyricsToFileMax", ui->songLyricsToFileMaxSpin->value());

    if (musicWindow && ui->songLyricsToFileCheck->isChecked())
    {
        saveSongLyrics();
    }
    if (musicWindow && webServer->sendLyricListToSockets)
    {
        sendLyricList();
    }
}

void MainWindow::on_allowWebControlCheck_clicked()
{
    us->setValue("server/allowWebControl", ui->allowWebControlCheck->isChecked());
}

void MainWindow::on_saveEveryGuardCheck_clicked()
{
    us->setValue("danmaku/saveEveryGuard", ui->saveEveryGuardCheck->isChecked());
}

void MainWindow::on_saveMonthGuardCheck_clicked()
{
    us->setValue("danmaku/saveMonthGuard", ui->saveMonthGuardCheck->isChecked());
}

void MainWindow::on_saveEveryGiftCheck_clicked()
{
    us->setValue("danmaku/saveEveryGift", ui->saveEveryGiftCheck->isChecked());
}

void MainWindow::on_exportDailyButton_clicked()
{
    if (ac->roomId.isEmpty())
        return ;

    QString oldPath = us->value("danmaku/exportPath", "").toString();
    QString path = QFileDialog::getSaveFileName(this, "选择导出位置", oldPath, "Tables (*.csv *.txt)");
    if (path.isEmpty())
        return ;
    us->setValue("danmaku/exportPath", path);
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file);
    stream.setGenerateByteOrderMark(true);
    if (!liveService->recordFileCodec.isEmpty())
        stream.setCodec(liveService->recordFileCodec.toUtf8());

    // 拼接数据
    QString dirPath = rt->dataPath + "live_daily";
    QDir dir(dirPath);
    auto files = dir.entryList(QStringList{ac->roomId + "_*.ini"}, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    stream << QString("日期,进入人次,进入人数,弹幕数量,新人弹幕,新增关注,关注总数,总金瓜子,总银瓜子,上船人数,船员总数,平均人气,最高人气\n").toUtf8();
    for (int i = 0; i < files.size(); i++)
    {
        QStringList sl;
        QSettings st(dirPath + "/" + files.at(i), QSettings::Format::IniFormat);
        QString day = files.at(i);
        day.replace(ac->roomId + "_", "").replace(".ini", "");
        stream << day << ","
               << st.value("come", 0).toInt() << ","
               << st.value("people_num", 0).toInt() << ","
               << st.value("danmaku", 0).toInt() << ","
               << st.value("newbie_msg", 0).toInt() << ","
               << st.value("new_fans", 0).toInt() << ","
               << st.value("total_fans", 0).toInt() << ","
               << st.value("gift_gold", 0).toInt() << ","
               << st.value("gift_silver", 0).toInt() << ","
               << st.value("guard", 0).toInt() << ","
               << st.value("guard_count", 0).toInt()  << ","
               << st.value("average_popularity", 0).toInt() << ","
               << st.value("max_popularity", 0).toInt()
               << "\n";
    }

    file.close();
}

void MainWindow::on_closeTransMouseButton_clicked()
{
    if (danmakuWindow)
        danmakuWindow->closeTransMouse();
}

void MainWindow::on_pkAutoMaxGoldCheck_clicked()
{
    us->setValue("pk/autoMaxGold", ui->pkAutoMaxGoldCheck->isChecked());
}

void MainWindow::on_saveRecvCmdsCheck_clicked()
{
    saveRecvCmds = ui->saveRecvCmdsCheck->isChecked();
    us->setValue("debug/saveRecvCmds", saveRecvCmds);

    if (saveRecvCmds)
    {
        QDir dir;
        dir.mkdir(rt->dataPath+"websocket_cmds");
        QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd hh.mm.ss");
        saveCmdsFile = new QFile(rt->dataPath+"websocket_cmds/" + ac->roomId + "_" + date + ".txt");
        saveCmdsFile->open(QIODevice::WriteOnly | QIODevice::Append);
        qInfo() << "开始保存cmds：" << rt->dataPath+"websocket_cmds/" + ac->roomId + "_" + date + ".txt";
    }
    else if (saveCmdsFile)
    {
        qInfo() << "结束保存cmds";
        saveCmdsFile->close();
        saveCmdsFile->deleteLater();
        saveCmdsFile = nullptr;
    }
}

void MainWindow::on_allowRemoteControlCheck_clicked()
{
    us->setValue("danmaku/remoteControl", us->remoteControl = ui->allowRemoteControlCheck->isChecked());
}

void MainWindow::on_actionJoin_Battle_triggered()
{
    liveService->joinBattle(1);
}

void MainWindow::on_actionQRCode_Login_triggered()
{
    QRCodeLoginDialog* dialog = new QRCodeLoginDialog(this);
    connect(dialog, &QRCodeLoginDialog::logined, this, [=](QString s){
        autoSetCookie(s);
    });
    dialog->exec();
}

void MainWindow::on_allowAdminControlCheck_clicked()
{
    us->setValue("danmaku/adminControl", ui->allowAdminControlCheck->isChecked());
}

void MainWindow::on_actionSponsor_triggered()
{
    EscapeDialog* dialog = new EscapeDialog("友情赞助", "您的支持是开发者为爱发电的最大动力！", "不想付钱", "感谢支持", this);
    dialog->exec();
}

void MainWindow::on_actionPaste_Code_triggered()
{
    QString clipText = QApplication::clipboard()->text();
    if (clipText.isEmpty())
    {
        QMessageBox::information(this, "粘贴代码片段", "在定时任务、自动回复、事件动作等列表的右键菜单中复制的结果，可用于备份或发送至其余地方");
        return ;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(clipText.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        QMessageBox::information(this, "粘贴代码片段", "只支持JSON格式的代码配置\n" + error.errorString());
        return ;
    }

    addCodeSnippets(doc);
}

void MainWindow::on_actionGenerate_Default_Code_triggered()
{
    QString oldPath = us->value("danmaku/codePath", "").toString();
    QString path = QFileDialog::getSaveFileName(this, "选择导出位置", oldPath, "Json (*.json *.txt)");
    if (path.isEmpty())
        return ;
    us->setValue("danmaku/codePath", path);

    generateDefaultCode(path);
}

void MainWindow::on_actionRead_Default_Code_triggered()
{
    QString oldPath = us->value("danmaku/codePath", "").toString();
    QString path = QFileDialog::getOpenFileName(this, "选择读取位置", oldPath, "Json (*.json *.txt)");
    if (path.isEmpty())
        return ;
    us->setValue("danmaku/codePath", path);

    readDefaultCode(path);
}

void MainWindow::on_giftComboTopCheck_clicked()
{
    us->setValue("danmaku/giftComboTop", ui->giftComboTopCheck->isChecked());
}

void MainWindow::on_giftComboMergeCheck_clicked()
{
    us->setValue("danmaku/giftComboMerge", ui->giftComboMergeCheck->isChecked());
}

void MainWindow::on_listenMedalUpgradeCheck_clicked()
{
    us->setValue("danmaku/listenMedalUpgrade", ui->listenMedalUpgradeCheck->isChecked());
}

void MainWindow::on_pushRecvCmdsButton_clicked()
{
    if (!pushCmdsFile) // 开启模拟输入
    {
        // 输入文件
        QString oldPath = us->value("debug/cmdsPath", "").toString();
        QString path = QFileDialog::getOpenFileName(this, "选择模拟输入的CMDS文件位置", oldPath, "Text (*.txt)");
        if (path.isEmpty())
            return ;
        us->setValue("debug/cmdsPath", path);

        // 创建文件对象
        pushCmdsFile = new QFile(path);
        if (!pushCmdsFile->open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::critical(this, "模拟CMDS", "读取文件失败");
            return ;
        }

        // 本地模式提示
        if (!ui->actionLocal_Mode->isChecked())
        {
            if (QMessageBox::question(this, "模拟CMDS", "您的[本地模式]未开启，可能会回复奇奇怪怪的内容，是否开启[本地模式]？",
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
                ui->actionLocal_Mode->setChecked(us->localMode = true);
        }
        ui->saveRecvCmdsCheck->setChecked(saveRecvCmds = false);

        // 定时器
        if (!pushCmdsTimer)
        {
            pushCmdsTimer = new QTimer(this);
            pushCmdsTimer->setInterval(ui->timerPushCmdSpin->value() * 100);
            connect(pushCmdsTimer, SIGNAL(timeout()), this, SLOT(on_pushNextCmdButton_clicked()));
        }
        pushCmdsTimer->setInterval(ui->timerPushCmdSpin->value() * 100);
        if (ui->timerPushCmdCheck->isChecked())
            pushCmdsTimer->start();

        ui->pushRecvCmdsButton->setText("停止");
        ui->pushNextCmdButton->show();
        ui->timerPushCmdCheck->show();
        ui->timerPushCmdSpin->show();
    }
    else // 关闭模拟输入
    {
        if (ui->actionLocal_Mode->isChecked() && us->value("debug/localDebug", false).toBool())
            ui->actionLocal_Mode->setChecked(us->localMode = false);

        if (us->value("debug/saveRecvCmds", false).toBool())
            ui->saveRecvCmdsCheck->setChecked(saveRecvCmds = true);

        if (pushCmdsTimer)
            pushCmdsTimer->stop();

        ui->pushRecvCmdsButton->setText("输入CMDS");
        ui->pushNextCmdButton->hide();
        ui->timerPushCmdCheck->hide();
        ui->timerPushCmdSpin->hide();

        pushCmdsFile->close();
        pushCmdsFile->deleteLater();
        pushCmdsFile = nullptr;
    }
}

void MainWindow::on_pushNextCmdButton_clicked()
{
    if (!pushCmdsFile)
        return ;

    QByteArray line = pushCmdsFile->readLine();
    if (line.isNull())
    {
        localNotify("[模拟输入CMDS结束，已退出]");
        on_pushRecvCmdsButton_clicked();
        return ;
    }

    // 处理下一行
    line.replace("__bmd__n__", "\n").replace("__bmd__r__", "\r");
    slotBinaryMessageReceived(line);
}

void MainWindow::on_timerPushCmdCheck_clicked()
{
    bool enable = ui->timerPushCmdCheck->isChecked();
    us->setValue("debug/pushCmdsTimer", enable);

    if (pushCmdsFile && pushCmdsTimer)
    {
        if (enable)
            pushCmdsTimer->start();
        else
            pushCmdsTimer->stop();
    }
}

void MainWindow::on_timerPushCmdSpin_editingFinished()
{
    int val = ui->timerPushCmdSpin->value();
    us->setValue("debug/pushCmdsInterval", val);

    if (pushCmdsTimer)
        pushCmdsTimer->setInterval(val * 100);
}

void MainWindow::on_pkChuanmenCheck_stateChanged(int arg1)
{
    ui->pkMsgSyncCheck->setEnabled(arg1);
}

void MainWindow::on_actionLast_Candidate_triggered()
{
    // QMessageBox::information(this, "最后一次调试的候选弹幕", "-------- 填充变量 --------\n\n" + lastConditionDanmu + "\n\n-------- 随机发送 --------\n\n" + lastCandidateDanmaku);
    QDialog* dialog = new QDialog(this);
    QTabWidget* tabWidget = new QTabWidget(dialog);

    int count = cr->lastConditionDanmu.size();
    for (int i = count - 1; i >= 0; i--)
    {
        QPlainTextEdit* edit = new QPlainTextEdit(tabWidget);
        edit->setPlainText("-------- 填充变量 --------\n\n"
                           + cr->lastConditionDanmu.at(i)
                           + "\n\n-------- 随机发送 --------\n\n"
                           + cr->lastCandidateDanmaku.at(i));
        tabWidget->addTab(edit, i == count - 1 ? "最新" : i == 0 ? "最旧" : snum(i+1));
    }

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->addWidget(tabWidget);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setWindowTitle("变量历史");
    dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    dialog->setGeometry(this->geometry());
    dialog->exec();
}

void MainWindow::on_actionLocal_Mode_triggered()
{
    us->setValue("debug/localDebug", us->localMode = ui->actionLocal_Mode->isChecked());
    qInfo() << "本地模式：" << us->localMode;
}

void MainWindow::on_actionDebug_Mode_triggered()
{
    us->setValue("debug/debugPrint", us->debugPrint = ui->actionDebug_Mode->isChecked());
    qInfo() << "调试模式：" << us->debugPrint;
    if (us->debugPrint)
        ensureFileExist("update_tool.log");
    else
        deleteFile("update_tool.log");
}

void MainWindow::on_actionGuard_Online_triggered()
{
    GuardOnlineDialog* god = new GuardOnlineDialog(us, ac->roomId, ac->upUid, this);
    god->show();
}

void MainWindow::on_actionOfficial_Website_triggered()
{
    QDesktopServices::openUrl(QUrl("http://danmaku.lyixi.com"));
}

void MainWindow::on_actionAnchor_Case_triggered()
{
    QDesktopServices::openUrl(QUrl("http://anchor.lyixi.com"));
}

void MainWindow::on_robotNameButton_clicked()
{
    newFacileMenu;
    menu->addAction(ui->actionQRCode_Login);
    menu->addAction(ui->actionSet_Cookie);
    menu->addAction(ui->actionSet_Danmaku_Data_Format);
    menu->split()->addAction(ui->actionLogout)->disable(ac->cookieUid.isEmpty());
    menu->exec();
}

void MainWindow::on_thankWelcomeTabButton_clicked()
{
    ui->thankStackedWidget->setCurrentIndex(0);
    us->setValue("mainwindow/thankStackIndex", 0);

    foreach (auto btn, thankTabButtons)
    {
        btn->setNormalColor(Qt::transparent);
        btn->setTextColor(Qt::black);
    }
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setNormalColor(themeSbg);
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setTextColor(themeSfg);
}

void MainWindow::on_thankGiftTabButton_clicked()
{
    ui->thankStackedWidget->setCurrentIndex(1);
    us->setValue("mainwindow/thankStackIndex", 1);

    foreach (auto btn, thankTabButtons)
    {
        btn->setNormalColor(Qt::transparent);
        btn->setTextColor(Qt::black);
    }
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setNormalColor(themeSbg);
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setTextColor(themeSfg);
}

void MainWindow::on_thankAttentionTabButton_clicked()
{
    ui->thankStackedWidget->setCurrentIndex(2);
    us->setValue("mainwindow/thankStackIndex", 2);

    foreach (auto btn, thankTabButtons)
    {
        btn->setNormalColor(Qt::transparent);
        btn->setTextColor(Qt::black);
    }
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setNormalColor(themeSbg);
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setTextColor(themeSfg);
}

void MainWindow::on_blockTabButton_clicked()
{
    ui->thankStackedWidget->setCurrentIndex(3);
    us->setValue("mainwindow/thankStackIndex", 3);

    foreach (auto btn, thankTabButtons)
    {
        btn->setNormalColor(Qt::transparent);
        btn->setTextColor(Qt::black);
    }
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setNormalColor(themeSbg);
    thankTabButtons.at(ui->thankStackedWidget->currentIndex())->setTextColor(themeSfg);
}

void MainWindow::on_sendMsgMoreButton_clicked()
{
    newFacileMenu;

    menu->addAction(ui->actionSend_Long_Text);

    menu->exec();
}

void MainWindow::on_showLiveDanmakuWindowButton_clicked()
{
    on_actionShow_Live_Danmaku_triggered();
}

void MainWindow::on_liveStatusButton_clicked()
{
    if (!ac->liveStatus)
        return ;
    on_actionShow_Live_Video_triggered();
}

void MainWindow::on_tenCardLabel1_linkActivated(const QString &link)
{
    openLink(link);
}

void MainWindow::on_tenCardLabel2_linkActivated(const QString &link)
{
    openLink(link);
}

void MainWindow::on_tenCardLabel3_linkActivated(const QString &link)
{
    openLink(link);
}

void MainWindow::on_tenCardLabel4_linkActivated(const QString &link)
{
    openLink(link);
}

void MainWindow::on_musicBlackListButton_clicked()
{
    QString blackList = orderSongBlackList.join(" ");
    bool ok;
    blackList = TextInputDialog::getText(this, "点歌黑名单", "带有关键词的点歌都将被阻止，并触发“ORDER_SONG_BLOCKED”事件"
                                                        "\n多个关键词用空格隔开，英文字母均使用小写", blackList, &ok);
    if (!ok)
        return ;
    orderSongBlackList = blackList.split(" ", QString::SkipEmptyParts);
    us->setValue("music/blackListKeys", blackList);
}

void MainWindow::on_enableFilterCheck_clicked()
{
    us->setValue("danmaku/enableFilter", cr->enableFilter = ui->enableFilterCheck->isChecked());
}

void MainWindow::on_actionReplace_Variant_triggered()
{
    QString text = saveReplaceVariant();
    bool ok;
    text = TextInputDialog::getText(this, "替换变量", "请输入发送前的替换内容，支持正则表达式：\n示例格式：发送前内容=发送后内容\n仅影响自动发送的弹幕", text, &ok);
    if (!ok)
        return ;
    us->setValue("danmaku/replaceVariant", text);

    restoreReplaceVariant(text);
}

void MainWindow::on_autoClearComeIntervalSpin_editingFinished()
{
    us->setValue("danmaku/clearDidntComeInterval", ui->autoClearComeIntervalSpin->value());
}

void MainWindow::on_roomDescriptionBrowser_anchorClicked(const QUrl &arg1)
{
    QDesktopServices::openUrl(arg1);
}

void MainWindow::on_adjustDanmakuLongestCheck_clicked()
{
    bool en = ui->adjustDanmakuLongestCheck->isChecked();
    us->setValue("danmaku/adjustDanmakuLongest", en);
    if (en)
    {
        liveService->adjustDanmakuLongest();
    }
}

void MainWindow::on_actionBuy_VIP_triggered()
{
    BuyVIPDialog* bvd = new BuyVIPDialog(rt->dataPath, ac->roomId, ac->upUid, ac->cookieUid, ac->roomTitle, ac->upName, ac->cookieUname, permissionDeadline, permissionType, this);
    connect(bvd, &BuyVIPDialog::refreshVIP, this, [=]{
        updatePermission();
    });
    bvd->show();
}

void MainWindow::on_droplight_clicked()
{
    if (rt->asFreeOnly)
    {
        showNotify("小提示", "右键可以修改文字");
        return ;
    }
    on_actionBuy_VIP_triggered();
}

void MainWindow::on_vipExtensionButton_clicked()
{
    if (rt->asFreeOnly)
    {
        QMessageBox::warning(this, "作为插件的Lite版说明", "Lite版未安装部分功能，请前往官网下载完整版");
        openLink("http://lyixi.com/?type=newsinfo&S_id=126");
        return ;
    }
    on_actionBuy_VIP_triggered();
}

void MainWindow::on_enableTrayCheck_clicked()
{
    bool en = ui->enableTrayCheck->isChecked();
    us->setValue("mainwindow/enableTray", en);
    if (en)
    {
        tray->show();
    }
    else
    {
        tray->hide();
    }
}

void MainWindow::on_toutaGiftCheck_clicked()
{
    us->setValue("danmaku/toutaGift", ui->toutaGiftCheck->isChecked());

    // 设置默认值
    if (!liveService->toutaGiftCounts.size())
    {
        QString s = "1 2 3 10 100 520 1314";
        ui->toutaGiftCountsEdit->setText(s);
        on_toutaGiftCountsEdit_textEdited(s);
    }

    if (!liveService->toutaGifts.size())
    {
        QString s = "20004 吃瓜 100\n\
                20014 比心 500\n\
                20008 冰阔落 800\n\
                30758 这个好诶 1000\n\
                30063 给大佬递茶 2000\n\
                30046 打榜 2000\n\
                30004 喵娘 5200\n\
                30064 礼花 28000\n\
                30873 花式夸夸 39000\n\
                30072 疯狂打call 52000\n\
                30087 天空之翼 100000\n\
                30924 机车娘 200000";
        us->setValue("danmaku/toutaGifts", s);
        restoreToutaGifts(s);
    }
}

void MainWindow::on_toutaGiftCountsEdit_textEdited(const QString &arg1)
{
    // 允许的数量，如：1 2 3 4 5 10 11 100 101
    us->setValue("danmaku/toutaGiftCounts", arg1);
    QStringList sl = arg1.split(" ", QString::SkipEmptyParts);
    liveService->toutaGiftCounts.clear();
    foreach (QString s, sl)
    {
        liveService->toutaGiftCounts.append(s.toInt());
    }
}

void MainWindow::on_toutaGiftListButton_clicked()
{
    // 礼物ID 礼物名字 金瓜子
    QStringList sl;
    foreach (auto gift, liveService->toutaGifts)
    {
        sl.append(QString("%1 %2 %3").arg(gift.getGiftId()).arg(gift.getGiftName()).arg(gift.getTotalCoin()));
    }

    bool ok;
    QString totalText = TextInputDialog::getText(this, "允许赠送的礼物列表", "一行一个礼物，格式：\n礼物ID 礼物名字 金瓜子数量", sl.join("\n"), &ok);
    if (!ok)
        return ;
    us->setValue("danmaku/toutaGifts", totalText);
    restoreToutaGifts(totalText);
}

void MainWindow::on_timerConnectIntervalSpin_editingFinished()
{
    us->setValue("live/timerConnectInterval", ui->timerConnectIntervalSpin->value());
    liveService->connectServerTimer->setInterval(ui->timerConnectIntervalSpin->value() * 60000);
}

void MainWindow::on_heartTimeSpin_editingFinished()
{
    us->setValue("danmaku/acquireHeartTime", us->getHeartTimeCount = ui->heartTimeSpin->value());
}

void MainWindow::on_syncShieldKeywordCheck_clicked()
{
    bool enabled = ui->syncShieldKeywordCheck->isChecked();
    us->setValue("block/syncShieldKeyword", enabled);
    if (enabled)
        pullRoomShieldKeyword();
}

void MainWindow::on_roomCoverSpacingLabel_customContextMenuRequested(const QPoint &)
{
    newFacileMenu;

    // 主播专属操作
    if (ac->cookieUid == ac->upUid)
    {
        menu->addTitle("直播设置", 0);
        menu->addAction(QIcon(":/icons/title"), "修改直播标题", [=]{
            liveService->myLiveSetTitle();
        });
        menu->addAction(QIcon(":/icons/default_cover"), "更换直播封面", [=]{
            liveService->myLiveSetCover();
        });
        menu->addAction(QIcon(":/icons/news"), "修改主播公告", [=]{
            liveService->myLiveSetNews();
        });
        menu->addAction(QIcon(":/icons/person_description"), "修改个人简介", [=]{
            liveService->myLiveSetDescription();
        });
        menu->addAction(QIcon(":/icons/tags"), "修改个人标签", [=]{
            liveService->myLiveSetTags();
        });

        menu->addTitle("快速开播", -1);
        menu->addAction(QIcon(":/icons/area"), "选择分区", [=]{
            liveService->myLiveSelectArea(false);
        });
        menu->addAction(QIcon(":/icons/video2"), "一键开播", [=]{
            if (!liveService->isLiving())
            {
                // 一键开播
                liveService->myLiveStartLive();
            }
            else
            {
                // 一键下播
                liveService->myLiveStopLive();
            }
        })->text(liveService->isLiving(), "一键下播");

        menu->addAction(QIcon(":/icons/rtmp"), "复制rtmp", [=]{
            QApplication::clipboard()->setText(liveService->myLiveRtmp);
        })->disable(liveService->myLiveRtmp.isEmpty())->lingerText("已复制");

        menu->addAction(QIcon(":/icons/token"), "复制直播码", [=]{
            QApplication::clipboard()->setText(liveService->myLiveCode);
        })->disable(liveService->myLiveCode.isEmpty())->lingerText("已复制");

        menu->split();
    }

    // 普通操作
    menu->addAction(QIcon(":/icons/save"), "保存直播间封面", [=]{
        QString oldPath = us->value("danmaku/exportPath", "").toString();
        if (!oldPath.isEmpty())
        {
            QFileInfo info(oldPath);
            if (info.isFile()) // 去掉文件名（因为可能是其他文件）
                oldPath = info.absolutePath();
        }
        QString path = QFileDialog::getSaveFileName(this, "选择保存位置", oldPath, "Images (*.jpg *.png)");
        if (path.isEmpty())
            return ;
        us->setValue("danmaku/exportPath", path);

        liveService->roomCover.save(path);
    })->disable(liveService->roomCover.isNull());

    menu->exec();
}

void MainWindow::on_upHeaderLabel_customContextMenuRequested(const QPoint &)
{
    newFacileMenu;
    menu->addAction(QIcon(":/icons/save"), "保存主播头像", [=]{
        QString oldPath = us->value("danmaku/exportPath", "").toString();
        if (!oldPath.isEmpty())
        {
            QFileInfo info(oldPath);
            if (info.isFile()) // 去掉文件名（因为可能是其他文件）
                oldPath = info.absolutePath();
        }
        QString path = QFileDialog::getSaveFileName(this, "选择保存位置", oldPath, "Images (*.jpg *.png)");
        if (path.isEmpty())
            return ;
        us->setValue("danmaku/exportPath", path);

        liveService->upFace.save(path);
    })->disable(liveService->upFace.isNull());

    menu->exec();
}

void MainWindow::on_saveEveryGiftButton_clicked()
{
    QDir dir(rt->dataPath + "gift_histories");
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void MainWindow::on_saveEveryGuardButton_clicked()
{
    QDir dir(rt->dataPath + "guard_histories");
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void MainWindow::on_saveMonthGuardButton_clicked()
{
    QDir dir(rt->dataPath + "guard_month");
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void MainWindow::on_musicConfigButton_clicked()
{
    int page = ui->musicConfigStack->currentIndex();
    if (page == 0) // 设置页面
    {
        ui->musicConfigStack->setCurrentIndex(1);
    }
    else if (page == 1) // 列表页面
    {
        ui->musicConfigStack->setCurrentIndex(0);
    }
}

void MainWindow::on_musicConfigStack_currentChanged(int arg1)
{
    us->setValue("mainwindow/musicStackIndex", arg1);
}

void MainWindow::on_addMusicToLiveButton_clicked()
{
    ui->extensionPageButton->simulateStatePress();
    ui->tabWidget->setCurrentWidget(ui->tabRemote);
    shakeWidget(ui->existExtensionsLabel);
}

void MainWindow::on_roomNameLabel_customContextMenuRequested(const QPoint &)
{
    if (ac->cookieUid != ac->upUid)
        return ;

    newFacileMenu;

    menu->addAction(QIcon(":/icons/title"), "修改直播标题", [=]{
        liveService->myLiveSetTitle();
    });

    menu->exec();
}

void MainWindow::on_upNameLabel_customContextMenuRequested(const QPoint &)
{
    newFacileMenu;

    menu->addAction(QIcon(":/icons/copy"), "复制昵称", [=]{
        QApplication::clipboard()->setText(ac->upName);
    });

    if (ac->cookieUid == ac->upUid)
    {
        menu->addAction(QIcon(":/icons/modify"), "修改昵称", [=]{

        })->disable();
    }

    menu->exec();
}

void MainWindow::on_roomAreaLabel_customContextMenuRequested(const QPoint &)
{
    if (ac->cookieUid != ac->upUid)
        return ;

    newFacileMenu;

    menu->addAction(QIcon(":/icons/area"), "修改分区", [=]{
        liveService->myLiveSelectArea(true);
    });

    menu->exec();
}

void MainWindow::on_tagsButtonGroup_customContextMenuRequested(const QPoint &)
{
    if (ac->cookieUid != ac->upUid)
        return ;

    newFacileMenu;

    menu->addAction(QIcon(":/icons/tags"), "修改个人标签", [=]{
        liveService->myLiveSetTags();
    });

    menu->exec();
}

void MainWindow::on_roomDescriptionBrowser_customContextMenuRequested(const QPoint &)
{
    if (ac->cookieUid != ac->upUid)
        return ;

    newFacileMenu;

    menu->addAction(QIcon(":/icons/person_description"), "修改个人简介", [=]{
        liveService->myLiveSetDescription();
    });

    menu->exec();
}

void MainWindow::on_upLevelLabel_customContextMenuRequested(const QPoint &)
{
    if (ac->cookieUid != ac->upUid)
        return ;

    newFacileMenu;

    menu->addAction(QIcon(":/icons/person_description"), "修改个人签名", [=]{

    })->disable();

    menu->exec();
}

void MainWindow::on_refreshExtensionListButton_clicked()
{
    loadWebExtensionList();
    qInfo() << "网页扩展数量：" << ui->extensionListWidget->count();
}

void MainWindow::on_droplight_customContextMenuRequested(const QPoint &)
{
    newFacileMenu;

    menu->addAction(QIcon(":/icons/heart2"), "自定义文字", [=]{
        bool ok;
        QString s = QInputDialog::getText(this, "自定义文字", "高度自定义显示文字", QLineEdit::Normal, permissionText, &ok);
        if (!ok)
            return ;
        us->setValue("mainwindow/permissionText", permissionText = s);
        ui->droplight->setText(permissionText);
        ui->droplight->adjustMinimumSize();
    })->disable(!hasPermission() && !rt->asPlugin);

    menu->exec();
}

void MainWindow::on_autoUpdateCheck_clicked()
{
    us->setValue("runtime/autoUpdate", ui->autoUpdateCheck->isChecked());
}

void MainWindow::on_showChangelogCheck_clicked()
{
    us->setValue("runtime/showChangelog", ui->showChangelogCheck->isChecked());
}

void MainWindow::on_updateBetaCheck_clicked()
{
    us->setValue("runtime/updateBeta", ui->updateBetaCheck->isChecked());
}

void MainWindow::on_dontSpeakOnPlayingSongCheck_clicked()
{
    us->setValue("danmaku/dontSpeakOnPlayingSong", ui->dontSpeakOnPlayingSongCheck->isChecked());
}

void MainWindow::on_shieldKeywordListButton_clicked()
{
    QDesktopServices::openUrl(QUrl(SERVER_DOMAIN + "/web/keyword/list"));
}

void MainWindow::on_saveEveryGuardButton_customContextMenuRequested(const QPoint &pos)
{
    newFacileMenu;

    menu->addAction("导出每月表格", [=]{
        QString oldPath = us->value("danmaku/exportPath", "").toString();
        QString path = QFileDialog::getSaveFileName(this, "选择导出位置", oldPath, "Tables (*.csv)");
        if (path.isEmpty())
            return ;
        us->setValue("danmaku/exportPath", path);

        exportAllGuardsByMonth(path);
    });

    menu->exec();
}

/**
 * 导出每个月所有舰长的表格
 * 横向：月份
 * 纵向：UID 昵称 备注
 */
void MainWindow::exportAllGuardsByMonth(QString exportPath)
{
    QString readPath = rt->dataPath + "guard_histories/" + ac->roomId + ".csv";
    if (!isFileExist(readPath))
    {
        showError("不存在舰长记录", readPath);
        return ;
    }

    /// 购买舰长的记录表
    struct GuardTrade
    {
        QString date;
        QString nickname;
        QString giftName;
        int count;
        qint64 uid;
        QString remark;
    };

    /// 读取表格数据
    // 默认表格格式：日期 时间 昵称 礼物 数量 累计 UID 备注
    QString content = readTextFileAutoCodec(readPath);
    QStringList lines = content.split("\n", QString::SkipEmptyParts);
    QList<GuardTrade> trades;
    for (int i = 1; i < lines.size(); ++i)
    {
        QStringList cells = lines.at(i).split(",");
        if (cells.size() < 7)
        {
            qWarning() << "不符合的格式：" << lines.at(i);
            continue;
        }
        GuardTrade trade{cells[0], cells[2], cells[3], cells[4].toInt(), cells[6].toLongLong(), cells.size() >= 8 ? cells[7] : ""};
        trades.append(trade);
    }

    // 获取所有月份
    QList<QString> months; // 月份表，格式：2021-1,2021-2,2021-3...
    QString prevMonth = "";
    for (GuardTrade& trade: trades)
    {
        QString month = QDate::fromString(trade.date, "yyyy-MM-dd").toString("yyyy.MM");
        trade.date = month;
        if (prevMonth == month)
            continue;
        months.append(month);
        prevMonth = month;
    }

    // 获取所有用户
    QList<qint64> users;
    QList<QString> nicknames;
    for (const GuardTrade& trade: trades)
    {
        if (!users.contains(trade.uid))
        {
            users.append(trade.uid);
            nicknames.append(trade.nickname); // 只用第一次上船时的昵称
        }
    }

    /// 映射向量
    int guardCount[users.size()][months.size()][3];
    memset(guardCount, 0, sizeof (int) * users.size() * months.size() * 3);

    const int ZD = 0; // 总督索引
    const int TD = 1; // 提督
    const int JZ = 2; // 舰长

    for (const GuardTrade& trade: trades)
    {
        int userIndex = users.indexOf(trade.uid);
        int monthIndex = months.indexOf(trade.date);
        Q_ASSERT(userIndex > -1);
        Q_ASSERT(monthIndex > -1);

        if (trade.giftName == "总督")
            guardCount[userIndex][monthIndex][ZD] += trade.count;
        else if (trade.giftName == "提督")
            guardCount[userIndex][monthIndex][TD] += trade.count;
        else if (trade.giftName == "舰长")
            guardCount[userIndex][monthIndex][JZ] += trade.count;
        else
            qWarning() << "未知的礼物：" << trade.giftName;
    }

    /// 导出表格
    // 标题
    QString fullText = "UID,昵称";
    for (int i = 0; i < months.size(); i++)
        fullText += "," + months.at(i);

    // 内容每一行
    for (int i = 0; i < users.size(); i++)
    {
        // 添加用户ID和昵称列
        fullText += "\n" + snum(users.at(i)) + "," + nicknames.at(i);
        // 添加每次上船的标记
        for (int j = 0; j < months.size(); j++)
        {
            QStringList cellList;
            if (guardCount[i][j][ZD] != 0)
                cellList.append("总督" + (guardCount[i][j][ZD] > 1 ? snum(guardCount[i][j][ZD]) : ""));
            if (guardCount[i][j][TD] != 0)
                cellList.append("提督" + (guardCount[i][j][TD] > 1 ? snum(guardCount[i][j][TD]) : ""));
            if (guardCount[i][j][JZ] != 0)
                cellList.append("舰长" + (guardCount[i][j][JZ] > 1 ? snum(guardCount[i][j][JZ]) : ""));
            fullText += "," + cellList.join(" / ");
        }
    }

    writeTextFile(exportPath, fullText, liveService->recordFileCodec);
}


void MainWindow::on_MSAreaCodeEdit_editingFinished()
{
    us->setValue("mstts/areaCode", ui->MSAreaCodeEdit->text());
    if (voiceService->msTTS)
    {
        voiceService->msTTS->setAreaCode(ui->MSAreaCodeEdit->text());
    }
}

void MainWindow::on_MSSubscriptionKeyEdit_editingFinished()
{
    us->setValue("mstts/subscriptionKey", ui->MSSubscriptionKeyEdit->text());
    if (voiceService->msTTS)
    {
        voiceService->msTTS->setSubscriptionKey(ui->MSSubscriptionKeyEdit->text());
    }
}

void MainWindow::on_MS_TTS__SSML_Btn_clicked()
{
    bool ok;
    QString fmt = TextInputDialog::getText(this, "微软语音SSML格式", "使用 %text% 替换要朗读的文字。<a href='https://docs.microsoft.com/zh-cn/azure/cognitive-services/speech-service/speech-synthesis-markup?tabs=csharp#create-an-ssml-document'>SSML文档</a>", voiceService->msTTSFormat, &ok);
    if (!ok)
        return ;
    if (fmt.trimmed().isEmpty())
        fmt = DEFAULT_MS_TTS_SSML_FORMAT;
    us->setValue("mstts/format", voiceService->msTTSFormat = fmt);
}

void MainWindow::on_receivePrivateMsgCheck_clicked()
{
    us->setValue("privateMsg/enabled", ui->receivePrivateMsgCheck->isChecked());
}

void MainWindow::on_receivePrivateMsgCheck_stateChanged(int arg1)
{
    if (arg1)
    {
        if (!liveService->privateMsgTimer)
        {
            // 初始化时钟
            liveService->privateMsgTimer = new QTimer(this);
            liveService->privateMsgTimer->setInterval(5000);
            liveService->privateMsgTimer->setSingleShot(false);
            connect(liveService->privateMsgTimer, &QTimer::timeout, liveService, &LiveRoomService::refreshPrivateMsg);
        }

        liveService->privateMsgTimer->start();
    }
    else
    {
        if (liveService->privateMsgTimer)
            liveService->privateMsgTimer->stop();
    }
}

void MainWindow::on_processUnreadMsgCheck_clicked()
{
    us->setValue("privateMsg/processUnread", ui->processUnreadMsgCheck->isChecked());
}

void MainWindow::on_TXSecretIdEdit_editingFinished()
{
    QString text = ui->TXSecretIdEdit->text();
    us->setValue("tx_nlp/secretId", text);
    chatService->txNlp->setSecretId(text);
}

void MainWindow::on_TXSecretKeyEdit_editingFinished()
{
    QString text = ui->TXSecretKeyEdit->text();
    us->setValue("tx_nlp/secretKey", text);
    chatService->txNlp->setSecretKey(text);
}

void MainWindow::on_saveDanmakuToFileButton_clicked()
{
    QDir dir(rt->dataPath + "danmaku_histories");
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void MainWindow::on_calculateDailyDataButton_clicked()
{
    QDir dir(rt->dataPath + "live_daily");
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void MainWindow::on_syntacticSugarCheck_clicked()
{
    us->setValue("programming/syntacticSugar", ui->syntacticSugarCheck->isChecked());
}

void MainWindow::on_forumButton_clicked()
{
    QDesktopServices::openUrl(QUrl("http://live.lyixi.com"));
}

void MainWindow::on_complexCalcCheck_clicked()
{
    us->setValue("programming/compexCalc", us->complexCalc = ui->complexCalcCheck->isChecked());
}

/**
 * 饭贩好评
 * 需要 Cookie 带有 PEA_AU 字段
 */
void MainWindow::on_positiveVoteCheck_clicked()
{
    if (ac->browserData.isEmpty()) // 未登录
    {
        QDesktopServices::openUrl(QUrl("https://play-live.bilibili.com/details/1653383145397"));
        return ;
    }

    us->setValue("ask/positiveVote", true);
    if (_fanfanLike > 0) // 取消好评
    {
        if (QMessageBox::question(this, "取消好评", "您已为神奇弹幕点赞，要残忍地取消吗？", QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
        {
            QTimer::singleShot(200, [=]{
                ui->positiveVoteCheck->setChecked(true);
            });
            return ;
        }
    }

    if (ac->browserCookie.contains("PEA_AU=")) // 浏览器复制的cookie，一键好评
    {
        positiveVote();
    }
    else // 先获取登录信息，再进行好评
    {
        fanfanLogin();
    }
}

void MainWindow::on_saveLogCheck_clicked()
{
    us->setValue("debug/logFile", ui->saveLogCheck->isChecked());
    if (ui->saveLogCheck->isChecked())
    {
        showNotify("打印日志", "重启生效：安装路径/debug.log");
    }
}

void MainWindow::on_stringSimilarCheck_clicked()
{
    us->setValue("programming/stringSimilar", us->useStringSimilar = ui->stringSimilarCheck->isChecked());
}

void MainWindow::on_onlineRankListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    openLink("https://space.bilibili.com/" + snum(item->data(Qt::UserRole).toLongLong()));
}

void MainWindow::on_actionShow_Gift_List_triggered()
{
    if (pl->allGiftMap.empty())
    {
        liveService->getGiftList();
        QTimer::singleShot(2000, [=]{
            if (pl->allGiftMap.size())
                on_actionShow_Gift_List_triggered();
        });
        return ;
    }

    // 创建MV
    QTableView* view = new QTableView(nullptr);
    QStandardItemModel* model = new QStandardItemModel(view);
    view->setModel(model);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    // view->setAttribute(Qt::WA_ShowModal, true);
    view->setAttribute(Qt::WA_DeleteOnClose, true);
    view->setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::Dialog);
    view->setWordWrap(true);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setWindowTitle("当前直播间的礼物");

    // 读取数据
    model->setColumnCount(4);
    model->setRowCount(pl->allGiftMap.size());
    model->setHorizontalHeaderLabels({"ID", "名称", "价格", "描述"});
    QColor goldColor("#CD7F32");
    QColor silverColor("#C0C0C0");
    int row = 0;
    for (auto it = pl->allGiftMap.begin(); it != pl->allGiftMap.end(); it++)
    {
        const LiveDanmaku& info = it.value();
        model->setItem(row, 0, new QStandardItem(snum(info.getGiftId())));
        model->setItem(row, 1, new QStandardItem(info.getGiftName()));
        if (info.getTotalCoin() > 0)
        {
            auto item = new QStandardItem(snum(info.getTotalCoin()));
            if (info.isGoldCoin())
                item->setForeground(goldColor);
            else if (info.isSilverCoin())
                item->setForeground(silverColor);
            model->setItem(row, 2, item);
        }
        model->setItem(row, 3, new QStandardItem(info.extraJson.value("desc").toString()));
        row++;
    }

    // 显示控件
    view->setGeometry(this->geometry());
    view->show();
}

void MainWindow::on_liveOpenCheck_clicked()
{
    us->set("live-open/enabled", ui->liveOpenCheck->isChecked());
    if (ui->liveOpenCheck->isChecked())
    {
        if (liveService->isLiving())
            liveOpenService->start();
    }
    else
    {
        if (liveOpenService)
            liveOpenService->endIfStarted();
    }
}

void MainWindow::on_identityCodeEdit_editingFinished()
{
    ac->identityCode = ui->identityCodeEdit->text();
    us->set("live-open/identityCode", ac->identityCode);
}

void MainWindow::on_recordDataButton_clicked()
{
    QDir dir(rt->dataPath + "record");
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void MainWindow::on_actionLogout_triggered()
{
    if (QMessageBox::question(this, "退出登录", "是否退出登录？\n这将清除该账号cookie，需要重新登录方可使用",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
        return ;

    us->setValue("danmaku/browserCookie", ac->browserCookie = "");
    us->setValue("danmaku/browserData", ac->browserData = "");
    ac->userCookies = QVariant();
    ac->csrf_token = "";
    ac->cookieUid = "";
    ac->cookieUname = "";
    ac->cookieToken = "";
    ac->cookieULevel = 0;
    ac->cookieGuardLevel = 0;

    ui->robotNameButton->setText("点此重新登录");
    ui->robotNameButton->adjustMinimumSize();
    ui->robotInfoWidget->setMinimumWidth(ui->robotNameButton->width());
}

void MainWindow::on_saveToSqliteCheck_clicked()
{
    if (ui->saveToSqliteCheck->isChecked() && !hasPermission())
    {
        on_actionBuy_VIP_triggered();
        ui->saveToSqliteCheck->setChecked(false);
        return ;
    }

    us->set("db/sqlite", saveToSqlite = ui->saveToSqliteCheck->isChecked());

    ui->saveCmdToSqliteCheck->setEnabled(saveToSqlite);
    if (saveToSqlite)
    {
        sqlService.open();
    }
    else
    {
        sqlService.close();
    }
}

void MainWindow::on_databaseQueryButton_clicked()
{
    on_actionQueryDatabase_triggered();
}

void MainWindow::on_actionQueryDatabase_triggered()
{
    showSqlQueryResult("");
}

void MainWindow::on_saveCmdToSqliteCheck_clicked()
{
    us->set("db/cmd", saveCmdToSqlite = ui->saveCmdToSqliteCheck->isChecked());
}

void MainWindow::on_recordFormatCheck_clicked()
{
    us->set("record/format", ui->recordFormatCheck->isChecked());
    if (ui->recordFormatCheck->isChecked() && rt->ffmpegPath.isEmpty())
    {
#ifdef Q_OS_WIN
        const QString ffmpegApp = "ffmpeg.exe";
#else
        const QString ffmpegApp = "ffmpeg";
#endif

        // 尝试自动获取 ffmpeg.exe 位置
        if (isFileExist(QApplication::applicationDirPath() + "/" + ffmpegApp)) // 应用目录下
        {
            rt->ffmpegPath = QApplication::applicationDirPath() + "/" + ffmpegApp;
        }
        else if (isFileExist(QApplication::applicationDirPath() + "/tools/" + ffmpegApp))
        {
            rt->ffmpegPath = QApplication::applicationDirPath() + "/tools" + ffmpegApp;
        }
        else if (isFileExist(QApplication::applicationDirPath() + "/tools/ffmpeg/" + ffmpegApp))
        {
            rt->ffmpegPath = QApplication::applicationDirPath() + "/tools/ffmpeg/" + ffmpegApp;
        }
        else if (isFileExist(QApplication::applicationDirPath() + "/www/ffmpeg/" + ffmpegApp))
        {
            rt->ffmpegPath = QApplication::applicationDirPath() + "/www/ffmpeg" + ffmpegApp;
        }
        else // 检查环境变量
        {
            QStringList environmentList = QProcess::systemEnvironment();
            rt->ffmpegPath = "";
            foreach (QString environment, environmentList)
            {
                if (environment.startsWith("Path="))
                {
                    QStringList sl = environment.right(environment.length() - 5).split(";", QString::SkipEmptyParts);
                    foreach (QString d, sl)
                    {
                        QString path = QDir(d).absoluteFilePath(ffmpegApp);
                        if (QFileInfo(path).exists())
                        {
                            qInfo() << "自动设置" << ffmpegApp << "位置：" << path;
                            rt->ffmpegPath = path;
                            us->setValue("record/ffmpegPath", path);
                            break;
                        }
                    }
                    break;
                }
            }
        }

        // 手动获取
        if (rt->ffmpegPath.isEmpty())
        {
            on_ffmpegButton_clicked();
        }

        // 取消勾选
        if (rt->ffmpegPath.isEmpty())
        {
            ui->recordFormatCheck->setChecked(false);
            us->set("record/format", false);
        }
    }
}

void MainWindow::on_ffmpegButton_clicked()
{
    QString oldPath = us->value("recent/ffmpegPath", "").toString();
    QString path = QFileDialog::getOpenFileName(this, "选择 ffmpeg.exe 位置", oldPath, "应用程序 (*.exe)");
    if (path.isEmpty())
        return ;
    us->setValue("recent/ffmpegPath", path);
    us->setValue("record/ffmpegPath", path);
    rt->ffmpegPath = path;
}

void MainWindow::on_closeGuiCheck_clicked()
{
    us->closeGui = ui->closeGuiCheck->isChecked();
    us->setValue("mainwindow/closeGui", us->closeGui = ui->closeGuiCheck->isChecked());

    if (us->closeGui)
    {
        // 关闭已有的GUI效果
        auto lw = ui->onlineRankListWidget;
        while (lw->count())
        {
            auto item = lw->item(0);
            auto widget = lw->itemWidget(item);
            lw->takeItem(0);
            delete widget;
        }
        ui->onlineRankDescLabel->setPixmap(QPixmap());

        lw = ui->giftListWidget;
        while (lw->count())
        {
            auto item = lw->item(0);
            auto widget = lw->itemWidget(item);
            lw->takeItem(0);
            delete widget;
        }
    }
}

void MainWindow::on_gptKeyEdit_textEdited(const QString &arg1)
{
    us->setValue("chatgpt/open_ai_key", us->open_ai_key = arg1);
}

void MainWindow::on_chatGPTModelNameCombo_activated(const QString &arg1)
{
    us->setValue("chatgpt/model_name", us->chatgpt_model_name = arg1);
}

void MainWindow::on_chatGPTMaxTokenCountSpin_valueChanged(int arg1)
{
    us->setValue("chatgpt/max_token_count", us->chatgpt_max_token_count = arg1);
}

void MainWindow::on_chatGPTMaxContextCountSpin_valueChanged(int arg1)
{
    us->setValue("chatgpt/max_context_count", us->chatgpt_max_context_count = arg1);
}

void MainWindow::on_chatGPTKeyButton_clicked()
{
    QFuture<bool> future = NetUtil::checkPublicNetThread(true);
    NetUtil::checkPublicNetThread(false);
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

    connect(watcher, &QFutureWatcher<void>::finished,[=]{
        // qInfo() << "可连接外网：" << future.result();
        QString url;
        if(future.result())
        {
            url = "https://www.openai.com";
        }
        else
        {
            url = "https://api2d.com";
        }
        QDesktopServices::openUrl(url);
    });
    watcher->setFuture(future);
}

void MainWindow::on_chatGPTRadio_clicked()
{
    ui->chatTxRadio->setChecked(false);
    ui->chatGPTRadio->setChecked(true);
    us->setValue("chat/paltform", chatService->chatPlatform = ChatGPT);
}

void MainWindow::on_chatTxRadio_clicked()
{
    ui->chatTxRadio->setChecked(true);
    ui->chatGPTRadio->setChecked(false);
    us->setValue("chat/paltform", chatService->chatPlatform = TxNLP);
}
