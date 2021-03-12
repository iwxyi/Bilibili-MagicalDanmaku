#include "picturebrowser.h"
#include "ui_picturebrowser.h"

PictureBrowser::PictureBrowser(QSettings &settings, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PictureBrowser),
    settings(settings)
{
    ui->setupUi(this);

    // 图标大小
    QActionGroup* iconSizeGroup = new QActionGroup(this);
    iconSizeGroup->addAction(ui->actionIcon_Largest);
    iconSizeGroup->addAction(ui->actionIcon_Large);
    iconSizeGroup->addAction(ui->actionIcon_Middle);
    iconSizeGroup->addAction(ui->actionIcon_Small);
    if (settings.contains("picturebrowser/iconSize"))
    {
        int size = settings.value("picturebrowser/iconSize").toInt();
        ui->listWidget->setIconSize(QSize(size, size));

        if (size == 64)
            ui->actionIcon_Small->setChecked(true);
        else if (size == 128)
            ui->actionIcon_Middle->setChecked(true);
        else if (size == 256)
            ui->actionIcon_Large->setChecked(true);
        else if (size == 512)
            ui->actionIcon_Largest->setChecked(true);
    }

    connect(ui->splitter, &QSplitter::splitterMoved, this, [=](int, int){
        QSize size(ui->listWidget->iconSize());
        ui->listWidget->setIconSize(QSize(1, 1));
        ui->listWidget->setIconSize(size);
    });

    readSortFlags();

    // 排序
    QActionGroup* sortTypeGroup = new QActionGroup(this);
    sortTypeGroup->addAction(ui->actionSort_By_Time);
    sortTypeGroup->addAction(ui->actionSort_By_Name);
    if (sortFlags & QDir::Name)
        ui->actionSort_By_Name->setChecked(true);
    else // 默认按时间
        ui->actionSort_By_Time->setChecked(true);

    QActionGroup* sortReverseGroup = new QActionGroup(this);
    sortReverseGroup->addAction(ui->actionSort_DESC);
    sortReverseGroup->addAction(ui->actionSort_AESC);
    if (sortFlags &QDir::Reversed)
        ui->actionSort_DESC->setChecked(true);
    else // 默认从小到大（新的在后面）
        ui->actionSort_AESC->setChecked(true);

    // 播放动图
    slideTimer = new QTimer(this);
    int interval = settings.value("picturebrowser/slideInterval", 100).toInt();
    slideTimer->setInterval(interval);
    connect(slideTimer, &QTimer::timeout, this, [&]{
        int row = ui->listWidget->currentRow();
        if (row == -1)
            return ;

        // 切换到下一帧
        int targetRow = 0;
        auto selectedItems = ui->listWidget->selectedItems();

        if (slideInSelected && selectedItems.size() > 1 && selectedItems.contains(ui->listWidget->currentItem())) // 在多个选中项中
        {
            int index = selectedItems.indexOf(ui->listWidget->currentItem());
            index++;
            if (index == selectedItems.size())
                index = 0;
            targetRow = ui->listWidget->row(selectedItems.at(index));
        }
        else
        {
            int count = ui->listWidget->count();
            if (row < count-1)
            {
                targetRow = row + 1; // 播放下一帧
            }
            else if (count > 1 && row == count-1
                     && settings.value("picturebrowser/slideReturnFirst", false).toBool())
            {
                if (ui->listWidget->item(0)->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
                    // 子目录第一项是返回上一级
                    targetRow = 1; // 从头(第一张图)开始播放
                else
                    targetRow = 0; // 从头开始播放
            }
        }
        ui->listWidget->setCurrentRow(targetRow, QItemSelectionModel::Current);
    });
    QActionGroup* intervalGroup = new QActionGroup(this);
    intervalGroup->addAction(ui->actionSlide_16ms);
    intervalGroup->addAction(ui->actionSlide_33ms);
    intervalGroup->addAction(ui->actionSlide_100ms);
    intervalGroup->addAction(ui->actionSlide_200ms);
    intervalGroup->addAction(ui->actionSlide_500ms);
    intervalGroup->addAction(ui->actionSlide_1000ms);
    intervalGroup->addAction(ui->actionSlide_3000ms);
    if (interval == 16)
        ui->actionSlide_16ms->setChecked(true);
    else if (interval == 33)
        ui->actionSlide_33ms->setChecked(true);
    else if (interval == 50)
        ui->actionSlide_50ms->setChecked(true);
    else if (interval == 100)
        ui->actionSlide_100ms->setChecked(true);
    else if (interval == 200)
        ui->actionSlide_200ms->setChecked(true);
    else if (interval == 500)
        ui->actionSlide_500ms->setChecked(true);
    else if (interval == 1000)
        ui->actionSlide_1000ms->setChecked(true);
    else if (interval == 3000)
        ui->actionSlide_3000ms->setChecked(true);

    // 生成位置
    QActionGroup* createFolderGroup = new QActionGroup(this);
    createFolderGroup->addAction(ui->actionCreate_To_Origin_Folder);
    createFolderGroup->addAction(ui->actionCreate_To_One_Folder);
    bool toOne = settings.value("gif/createToOne", false).toBool();
    if (toOne)
        ui->actionCreate_To_One_Folder->setChecked(true);
    else
        ui->actionCreate_To_Origin_Folder->setChecked(true);

    // GIF录制参数
    QActionGroup* gifRecordGroup = new QActionGroup(this);
    gifRecordGroup->addAction(ui->actionGIF_Use_Record_Interval);
    gifRecordGroup->addAction(ui->actionGIF_Use_Display_Interval);
    bool gifUseRecordInterval = settings.value("gif/recordInterval", true).toBool();
    ui->actionGIF_Use_Record_Interval->setChecked(gifUseRecordInterval);
    ui->actionGIF_Use_Display_Interval->setChecked(!gifUseRecordInterval);
    QActionGroup* gifDitherGroup = new QActionGroup(this);
    gifDitherGroup->addAction(ui->actionDither_Enabled);
    gifDitherGroup->addAction(ui->actionDither_Disabled);
    if (settings.value("gif/dither", true).toBool())
        ui->actionDither_Enabled->setChecked(true);
    else
        ui->actionDither_Disabled->setChecked(true);

    QActionGroup* gifCompressGroup = new QActionGroup(this);
    gifCompressGroup->addAction(ui->actionGIF_Compress_None);
    gifCompressGroup->addAction(ui->actionGIF_Compress_x2);
    gifCompressGroup->addAction(ui->actionGIF_Compress_x4);
    gifCompressGroup->addAction(ui->actionGIF_Compress_x8);
    int gifCompress = settings.value("gif/compress", 0).toInt();
    if (gifCompress == 0)
        ui->actionGIF_Compress_None->setChecked(true);
    else if (gifCompress == 1)
        ui->actionGIF_Compress_x2->setChecked(true);
    else if (gifCompress == 2)
        ui->actionGIF_Compress_x4->setChecked(true);
    else if (gifCompress == 3)
        ui->actionGIF_Compress_x8->setChecked(true);

    // 图像转换选项
    QActionGroup* convertGroup1 = new QActionGroup(this);
    convertGroup1->addAction(ui->actionAutoColor);
    convertGroup1->addAction(ui->actionColorOnly);
    convertGroup1->addAction(ui->actionMonoOnly);

    QActionGroup* convertGroup2 = new QActionGroup(this);
    convertGroup2->addAction(ui->actionDiffuseDither);
    convertGroup2->addAction(ui->actionOrderedDither);
    convertGroup2->addAction(ui->actionThresholdDither);

    QActionGroup* convertGroup3 = new QActionGroup(this);
    convertGroup3->addAction(ui->actionThresholdAlphaDither);
    convertGroup3->addAction(ui->actionOrderedAlphaDither);
    convertGroup3->addAction(ui->actionDiffuseAlphaDither);

    QActionGroup* convertGroup4 = new QActionGroup(this);
    convertGroup4->addAction(ui->actionPreferDither);
    convertGroup4->addAction(ui->actionAvoidDither);
    convertGroup4->addAction(ui->actionAutoDither);
    convertGroup4->addAction(ui->actionNoOpaqueDetection);
    convertGroup4->addAction(ui->actionNoFormatConversion);

    imageConversion = (Qt::ImageConversionFlags)settings.value("gif/imageConversion", 0).toInt();
    fromImageConversionFlag();

    connect(this, &PictureBrowser::signalGeneralGIFProgress, this, [=](int index){
        progressBar->setValue(index);
    });

    connect(this, &PictureBrowser::signalGeneralGIFFinished, this, [=](QString path){
        progressBar->setMaximum(0);
        progressBar->hide();

        // 显示在当前列表中
        QFileInfo info(path);
        if (info.absoluteDir() == currentDirPath)
        {
            // 还是这个目录，直接加到这个地方
            QString name = info.baseName();
            if (name.contains(" "))
                name = name.right(name.length() - name.indexOf(" ") - 1);

            QListWidgetItem* item = new QListWidgetItem(QIcon(path), info.fileName(), ui->listWidget);
            item->setData(FilePathRole, info.absoluteFilePath());
            item->setToolTip(info.fileName());
        }

        // 操作对话框
        auto result = QMessageBox::information(this, "处理完毕", "路径：" + path, "打开文件", "打开文件夹", "取消", 0, 2);
        if(result == 0) // 打开文件
        {
        #ifdef Q_OS_WIN32
            QString m_szHelpDoc = QString("file:///") + path;
            bool is_open = QDesktopServices::openUrl(QUrl(m_szHelpDoc, QUrl::TolerantMode));
            if(!is_open)
                PBDEB << "打开图片失败：" << path;
        #else
            QString  cmd = QString("xdg-open ")+ m_szHelpDoc;　　　　　　　　//在linux下，可以通过system来xdg-open命令调用默认程序打开文件；
            system(cmd.toStdString().c_str());
        #endif
        }
        else if (result == 1) // 打开文件夹
        {
            if (path.isEmpty() || !QFileInfo(path).exists())
                return ;

            QProcess process;
            path.replace("/", "\\");
            QString cmd = QString("explorer.exe /select, \"%1\"").arg(path);
            process.startDetached(cmd);
        }
    });

    // 预览设置
    bool resizeAutoInit = settings.value("picturebrowser/resizeAutoInit", true).toBool();
    ui->actionResize_Auto_Init->setChecked(resizeAutoInit);
    ui->previewPicture->setResizeAutoInit(resizeAutoInit);

    // 快速分类
    fastSort = settings.value("picturebrowser/fastSort", true).toBool();

    // 状态栏
    selectLabel = new QLabel;
    ui->statusbar->addWidget(selectLabel);

    sizeLabel = new QLabel(this);
    ui->statusbar->addWidget(sizeLabel);

    progressBar = new QProgressBar(this);
    progressBar->hide();
    ui->statusbar->addPermanentWidget(progressBar);

    // 样式表
    QString progressBarQss= "QProgressBar{\
                                    border: none;\
                                    color: white;\
                                    text-align: center;\
                                    background: rgb(68, 69, 73);\
                            }\
                            QProgressBar::chunk {\
                                    border: none;\
                                    background: rgb(0, 160, 230);\
                            }";
    progressBar->setStyleSheet(progressBarQss);
}

PictureBrowser::~PictureBrowser()
{
    delete ui;

    QDir tempDir(tempDirPath);
    if (tempDir.exists())
        tempDir.removeRecursively();
}

void PictureBrowser::readDirectory(QString targetDir)
{
    // 如果之前就有目录，则进行临时目录的删除操作
    if (!rootDirPath.isEmpty())
    {
        QDir tempDir(tempDirPath);
        if (tempDir.exists())
            tempDir.removeRecursively();
    }

    // 读取目录
    rootDirPath = targetDir;
    tempDirPath = QDir(targetDir).absoluteFilePath(TEMP_DIRECTORY);
    recycleDir = QDir(QDir(tempDirPath).absoluteFilePath(RECYCLE_DIRECTORY));
    recycleDir.mkpath(recycleDir.absolutePath());
    enterDirectory(targetDir);
}

void PictureBrowser::enterDirectory(QString targetDir)
{
    currentDirPath = targetDir;
    bool isSubDir = currentDirPath != rootDirPath;
    ui->actionBack_Prev_Directory->setEnabled(isSubDir);
    ui->actionExtra_Selected->setEnabled(isSubDir);
    ui->actionExtra_And_Delete->setEnabled(isSubDir);
    ui->actionDelete_Unselected->setEnabled(isSubDir);
    ui->actionDelete_Up_Files->setEnabled(isSubDir);
    ui->actionDelete_Down_Files->setEnabled(isSubDir);

    ui->listWidget->clear();
    if (targetDir.isEmpty())
        return ;
    ui->previewPicture->setPixmap(QPixmap());
    ui->previewPicture->resetScale();

    if (currentDirPath != rootDirPath)
    {
        QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/cd_up"), BACK_PREV_DIRECTORY, ui->listWidget);
        item->setData(FilePathRole, BACK_PREV_DIRECTORY);
    }

    // 尺寸大小
    QSize maxIconSize = ui->listWidget->iconSize();
    if (maxIconSize.width() <= 16 || maxIconSize.height() <= 16)
        maxIconSize = QSize(32, 32);

    // 目录角标
    QPixmap dirIcon(":/icons/directory");
    dirIcon = dirIcon.scaled(QSize(maxIconSize.width()/3, maxIconSize.height()/3),
                                 Qt::AspectRatioMode::KeepAspectRatio);

    // 绘制工具
    QFont dirCountFont = this->font();
    dirCountFont.setPointSize(dirIcon.height()/4);
    dirCountFont.setBold(true);
    QFont gifMarkFont = this->font();
    gifMarkFont.setBold(true);

    // 读取目录的图片和文件夹
    QDir dir(targetDir);
    QList<QFileInfo> infos = dir.entryInfoList(
                QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                                              sortFlags);
    foreach (QFileInfo info, infos)
    {
        QString name = info.fileName();
        QString suffix = info.suffix();
        if (name == TEMP_DIRECTORY || name == GENERAL_DIRECTORY)
            continue;
        if (name.contains(" "))
            name = name.right(name.length() - name.indexOf(" ") - 1);
        QListWidgetItem* item;
        if (info.isDir())
        {
            // 获取文件夹下第一张图
            QString path;
            auto infos = QDir(info.absoluteFilePath()).entryInfoList(getImageFilters(),
                                                  QDir::Files | QDir::NoSymLinks,
                                                  sortFlags);
            if (infos.size())
                path = infos.first().absoluteFilePath();

            // 目录显示数量
            QPixmap myDirIcon = dirIcon;
            QPainter dirPainter(&myDirIcon);
            dirPainter.setFont(dirCountFont);
            dirPainter.setPen(QColor::fromHsl(rand() % 360, rand() % 256, rand() % 200)); // 随机颜色
            dirPainter.drawText(QRect(0, myDirIcon.height()/8,myDirIcon.width(),myDirIcon.height()*7/8), Qt::AlignCenter, QString::number(infos.size()));
            dirPainter.end();

            // 如果有图，则同时合并pixmap和icon
            QPixmap pixmap(path);
            if (!path.isEmpty())
            {
                pixmap = pixmap.scaled(maxIconSize, Qt::AspectRatioMode::KeepAspectRatio);

                // 画目录Icon到Pixmap上
                QPainter painter(&pixmap);
                painter.drawPixmap(QRect(pixmap.width() - myDirIcon.width(),
                                         pixmap.height() - myDirIcon.height(),
                                         myDirIcon.width(),
                                         myDirIcon.height()),
                                   myDirIcon);
            }
            else // 如果没有path，则只显示一个空文件夹图标
            {
                pixmap = myDirIcon;
            }
            item = new QListWidgetItem(QIcon(pixmap), name, ui->listWidget);
        }
        else if (info.isFile())
        {
            // gif后缀显示出来
            QString suffix = info.suffix();
            if (!getImageFilters().contains("*." + suffix))
                continue;
            if (name.contains("."))
                name = name.left(name.lastIndexOf("."));
            QPixmap pixmap(info.absoluteFilePath());
            if (pixmap.width() > maxIconSize.width() || pixmap.height() > maxIconSize.height())
                pixmap = pixmap.scaled(maxIconSize, Qt::AspectRatioMode::KeepAspectRatio);
            // 绘制标记
            if (suffix == "gif")
            {
                QPainter painter(&pixmap);
                painter.setFont(gifMarkFont);
                QFontMetrics fm(gifMarkFont);
                QSize size(fm.horizontalAdvance("GIF "), fm.lineSpacing());
                int padding = qMin(size.width(), size.height()) / 5;
                QRect rect(pixmap.width() - size.width() + padding,
                           padding,
                           size.width() - padding*2,
                           size.height() - padding*2);
                QPainterPath path;
                path.addRoundedRect(rect, 3, 3);
                painter.fillPath(path, QColor(255, 255, 255, 128));
                // painter.setPen(QColor(32, 32, 32, 192));
                painter.setPen(QColor::fromHsl(rand() % 360, rand() % 256, rand() % 200)); // 随机颜色
                painter.drawText(rect, Qt::AlignCenter, tr("GIF"));
            }
            item = new QListWidgetItem(QIcon(pixmap), name, ui->listWidget);
        }
        else
            continue;
        item->setData(FilePathRole, info.absoluteFilePath());
        item->setToolTip(info.fileName());
    }

    restoreCurrentViewPos();
}

void PictureBrowser::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    QSize size(ui->listWidget->iconSize());
    ui->listWidget->setIconSize(QSize(1, 1));
    ui->listWidget->setIconSize(size);
}

