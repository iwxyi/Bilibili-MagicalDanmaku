#ifndef CONDITIONLINEEDITOR_H
#define CONDITIONLINEEDITOR_H

#include "conditioneditor.h"
#include <QAbstractItemView>

class ConditionLineEditor : public ConditionEditor
{
    Q_OBJECT
public:
    ConditionLineEditor(QWidget *parent = nullptr) : ConditionEditor(parent)
    {
        QFontMetrics fm(font());
        setFixedHeight(fm.height() + 12); // 8为内边距，可微调

        // 自动调整宽度
        QObject::connect(this, &QPlainTextEdit::textChanged, [this]() {
            if (!_autoAdjustWidth)
                return;
            QString text = toPlainText();
            QStringList lines = text.split('\n');
            QString longestLine;
            for (const QString &line : lines) {
                if (line.length() > longestLine.length())
                    longestLine = line;
            }
            QFontMetrics fm(font());
            int width = fm.horizontalAdvance(longestLine);
            int margins = contentsMargins().left() + contentsMargins().right();
            int docMargin = document()->documentMargin() * 2;
            int extra = 20; // 滚动条、光标、边框等冗余
            int w = width + margins + docMargin + extra;
            if (w < _minWidth)
                w = _minWidth;
            setFixedWidth(w);
        });

        // 隐藏滚动条
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    void setAutoAdjustWidth(bool autoAdjustWidth)
    {
        _autoAdjustWidth = autoAdjustWidth;
    }

    void setMinimumWidth(int minWidth)
    {
        _minWidth = minWidth;
    }

signals:
    void returnPressed();
    void tabPressed();

protected:
    void keyPressEvent(QKeyEvent *e) override
    {
        if (completer->popup() && completer->popup()->isVisible()) {
            return ConditionEditor::keyPressEvent(e);
        }

        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            emit returnPressed();
            return;
        }
        if (e->key() == Qt::Key_Tab) {
            emit tabPressed();
            return;
        }
        ConditionEditor::keyPressEvent(e);
    }

private:
    bool _autoAdjustWidth = true;
    int _minWidth = 0;
};

#endif // CONDITIONLINEEDITOR_H
