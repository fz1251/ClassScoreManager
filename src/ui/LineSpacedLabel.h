#pragma once

#include <QLabel>

class QTextDocument;

class LineSpacedLabel : public QLabel
{
    Q_OBJECT

public:
    explicit LineSpacedLabel(QWidget* parent = nullptr);

    void setLineHeight(qreal lineHeight);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void prepareDocument(QTextDocument& document, qreal textWidth) const;
    QSize preparedDocumentSize() const;

    qreal m_lineHeight = 1.4;
};