void PictureBrowser::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    restoreGeometry(settings.value("picturebrowser/geometry").toByteArray());
    restoreState(settings.value("picturebrowser/state").toByteArray());
    ui->splitter->restoreGeometry(settings.value("picturebrowser/splitterGeometry").toByteArray());
    ui->splitter->restoreState(settings.value("picturebrowser/splitterState").toByteArray());

    // 初始化缩放
    static bool inited = false;
    if (!inited)
    {
        inited = true;
        ui->previewPicture->resetScale();
    }

    int is = settings.value("picturebrowser/iconSize", 64).toInt();
    ui->listWidget->setIconSize(QSize(1, 1));
    ui->listWidget->setIconSize(QSize(is, is));
}

void PictureBrowser::closeEvent(QCloseEvent *event)
{
    settings.setValue("picturebrowser/geometry", this->saveGeometry());
    settings.setValue("picturebrowser/state", this->saveState());
    settings.setValue("picturebrowser/splitterGeometry", ui->splitter->saveGeometry());
    settings.setValue("picturebrowser/splitterState", ui->splitter->saveState());

    slideTimer->stop();
}

void PictureBrowser::keyPressEvent(QKeyEvent *event)
{
    auto key = event->key();

    switch (key)
    {
    case Qt::Key_Escape:
        if (slideTimer->isActive())
        {
            slideTimer->stop();
            return ;
        }
        break;
    }

    if (fastSort && event->modifiers() == Qt::AltModifier)
    {
        if (key >= Qt::Key_A && key <= Qt::Key_Z)
        {
            char ch = key - Qt::Key_A + 'a';
            fastSortItems(QString(ch));
        }
        else if (key >= Qt::Key_0 && key <= Qt::Key_9)
        {
            char ch = key - Qt::Key_0 + '0';
            fastSortItems(QString(ch));
        }
    }

    QWidget::keyPressEvent(event);
}

