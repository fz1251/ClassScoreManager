#include "LineSpacedLabel.h"

#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <QtMath>

LineSpacedLabel::LineSpacedLabel(QWidget* parent)
    : QLabel(parent)
{
}

void LineSpacedLabel::setLineHeight(qreal lineHeight)
{
    if(qFuzzyCompare(m_lineHeight, lineHeight))
        return;
    m_lineHeight = lineHeight;
    updateGeometry();
    update();
}

QSize LineSpacedLabel::sizeHint() const
{
    if(wordWrap())
        return QLabel::sizeHint();
    return preparedDocumentSize();
}

QSize LineSpacedLabel::minimumSizeHint() const
{
    if(wordWrap())
        return QLabel::minimumSizeHint();
    return preparedDocumentSize();
}

void LineSpacedLabel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    QTextDocument document;
    prepareDocument(document, wordWrap() ? contentsRect().width() : -1);

    QAbstractTextDocumentLayout::PaintContext context;
    context.palette = palette();
    document.drawContents(&painter, QRectF(contentsRect()));
}

void LineSpacedLabel::prepareDocument(QTextDocument& document, qreal textWidth) const
{
    document.setDefaultFont(font());
    document.setDocumentMargin(0);
    document.setPlainText(text());

    QTextCursor cursor(&document);
    cursor.select(QTextCursor::Document);
    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(m_lineHeight * 100, QTextBlockFormat::ProportionalHeight);
    cursor.mergeBlockFormat(blockFormat);

    QTextOption option = document.defaultTextOption();
    option.setAlignment(alignment());
    document.setDefaultTextOption(option);
    document.setTextWidth(textWidth);
}

QSize LineSpacedLabel::preparedDocumentSize() const
{
    QTextDocument document;
    prepareDocument(document, -1);
    const QSizeF size = document.size();
    return QSize(qCeil(size.width()), qCeil(size.height()));
}