void PictureBrowser::readSortFlags()
{
    sortFlags = static_cast<QDir::SortFlags>(settings.value("picturebrowser/sortType", QDir::Time).toInt());

    if (settings.value("picturebrowser/sortReversed", false).toBool())
        sortFlags |= QDir::Reversed;
}

void PictureBrowser::showCurrentItemPreview()
{
    on_listWidget_currentItemChanged(ui->listWidget->currentItem(), nullptr);
}

void PictureBrowser::setListWidgetIconSize(int x)
{
    ui->listWidget->setIconSize(QSize(x, x));
    settings.setValue("picturebrowser/iconSize", x);
    on_actionRefresh_triggered();
}

void PictureBrowser::saveCurrentViewPos()
{
    if (currentDirPath.isEmpty())
        return ;
    if (ui->listWidget->currentItem() == nullptr)
        return ;
    ListProgress progress{ui->listWidget->currentRow(),
                         ui->listWidget->verticalScrollBar()->sliderPosition(),
                         ui->listWidget->currentItem()->data(FilePathRole).toString()};
    viewPoss[currentDirPath] = progress;
}

/**
 * 还原当前文件夹的当前项、滚动位置为上次浏览
 */
void PictureBrowser::restoreCurrentViewPos()
{
    if (currentDirPath.isEmpty() || !viewPoss.contains(currentDirPath))
    {
        if (ui->listWidget->count() > 0)
        {
            if (ui->listWidget->count() > 1
                    && ui->listWidget->item(0)->data(FilePathRole) == BACK_PREV_DIRECTORY)
                ui->listWidget->setCurrentRow(1);
            else
                ui->listWidget->setCurrentRow(0);
        }
        return ;
    }

    ListProgress progress = viewPoss.value(currentDirPath);
    ui->listWidget->setCurrentRow(progress.index);
    ui->listWidget->verticalScrollBar()->setSliderPosition(progress.scroll);
    if (!ui->listWidget->currentItem() || ui->listWidget->currentItem()->data(FilePathRole) != progress.file)
    {
        for (int i = 0; i < ui->listWidget->count(); i++)
        {
            auto item = ui->listWidget->item(i);
            if (item->data(FilePathRole).toString() == progress.file)
            {
                ui->listWidget->setCurrentRow(i);
                break;
            }
        }
    }
}

void PictureBrowser::setSlideInterval(int ms)
{
    settings.setValue("picturebrowser/slideInterval", ms);
    slideTimer->setInterval(ms);
}

void PictureBrowser::fastSortItems(QString key)
{
    QDir root(rootDirPath);
    QFileInfo sInfo(root.absoluteFilePath(CLASSIFICATION_FILE));
    if (!sInfo.exists())
    {
        root.mkpath(CLASSIFICATION_FILE);
        QString tip = "在根目录下“classification/”文件夹中创建形如“-S名称”的文件夹，";
        tip += "则可以使用“S”键一键移动选中项到对应位置。";
        tip += "\n支持A-Z的快捷键(只要不冲突)，请勿重复（不区分大小写）";
        QMessageBox::information(this, "快速分类", tip);
        return ;
    }

    QDir classification(sInfo.absoluteFilePath());
    QList<QFileInfo> dirs = classification.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfo target;
    bool find = false;
    for (int i = 0; i < dirs.size(); i++)
    {
        QString name = dirs.at(i).fileName();
        if (name.startsWith("-"+key.toLower()) || name.startsWith("-"+key.toUpper()))
        {
            target = dirs.at(i);
            find = true;
            break;
        }
    }
    if (!find)
    {
        qDebug() << "快速分类：没有找到“-" + key + "”的目录" << sInfo.absoluteFilePath();
        return ;
    }
    QDir clsfct(target.absoluteFilePath());

    // 开始移动
    auto items = ui->listWidget->selectedItems();
    if (!items.size())
        return ;
    ui->previewPicture->unbindMovie();
    int firstRow = ui->listWidget->row(items.first());
    foreach(auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;
        QFileInfo info(path);
        if (!info.exists())
            continue;
        if (info.isFile() || info.isDir())
        {
            QFile file(path);
            QString newPath = clsfct.absoluteFilePath(info.fileName());
            if (!file.rename(newPath))
                qDebug() << "重命名失败：" << path << newPath;
        }

        int row = ui->listWidget->row(item);
        ui->listWidget->takeItem(row);
    }
    if (firstRow >= ui->listWidget->count())
        firstRow = ui->listWidget->count()-1;
    ui->listWidget->setCurrentRow(firstRow);

}

void PictureBrowser::on_actionRefresh_triggered()
{
    saveCurrentViewPos();
    enterDirectory(currentDirPath);
}

void PictureBrowser::on_listWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
    {
        ui->previewPicture->setPixmap(QPixmap());
        sizeLabel->setText("");
        return ;
    }

    QString path = current->data(FilePathRole).toString();
    QFileInfo info(path);
    if (info.isFile())
    {
        QString suffix = info.suffix();
        // 显示图片预览
        if (info.suffix() == "gif")
        {
            ui->previewPicture->setGif(path);
        }
        else if (getImageFilters().contains("*." + suffix))
        {
            QPixmap pixmap(info.absoluteFilePath());
            if (!ui->previewPicture->setPixmap(pixmap))
                PBDEB << "打开图片失败：" << info.absoluteFilePath() << QPixmap(info.absoluteFilePath()).isNull();
        }
        else
        {
            PBDEB << "无法识别文件：" << info.absoluteFilePath();
        }
    }
    else if (info.isDir())
    {
        QList<QFileInfo> infos = QDir(info.absoluteFilePath()).
                entryInfoList(getImageFilters(),
                              QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                              sortFlags);
        if (infos.size())
        {
            ui->previewPicture->setPixmap(QPixmap(infos.first().absoluteFilePath()));
        }
    }

    QSize size = ui->previewPicture->getOriginPixmap().size();
    sizeLabel->setText(QString("%1×%2").arg(size.width()).arg(size.height()));
}

void PictureBrowser::on_listWidget_itemActivated(QListWidgetItem *item)
{
    if (!item)
    {
        ui->previewPicture->setPixmap(QPixmap());
        return ;
    }

    // 返回上一级
    if (item->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
    {
        on_actionBack_Prev_Directory_triggered();
        return ;
    }

    // 使用系统程序打开图片
    QString path = item->data(FilePathRole).toString();
    QFileInfo info(path);
    if (info.isFile())
    {
#ifdef Q_OS_WIN32
    QString m_szHelpDoc = QString("file:///") + path;
    bool is_open = QDesktopServices::openUrl(QUrl(m_szHelpDoc, QUrl::TolerantMode));
    if(!is_open)
    {
        PBDEB << "打开图片失败：" << path;
    }
#else
    QString  cmd = QString("xdg-open ")+ m_szHelpDoc;　　　　　　　　//在linux下，可以通过system来xdg-open命令调用默认程序打开文件；
    system(cmd.toStdString().c_str());
#endif
    }
    else if (info.isDir())
    {
        saveCurrentViewPos();

        // 进入文件夹
        currentDirPath = info.absoluteFilePath();
        enterDirectory(currentDirPath);
    }
}

void PictureBrowser::on_actionIcon_Small_triggered()
{
    setListWidgetIconSize(64);
    ui->actionIcon_Small->setChecked(true);
}

void PictureBrowser::on_actionIcon_Middle_triggered()
{
    setListWidgetIconSize(128);
    ui->actionIcon_Middle->setChecked(true);
}

void PictureBrowser::on_actionIcon_Large_triggered()
{
    setListWidgetIconSize(256);
    ui->actionIcon_Large->setChecked(true);
}

void PictureBrowser::on_actionIcon_Largest_triggered()
{
    setListWidgetIconSize(512);
    ui->actionIcon_Largest->setChecked(true);
}

void PictureBrowser::on_listWidget_customContextMenuRequested(const QPoint &pos)
{
    QMenu* menu = new QMenu(this);
    menu->addAction(ui->actionOpen_Select_In_Explore);
    menu->addAction(ui->actionCopy_File);
    menu->addSeparator();
    menu->addAction(ui->actionMark_Red);
    menu->addAction(ui->actionMark_Green);
    menu->addAction(ui->actionMark_None);
    menu->addSeparator();
    menu->addAction(ui->actionExtra_And_Copy);
    menu->addAction(ui->actionExtra_Selected);
    menu->addAction(ui->actionExtra_And_Delete);
    menu->addAction(ui->actionDelete_Selected);
    menu->addAction(ui->actionDelete_Up_Files);
    menu->addAction(ui->actionDelete_Down_Files);
    menu->addSeparator();
    menu->addAction(ui->actionStart_Play_GIF);
    menu->exec(QCursor::pos());
}

void PictureBrowser::on_actionIcon_Lager_triggered()
{
    int size = ui->listWidget->iconSize().width();
    size = size * 5 / 4;
    setListWidgetIconSize(size);
}

void PictureBrowser::on_actionIcon_Smaller_triggered()
{
    int size = ui->listWidget->iconSize().width();
    size = size * 4 / 5;
    setListWidgetIconSize(size);
}

void PictureBrowser::on_actionZoom_In_triggered()
{
    ui->previewPicture->scaleTo(1.25, ui->previewPicture->geometry().center());
}

void PictureBrowser::on_actionZoom_Out_triggered()
{
    ui->previewPicture->scaleTo(0.8, ui->previewPicture->geometry().center());
}

void PictureBrowser::on_actionRestore_Size_triggered()
{
    // 还原预览图默认大小
    ui->previewPicture->resetScale();
}

void PictureBrowser::on_actionFill_Size_triggered()
{
    // 预览图填充屏幕
    ui->previewPicture->scaleToFill();
}

void PictureBrowser::on_actionOrigin_Size_triggered()
{
    // 原始大小
    ui->previewPicture->scaleToOrigin();
}

void PictureBrowser::on_actionDelete_Selected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    if (!items.size())
        return ;
    ui->previewPicture->unbindMovie();
    int firstRow = ui->listWidget->row(items.first());
    foreach (auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;

        deleteFileOrDir(path);

        int row = ui->listWidget->row(item);
        ui->listWidget->takeItem(row);
    }
    commitDeleteCommand();
    if (firstRow >= ui->listWidget->count())
        firstRow = ui->listWidget->count() - 1;
    if (firstRow > -1)
        ui->listWidget->setCurrentRow(firstRow);
}

void PictureBrowser::on_actionExtra_Selected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    if (!items.size())
        return ;
    ui->previewPicture->unbindMovie();
    int firstRow = ui->listWidget->row(items.first());
    foreach(auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;
        QFileInfo info(path);
        if (!info.exists())
            continue;
        if (info.isFile() || info.isDir())
        {
            QFile file(path);
            QDir dir(info.absoluteDir());
            dir.cdUp();
            file.rename(dir.absoluteFilePath(info.fileName()));
        }

        int row = ui->listWidget->row(item);
        ui->listWidget->takeItem(row);
    }
    if (firstRow >= ui->listWidget->count())
        firstRow = ui->listWidget->count()-1;
    ui->listWidget->setCurrentRow(firstRow);
}

void PictureBrowser::on_actionExtra_And_Copy_triggered()
{
    auto items = ui->listWidget->selectedItems();
    if (!items.size())
        return ;

    foreach(auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;
        QFileInfo info(path);
        if (!info.exists())
            continue;
        if (info.isFile())
        {
            QFile file(path);
            QDir upDir(info.absoluteDir());
            upDir.cdUp();
            file.copy(upDir.absoluteFilePath(info.fileName()));
        }
        else if (info.isDir())
        {
            QDir dir(path);
            QDir upDir(info.absoluteDir());
            upDir.cdUp();
            copyDirectoryFiles(info.absoluteFilePath(), upDir.absoluteFilePath(info.fileName()), true);
        }
    }
}

void PictureBrowser::on_actionDelete_Unselected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    if (!items.size())
        return ;
    for (int i = 0; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (items.contains(item))
            continue;

        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;

        deleteFileOrDir(path);

        ui->listWidget->takeItem(i--);
    }
    commitDeleteCommand();
}

void PictureBrowser::on_actionExtra_And_Delete_triggered()
{
    auto items = ui->listWidget->selectedItems();
    // 如果没有选中就是全部删除
    QStringList paths;

    // 提取文件
    foreach (auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            return ;
        if (path.isEmpty() || !QFileInfo(path).exists())
            return ;

        paths.append(path);

        // 提取文件到外面
        QFileInfo info(path);
        if (!info.exists())
            continue;
        if (info.isFile() || info.isDir())
        {
            QFile file(path);
            QDir dir(info.absoluteDir());
            dir.cdUp();
            file.rename(dir.absoluteFilePath(info.fileName()));
        }
    }

    // 删除剩下的整个文件夹
    QDir dir(currentDirPath);
    QDir up(currentDirPath);
    up.cdUp();
    deleteFileOrDir(up.absoluteFilePath(dir.dirName()));
    commitDeleteCommand();
    enterDirectory(up.absolutePath());

    // 选中刚提取的
    ui->listWidget->selectionModel()->clear();
    for (int i = 0; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (paths.contains(item->data(FilePathRole).toString()))
        {
            ui->listWidget->setCurrentItem(item, QItemSelectionModel::Select);
        }
    }
}

void PictureBrowser::on_actionOpen_Select_In_Explore_triggered()
{
    auto item = ui->listWidget->currentItem();
    QString path = item->data(FilePathRole).toString();
    if (path == BACK_PREV_DIRECTORY)
        return ;
    if (path.isEmpty() || !QFileInfo(path).exists())
        return ;

    QProcess process;
    path.replace("/", "\\");
    QString cmd = QString("explorer.exe /select, \"%1\"").arg(path);
    process.startDetached(cmd);
}


void PictureBrowser::on_actionOpen_In_Explore_triggered()
{
    QDesktopServices::openUrl(QUrl("file:///" + currentDirPath, QUrl::TolerantMode));
}

void PictureBrowser::on_actionBack_Prev_Directory_triggered()
{
    if (currentDirPath == rootDirPath)
        return ;
    saveCurrentViewPos();

    QDir dir(currentDirPath);
    dir.cdUp();
    enterDirectory(dir.absolutePath());
    commitDeleteCommand();
}

void PictureBrowser::on_actionCopy_File_triggered()
{
    auto items = ui->listWidget->selectedItems();
    QList<QUrl> urls;
    foreach (auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;
        urls.append(QUrl("file:///" + path));
    }

    QMimeData* mime = new QMimeData();
    mime->setUrls(urls);
    QApplication::clipboard()->setMimeData(mime);
}

void PictureBrowser::on_actionCut_File_triggered()
{
    // 不支持剪切文件
}

void PictureBrowser::on_actionDelete_Up_Files_triggered()
{
    auto item = ui->listWidget->currentItem();
    int row = ui->listWidget->row(item);
    int start = 0;
    while (row--)
    {
        auto item = ui->listWidget->item(start);
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
        {
            start++;
            continue;
        }

        deleteFileOrDir(path);

        ui->listWidget->takeItem(start);
    }
    commitDeleteCommand();
    ui->listWidget->scrollToTop();
}

void PictureBrowser::on_actionDelete_Down_Files_triggered()
{
    auto item = ui->listWidget->currentItem();
    int row = ui->listWidget->row(item) + 1;
    while (row < ui->listWidget->count())
    {
        auto item = ui->listWidget->item(row);
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;

        deleteFileOrDir(path);

        ui->listWidget->takeItem(row);
    }
    commitDeleteCommand();
    ui->listWidget->scrollToBottom();
}

void PictureBrowser::on_listWidget_itemSelectionChanged()
{
    int count = ui->listWidget->selectedItems().size();

    ui->actionOpen_Select_In_Explore->setEnabled(count);
    ui->actionDelete_Up_Files->setEnabled(count == 1 && currentDirPath != rootDirPath);
    ui->actionDelete_Down_Files->setEnabled(count == 1 && currentDirPath != rootDirPath);

    ui->actionCopy_File->setEnabled(count);
    ui->actionCut_File->setEnabled(count);
    ui->actionExtra_Selected->setEnabled(count);
    ui->actionDelete_Selected->setEnabled(count);
    ui->actionDelete_Unselected->setEnabled(count);

    ui->actionMark_Red->setEnabled(count);
    ui->actionMark_Green->setEnabled(count);
    ui->actionMark_None->setEnabled(count);

    ui->actionGeneral_GIF->setEnabled(count >= 2);

    selectLabel->setText(count >= 2 ? QString("选中%1项").arg(count) : "");
}

void PictureBrowser::on_actionSort_By_Time_triggered()
{
    settings.setValue("picturebrowser/sortType", QDir::Time);
    readSortFlags();
    on_actionRefresh_triggered();
}

void PictureBrowser::on_actionSort_By_Name_triggered()
{
    settings.setValue("picturebrowser/sortType", QDir::Name);
    readSortFlags();
    on_actionRefresh_triggered();
}

void PictureBrowser::on_actionSort_AESC_triggered()
{
    settings.setValue("picturebrowser/sortReversed", false);
    readSortFlags();
    on_actionRefresh_triggered();
}

void PictureBrowser::on_actionSort_DESC_triggered()
{
    settings.setValue("picturebrowser/sortReversed", true);
    readSortFlags();
    on_actionRefresh_triggered();
}

void PictureBrowser::on_listWidget_itemPressed(QListWidgetItem *item)
{
    // 停止播放
    if (slideTimer->isActive())
        slideTimer->stop();
}

void PictureBrowser::on_actionStart_Play_GIF_triggered()
{
    if (!slideTimer->isActive())
    {
        // 调整选中项
        removeUselessItemSelect();
        auto selectedItems = ui->listWidget->selectedItems();

        if (selectedItems.size() > 1 && selectedItems.contains(ui->listWidget->currentItem()))
        {
            slideInSelected = true;

            // 排序
            std::sort(selectedItems.begin(), selectedItems.end(), [=](QListWidgetItem*a, QListWidgetItem* b){
                return ui->listWidget->row(a) < ui->listWidget->row(b);
            });
            ui->listWidget->clearSelection();
            for (int i = 0; i < selectedItems.size(); i++)
            {
                ui->listWidget->setCurrentItem(selectedItems.at(i), QItemSelectionModel::Select);
            }
            ui->listWidget->setCurrentItem(selectedItems.first(), QItemSelectionModel::Current);
        }
        else
            slideInSelected = false;

        slideTimer->start();
    }
    else
        slideTimer->stop();
}

void PictureBrowser::on_actionSlide_100ms_triggered()
{
    setSlideInterval(100);
    ui->actionSlide_100ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_200ms_triggered()
{
    setSlideInterval(200);
    ui->actionSlide_200ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_500ms_triggered()
{
    setSlideInterval(500);
    ui->actionSlide_500ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_1000ms_triggered()
{
    setSlideInterval(1000);
    ui->actionSlide_1000ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_3000ms_triggered()
{
    setSlideInterval(3000);
    ui->actionSlide_3000ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_Return_First_triggered()
{
    bool first = !settings.value("picturebrowser/slideReturnFirst", false).toBool();
    settings.setValue("picturebrowser/slideReturnFirst", first);

    ui->actionSlide_Return_First->setChecked(first);
}

void PictureBrowser::on_actionSlide_16ms_triggered()
{
    setSlideInterval(16);
    ui->actionSlide_16ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_33ms_triggered()
{
    setSlideInterval(33);
    ui->actionSlide_33ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_50ms_triggered()
{
    setSlideInterval(50);
    ui->actionSlide_50ms->setChecked(true);
}

void PictureBrowser::on_actionMark_Red_triggered()
{
    auto items = ui->listWidget->selectedItems();
    foreach (auto item, items)
    {
        item->setBackground(redMark);
    }
}

void PictureBrowser::on_actionMark_Green_triggered()
{
    auto items = ui->listWidget->selectedItems();
    foreach (auto item, items)
    {
        item->setBackground(greenMark);
    }
}

void PictureBrowser::on_actionMark_None_triggered()
{
    auto items = ui->listWidget->selectedItems();
    foreach (auto item, items)
    {
        item->setBackground(Qt::transparent);
    }
}

void PictureBrowser::on_actionSelect_Green_Marked_triggered()
{
    for (int i = 0; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (item->background() == greenMark)
        {
            ui->listWidget->setCurrentItem(item, QItemSelectionModel::Select);
        }
    }
}

void PictureBrowser::on_actionSelect_Red_Marked_triggered()
{
    for (int i = 0; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (item->background() == redMark)
        {
            ui->listWidget->setCurrentItem(item, QItemSelectionModel::Select);
        }
    }
}

void PictureBrowser::on_actionPlace_Red_Top_triggered()
{
    int index = 0;
    if (ui->listWidget->item(0)->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
        index++;
    for (int i = index+1; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (item->background() == redMark)
        {
            ui->listWidget->takeItem(i);
            ui->listWidget->insertItem(index++, item);
        }
    }
    ui->listWidget->scrollToTop();
}

void PictureBrowser::on_actionPlace_Green_Top_triggered()
{
    int index = 0;
    if (ui->listWidget->item(0)->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
        index++;
    for (int i = index+1; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (item->background() == greenMark)
        {
            ui->listWidget->takeItem(i);
            ui->listWidget->insertItem(index++, item);
        }
    }
    ui->listWidget->scrollToTop();
}

void PictureBrowser::on_actionOpen_Directory_triggered()
{
    QString path = QFileDialog::getExistingDirectory(this, "选择要打开的文件夹", rootDirPath);
    if (!path.isEmpty())
        readDirectory(path);
}

void PictureBrowser::on_actionSelect_Reverse_triggered()
{
    if (ui->listWidget->count() == 0)
        return ;
    auto selectedItems = ui->listWidget->selectedItems();
    int start = 0;
    if (ui->listWidget->item(0)->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
        start++;
    ui->listWidget->setCurrentRow(0, QItemSelectionModel::Clear);
    for (int i = start; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (!selectedItems.contains(item))
            ui->listWidget->setCurrentRow(i, QItemSelectionModel::Select);
    }
}

void PictureBrowser::deleteFileOrDir(QString path)
{
    QFileInfo info(path);
    QString newPath = recycleDir.absoluteFilePath(info.fileName());
    deleteCommandsQueue.append(QPair<QString, QString>(path, newPath));
    if (info.isFile())
    {
        if (QFileInfo(newPath).exists())
            QFile(newPath).remove();

        // QFile::remove(path);
        if (!QFile(path).rename(newPath))
            PBDEB << "删除文件失败：" << path;
    }
    else if (info.isDir())
    {
        if (QFileInfo(newPath).exists())
            QDir(newPath).removeRecursively();

        QDir dir(path);
        // dir.removeRecursively();
        if (!dir.rename(path, newPath))
            PBDEB << "删除文件夹失败：" << path;
    }
}

void PictureBrowser::commitDeleteCommand()
{
    deleteUndoCommands.append(deleteCommandsQueue);
    deleteCommandsQueue.clear(); // 等待下一次的commit
    ui->actionUndo_Delete_Command->setEnabled(true);
}

void PictureBrowser::removeUselessItemSelect()
{
    auto selectedItems = ui->listWidget->selectedItems();
    if (selectedItems.size() >= 1)
    {
        auto backItem = ui->listWidget->item(0);
        int  currentRow = ui->listWidget->currentRow();
        if (backItem->data(FilePathRole) == BACK_PREV_DIRECTORY && selectedItems.contains(backItem))
        {
            ui->listWidget->setCurrentRow(0, QItemSelectionModel::Deselect);
            selectedItems.removeOne(backItem);

            if (currentRow != 0 || !selectedItems.size())
                ui->listWidget->setCurrentRow(currentRow, QItemSelectionModel::Current);
            else
                ui->listWidget->setCurrentItem(selectedItems.first(), QItemSelectionModel::Current);
            PBDEB << "自动移除【返回上一级】项";
        }
    }
}

QStringList PictureBrowser::getImageFilters()
{
    return QStringList{"*.jpg", "*.png", "*.jpeg", "*.gif"};
}

bool PictureBrowser::copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);
    if(!targetDir.exists()){    /**< 如果目标目录不存在，则进行创建 */
        if(!targetDir.mkdir(targetDir.absolutePath()))
            return false;
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach(QFileInfo fileInfo, fileInfoList){
        if(fileInfo.fileName() == "." || fileInfo.fileName() == "..")
            continue;

        if(fileInfo.isDir()){    /**< 当为目录时，递归的进行copy */
            if(!copyDirectoryFiles(fileInfo.filePath(),
                                   targetDir.filePath(fileInfo.fileName()),
                                   coverFileIfExist))
                return false;
        }
        else{            /**< 当允许覆盖操作时，将旧文件进行删除操作 */
            if(coverFileIfExist && targetDir.exists(fileInfo.fileName())){
                targetDir.remove(fileInfo.fileName());
            }

            /// 进行文件copy
            if(!QFile::copy(fileInfo.filePath(),
                            targetDir.filePath(fileInfo.fileName()))){
                return false;
            }
        }
    }
    return true;
}

/**
 * 获取录制时间
 * 如果无法读取，则使用播放时间（用户决定）
 */
int PictureBrowser::getRecordInterval()
{
    bool gifUseRecordInterval = settings.value("gif/recordInterval", true).toBool();
    int interval = 0;
    if (gifUseRecordInterval)
    {
        // 从当前文件夹的配置文件中读取时间
        QFileInfo info(QDir(currentDirPath).absoluteFilePath(SEQUENCE_PARAM_FILE));
        if (info.exists())
        {
            QSettings st(info.absoluteFilePath(), QSettings::IniFormat);
            interval = st.value("gif/interval", slideTimer->interval()).toInt();
            PBDEB << "读取录制时间：" << interval;
        }
    }
    if (interval <= 0)
    {
        // 使用默认的播放时间
        interval = slideTimer->interval();
    }
    return interval;
}

void PictureBrowser::saveImageConversionFlag()
{
    imageConversion = Qt::AutoColor;

    if (ui->actionAutoColor->isChecked())
        imageConversion |= Qt::AutoColor;
    else if (ui->actionColorOnly->isChecked())
        imageConversion |= Qt::ColorOnly;
    else if (ui->actionMonoOnly->isChecked())
        imageConversion |= Qt::MonoOnly;

    if (ui->actionDiffuseDither->isChecked())
        imageConversion |= Qt::DiffuseDither;
    else if (ui->actionOrderedDither->isChecked())
        imageConversion |= Qt::OrderedDither;
    else if (ui->actionThresholdDither->isChecked())
        imageConversion |= Qt::ThresholdDither;

    if (ui->actionThresholdAlphaDither->isChecked())
        imageConversion |= Qt::ThresholdAlphaDither;
    else if (ui->actionOrderedAlphaDither->isChecked())
        imageConversion |= Qt::OrderedAlphaDither;
    else if (ui->actionDiffuseAlphaDither->isChecked())
        imageConversion |= Qt::ThresholdAlphaDither;

    if (ui->actionPreferDither->isChecked())
        imageConversion |= Qt::PreferDither;
    else if (ui->actionAvoidDither->isChecked())
        imageConversion |= Qt::AvoidDither;
    else if (ui->actionAutoDither->isChecked())
        imageConversion |= Qt::AutoDither;
    else if (ui->actionNoOpaqueDetection->isChecked())
        imageConversion |= Qt::NoOpaqueDetection;
    else if (ui->actionNoFormatConversion->isChecked())
        imageConversion |= Qt::NoFormatConversion;

    settings.setValue("gif/imageConversion", (int)imageConversion);
}

void PictureBrowser::fromImageConversionFlag()
{
    if (imageConversion & Qt::ColorOnly)
        ui->actionColorOnly->setChecked(true);
    else if (imageConversion & Qt::MonoOnly)
        ui->actionMonoOnly->setChecked(true);
    else
        ui->actionAutoColor->setChecked(true);

    if (imageConversion & Qt::OrderedDither)
        ui->actionOrderedDither->setChecked(true);
    else if (imageConversion & Qt::ThresholdDither)
        ui->actionThresholdDither->setChecked(true);
    else
        ui->actionDiffuseDither->setChecked(true);

    if (imageConversion & Qt::OrderedAlphaDither)
        ui->actionOrderedAlphaDither->setChecked(true);
    else if (imageConversion & Qt::ThresholdAlphaDither)
        ui->actionDiffuseAlphaDither->setChecked(true);
    else
        ui->actionThresholdAlphaDither->setChecked(true);

    if (imageConversion & Qt::PreferDither)
        ui->actionPreferDither->setChecked(true);
    else if (imageConversion & Qt::AvoidDither)
        ui->actionAvoidDither->setChecked(true);
    else if (imageConversion & Qt::NoOpaqueDetection)
        ui->actionNoOpaqueDetection->setChecked(true);
    else if (imageConversion & Qt::NoFormatConversion)
        ui->actionNoFormatConversion->setChecked(true);
    else
        ui->actionAutoDither->setChecked(true);
}

void PictureBrowser::on_actionUndo_Delete_Command_triggered()
{
    if (deleteUndoCommands.size() == 0)
        return ;

    QList<QPair<QString, QString>> deleteCommand = deleteUndoCommands.last();
    QStringList existOriginPaths;
    QStringList existRecyclePaths;
    for (int i = deleteCommand.size()-1; i >= 0; i--)
    {
        QString oldPath = deleteCommand.at(i).first;
        QString newPath = deleteCommand.at(i).second;

        // 回收站文件不存在（手动删除）
        if (!QFileInfo(newPath).exists())
            continue;

        // 文件已存在
        if (QFileInfo(oldPath).exists())
        {
            existOriginPaths.append(oldPath);
            existRecyclePaths.append(newPath);
            continue;
        }

        // 重命名
        QFileInfo info(newPath);
        if (info.isDir())
        {
            QDir(newPath).rename(newPath, oldPath);
        }
        else if (info.isFile())
        {
            QFile(newPath).rename(newPath, oldPath);
        }
    }

    deleteUndoCommands.removeLast();

    if (existOriginPaths.size())
    {
        QString pathText = existOriginPaths.size() > 5
                ? (existOriginPaths.first() + "  等" + QString::number(existOriginPaths.size()) + "个文件")
                : existOriginPaths.join("\n");
        if (QMessageBox::question(this, "文件已存在", "下列原文件存在，是否覆盖？\n\n" + pathText,
                                  QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes) == QMessageBox::Yes)
        {
            for (int i = existOriginPaths.size() - 1; i >= 0; i--)
            {
                QString oldPath = existOriginPaths.at(i);
                QString newPath = existRecyclePaths.at(i);

                // 文件已存在
                QFileInfo oldInfo(oldPath);
                if (oldInfo.exists())
                {
                    if (oldInfo.isDir())
                        QDir(oldPath).removeRecursively();
                    else if (oldInfo.isFile())
                        QFile(oldPath).remove();
                }

                // 重命名
                QFileInfo info(newPath);
                if (info.isDir())
                {
                    QDir(newPath).rename(newPath, oldPath);
                }
                else if (info.isFile())
                {
                    QFile(newPath).rename(newPath, oldPath);
                }
            }
        }
    }

    on_actionRefresh_triggered();
    ui->actionUndo_Delete_Command->setEnabled(deleteUndoCommands.size());
}

/**
 * 源代码参考自：https://github.com/douzhongqiang/EasyGifTool
 */
void PictureBrowser::on_actionGeneral_GIF_triggered()
{
    removeUselessItemSelect();
    auto selectedItems = ui->listWidget->selectedItems();

    if (selectedItems.size() < 2)
    {
        QMessageBox::warning(this, "生成GIF", "请选中至少2张图片");
        return ;
    }

    // 进行排序啊
    std::sort(selectedItems.begin(), selectedItems.end(), [=](QListWidgetItem*a, QListWidgetItem* b){
        return ui->listWidget->row(a) < ui->listWidget->row(b);
    });

    // 获取间隔
    int interval = getRecordInterval();

    // 所有图片路径
    QStringList pixmapPaths;
    for (int i = 0; i < selectedItems.size(); i++)
        pixmapPaths.append(selectedItems.at(i)->data(FilePathRole).toString());

    // 获取图片大小
    auto item = selectedItems.first();
    QPixmap firstPixmap(item->data(FilePathRole).toString());
    QSize size = firstPixmap.size(); // 图片大小
    QString fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss.zzz")+".gif";
    QString dirPath = ui->actionCreate_To_Origin_Folder->isChecked()
            ? currentDirPath : QDir(rootDirPath).absoluteFilePath(GENERAL_DIRECTORY);
    QString gifPath = QDir(dirPath)
            .absoluteFilePath(fileName);

    // 压缩程度
    int compress = settings.value("gif/compress", 0).toInt();
    int prop = 1;
    for (int i = 0; i < compress; i++)
        prop *= 2;
    size_t wt = static_cast<uint32_t>(size.width() / prop);
    size_t ht = static_cast<uint32_t>(size.height() / prop);
    size_t iv = static_cast<uint32_t>(interval / 8); // GIF合成的工具有问题，只能自己微调时间了

    // 创建GIF
    progressBar->setMaximum(pixmapPaths.size());
    progressBar->show();
    QtConcurrent::run([=]{
        Gif_H m_Gif;
        Gif_H::GifWriter* m_GifWriter = new Gif_H::GifWriter;
        if (!m_Gif.GifBegin(m_GifWriter, gifPath.toLocal8Bit().data(), wt, ht, iv))
        {
            PBDEB << "开启gif失败";
            delete m_GifWriter;
            return;
        }

        QDir(dirPath).mkpath(dirPath);
        for (int i = 0; i < pixmapPaths.size(); i++)
        {
            QPixmap pixmap(pixmapPaths.at(i));
            if (!pixmap.isNull())
            {
                if (prop > 1)
                    pixmap = pixmap.scaled(static_cast<int>(wt), static_cast<int>(ht));
                m_Gif.GifWriteFrame(m_GifWriter, pixmap.toImage().convertToFormat(QImage::Format_RGBA8888, imageConversion).bits(), wt, ht, iv, 8, gifDither);
            }
            emit signalGeneralGIFProgress(i+1);
        }

        m_Gif.GifEnd(m_GifWriter);
        delete m_GifWriter;

        emit signalGeneralGIFFinished(gifPath);
        PBDEB << "GIF生成完毕：" << size << pixmapPaths.size() << interval << compress;
    });
}

void PictureBrowser::on_actionGIF_Use_Record_Interval_triggered()
{
    settings.setValue("gif/recordInterval", true);
    ui->actionGIF_Use_Record_Interval->setChecked(true);
}

void PictureBrowser::on_actionGIF_Use_Display_Interval_triggered()
{
    settings.setValue("gif/recordInterval", false);
    ui->actionGIF_Use_Display_Interval->setChecked(true);
}

void PictureBrowser::on_actionGIF_Compress_None_triggered()
{
    settings.setValue("gif/compress", 0);
    ui->actionGIF_Compress_None->setChecked(true);
}

void PictureBrowser::on_actionGIF_Compress_x2_triggered()
{
    settings.setValue("gif/compress", 1);
    ui->actionGIF_Compress_x2->setChecked(true);
}

void PictureBrowser::on_actionGIF_Compress_x4_triggered()
{
    settings.setValue("gif/compress", 2);
    ui->actionGIF_Compress_x4->setChecked(true);
}

void PictureBrowser::on_actionGIF_Compress_x8_triggered()
{
    settings.setValue("gif/compress", 3);
    ui->actionGIF_Compress_x8->setChecked(true);
}

void PictureBrowser::on_actionUnpack_GIF_File_triggered()
{
    // 选择gif
    QString path = QFileDialog::getOpenFileName(this, "请选择GIF路径，拆分成为一帧一帧的图片", rootDirPath, "Images (*.gif)");
    if (path.isEmpty())
        return ;

    // 设置提取路径
    QFileInfo info(path);
    QString name = info.baseName();
    QString dir = QDir(currentDirPath).absoluteFilePath(name);
    if (QFileInfo(dir).exists())
    {
        int index = 0;
        while (QFileInfo(dir+"("+QString::number(++index)+")").exists());
        dir += "("+QString::number(++index)+")";
    }

    // 开始提取
    QDir saveDir(dir);
    saveDir.mkpath(saveDir.absolutePath());
    QMovie movie(path);
    movie.setCacheMode(QMovie::CacheAll);
    for (int i = 0; i < movie.frameCount(); ++i)
    {
        movie.jumpToFrame(i);
        QImage image = movie.currentImage();
        QFile file(saveDir.absoluteFilePath(QString("%1.jpg").arg(i)));
        file.open(QFile::WriteOnly);
        image.save(&file, "JPG");
        file.close();
    }

    // 打开文件夹
    auto result = QMessageBox::information(this, "分解GIF完毕", "路径：" + saveDir.absolutePath(), "进入文件夹", "显示在资源管理器", "取消", 0, 2);
    if (result == 0) // 进入文件夹
    {
        enterDirectory(saveDir.absolutePath());
    }
    else if (result == 1) // 显示在资源管理器
    {
        QDesktopServices::openUrl(QUrl("file:///" + saveDir.absolutePath(), QUrl::TolerantMode));
    }
}

/**
 * 字符画链接：https://zhuanlan.zhihu.com/p/65232824
 */
void PictureBrowser::on_actionGIF_ASCII_Art_triggered()
{
    // 选择gif
    QString path = QFileDialog::getOpenFileName(this, "请选择图片路径，制作字符画", rootDirPath, "Images (*.gif *.jpg *.png *.jpeg)");
    if (path.isEmpty())
        return ;

    // 保存路径
    QFileInfo info(path);
    QString suffix = info.suffix();
    QString savePath = QDir(info.absoluteDir()).absoluteFilePath(info.baseName() + "_ART");
    if (QFileInfo(savePath + "." + suffix).exists())
    {
        int index = 0;
        while (QFileInfo(savePath+"("+QString::number(++index)+")." + suffix).exists());
        savePath += "(" + QString::number(index) + ")";
    }
    savePath += "." + suffix;

    // 普通图片，马上弄
    if (!path.endsWith(".gif"))
    {
        ASCIIArt art;
        QPixmap pixmap(path);
        QPixmap cache(art.setImage(pixmap.toImage(),
                                   pixmap.hasAlpha() ? Qt::transparent : Qt::white));
        cache.save(savePath);
        emit signalGeneralGIFFinished(savePath);
        return ;
    }

    QtConcurrent::run([=]{
        // 初始化
        QMovie movie(path);
        ASCIIArt art;
        movie.jumpToFrame(0);

        // 生成cache
        QList<QPixmap> cachePixmaps;
        QList<QPixmap> srcPixmaps;
        int count = movie.frameCount();
        for (int i = 0; i < count; i++)
        {
            srcPixmaps.insert(i, movie.currentPixmap());
            cachePixmaps.insert(i, art.setImage(movie.currentImage(), Qt::white));
            movie.jumpToNextFrame();
        }

        QSize size = cachePixmaps.first().size();
        size_t wt = static_cast<uint32_t>(size.width());
        size_t ht = static_cast<uint32_t>(size.height());
        size_t iv = static_cast<uint32_t>(movie.speed()) / 8;

        Gif_H m_Gif;
        Gif_H::GifWriter* m_GifWriter = new Gif_H::GifWriter;
        if (!m_Gif.GifBegin(m_GifWriter, savePath.toLocal8Bit().data(), wt, ht, iv))
        {
            PBDEB << "开启gif失败";
            delete m_GifWriter;
            return;
        }

        for (int i = 0; i < cachePixmaps.size(); i++)
        {
            m_Gif.GifWriteFrame(m_GifWriter,
                                cachePixmaps.at(i).toImage()
                                    .convertToFormat(QImage::Format_RGBA8888).bits(),
                                wt, ht, iv);
        }

        m_Gif.GifEnd(m_GifWriter);
        delete m_GifWriter;

        emit signalGeneralGIFFinished(savePath);
        PBDEB << "GIF生成完毕：" << size << cachePixmaps.first().size() << iv;
    });
}

void PictureBrowser::on_actionDither_Enabled_triggered()
{
    settings.setValue("gif/dither", true);
}

void PictureBrowser::on_actionDither_Disabled_triggered()
{
    settings.setValue("gif/dither", false);
}

void PictureBrowser::on_actionGeneral_AVI_triggered()
{
    removeUselessItemSelect();
    auto selectedItems = ui->listWidget->selectedItems();

    if (selectedItems.size() < 2)
    {
        QMessageBox::warning(this, "生成GIF", "请选中至少2张图片");
        return ;
    }

    // 进行排序啊
    std::sort(selectedItems.begin(), selectedItems.end(), [=](QListWidgetItem*a, QListWidgetItem* b){
        return ui->listWidget->row(a) < ui->listWidget->row(b);
    });

    // 获取间隔
    int interval = getRecordInterval();

    // 所有图片路径
    QStringList pixmapPaths;
    for (int i = 0; i < selectedItems.size(); i++)
        pixmapPaths.append(selectedItems.at(i)->data(FilePathRole).toString());

    // 获取图片大小
    auto item = selectedItems.first();
    QPixmap firstPixmap(item->data(FilePathRole).toString());
    QSize size = firstPixmap.size(); // 图片大小
    QString fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss.zzz")+".avi";
    QString dirPath = ui->actionCreate_To_Origin_Folder->isChecked()
            ? currentDirPath : QDir(rootDirPath).absoluteFilePath(GENERAL_DIRECTORY);
    QString gifPath = QDir(dirPath)
            .absoluteFilePath(fileName);

    // 压缩程度
    int compress = settings.value("gif/compress", 0).toInt();
    int prop = 1;
    for (int i = 0; i < compress; i++)
        prop *= 2;
    size_t wt = static_cast<uint32_t>(size.width() / prop);
    size_t ht = static_cast<uint32_t>(size.height() / prop);
    size_t iv = static_cast<uint32_t>(interval);

    // 创建GIF
    progressBar->setMaximum(pixmapPaths.size());
    progressBar->show();
    QtConcurrent::run([=]{
        QDir(dirPath).mkpath(dirPath);
        avi_t* avi = AVI_open_output_file(gifPath.toLocal8Bit().data());
        AVI_set_video(avi, wt, ht, 1000/interval, "mjpg");

        for (int i = 0; i < pixmapPaths.size(); i++)
        {
            QPixmap pixmap(pixmapPaths.at(i));
            if (!pixmap.isNull())
            {
                if (prop > 1)
                    pixmap = pixmap.scaled(static_cast<int>(wt), static_cast<int>(ht));
                QByteArray ba;
                QBuffer    bf(&ba);
                if (!pixmap.save(&bf, "jpg", -1))
                {
                    qDebug() << "保存图片Buffer失败" << pixmapPaths.at(i);
                    continue;
                }
                AVI_write_frame(avi, ba.data(), ba.size(), 1);
            }
            emit signalGeneralGIFProgress(i+1);
        }

        AVI_close(avi);

        emit signalGeneralGIFFinished(gifPath);
        PBDEB << "AVI生成完毕：" << size << pixmapPaths.size() << interval << compress;
    });
}

void PictureBrowser::on_actionCreate_To_Origin_Folder_triggered()
{
    settings.setValue("gif/createToOne", false);
}

void PictureBrowser::on_actionCreate_To_One_Folder_triggered()
{
    settings.setValue("gif/createToOne", true);
}

void PictureBrowser::on_actionResize_Auto_Init_triggered()
{
    bool init = ui->actionResize_Auto_Init->isChecked();
    settings.setValue("picturebrowser/resizeAutoInit", init);
    ui->previewPicture->setResizeAutoInit(init);
}

/**
 * 根据显示的大小裁剪选中项
 * 需要图片大小一致，不一致的跳过
 */
void PictureBrowser::on_actionClip_Selected_triggered()
{
    // 获取裁剪位置
    QSize originSize;
    QRect imageArea;
    QRect showArea;
    ui->previewPicture->getClipArea(originSize, imageArea, showArea);
    if (!originSize.isValid())
        return ;

    // 计算裁剪比例（全都是比例，适用于所有图片）
    double clipLeft = (showArea.left() - imageArea.left()) / (double)imageArea.width();
    double clipTop = (showArea.top() - imageArea.top()) / (double)imageArea.height();
    double clipRight = (showArea.right() - imageArea.left()) / (double)imageArea.width();
    double clipBottom = (showArea.bottom() - imageArea.top()) / (double)imageArea.height();
    if (clipLeft >=1 || clipTop >= 1 || clipRight <= 0 || clipBottom <= 0) // 没有显示出来的
        return ;
    if (clipLeft < 0)
        clipLeft = 0;
    if (clipTop < 0)
        clipTop = 0;
    if (clipRight > 1)
        clipRight = 1;
    if (clipBottom > 1)
        clipBottom = 1;
    double clipWidth = clipRight - clipLeft;
    double clipHeight = clipBottom - clipTop;
    if (clipWidth >= 1 && clipHeight >= 1) // 不需要裁剪
        return ;

    // 获取选中项
    removeUselessItemSelect();
    auto selectedItems = ui->listWidget->selectedItems();
    if (selectedItems.size() == 0)
        return ;

    // 图标尺寸
    QSize maxIconSize = ui->listWidget->iconSize();
    if (maxIconSize.width() <= 16 || maxIconSize.height() <= 16)
        maxIconSize = QSize(32, 32);

    foreach (auto item, selectedItems)
    {
        QString path = item->data(FilePathRole).toString();
        QFileInfo info(path);
        if (!info.exists() || !info.isFile())
            continue;
        QString suffix = info.suffix();
        if (suffix != "jpg" && suffix != "png" && suffix != "jpeg")
            continue;

        // 先拷贝文件
        QString newPath = recycleDir.absoluteFilePath(info.fileName());
        deleteCommandsQueue.append(QPair<QString, QString>(path, newPath));
        QFile file(path);
        file.copy(newPath);

        // 裁剪图片
        QPixmap pixmap(path);
        pixmap = pixmap.copy(pixmap.width()*clipLeft, pixmap.height()*clipTop,
                             pixmap.width()*clipWidth, pixmap.height()*clipHeight);
        pixmap.save(path);
        pixmap = pixmap.scaled(maxIconSize, Qt::AspectRatioMode::KeepAspectRatio);
        item->setIcon(QIcon(pixmap));
    }

    commitDeleteCommand();
    showCurrentItemPreview();
}

void PictureBrowser::on_actionSort_Enabled_triggered()
{
    fastSort = !fastSort;
    settings.setValue("picturebrowser/fastSort", fastSort);
}

void PictureBrowser::on_actionCreate_Folder_triggered()
{
    bool ok;
    QString name = QInputDialog::getText(this, "新建文件夹", "请输入文件夹名字", QLineEdit::Normal, "新建文件夹", &ok);
    if (!ok)
        return ;

    QDir(currentDirPath).mkpath(name);
    enterDirectory(currentDirPath);
}

void PictureBrowser::on_actionAutoColor_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionColorOnly_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionMonoOnly_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionDiffuseDither_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionOrderedDither_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionThresholdDither_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionThresholdAlphaDither_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionOrderedAlphaDither_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionDiffuseAlphaDither_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionPreferDither_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionAvoidDither_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionAutoDither_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionNoOpaqueDetection_triggered()
{
    saveImageConversionFlag();
}

void PictureBrowser::on_actionNoFormatConversion_triggered()
{
    saveImageConversionFlag();
}
