#pragma once

#include <QObject>

class lcPartSelectionWidget;
class lcPartSelectionListView;
class lcTrainTrackInfo;

QString lcFormatValue(float Value, int Precision);
QString lcFormatValueLocalized(float Value);
float lcParseValueLocalized(const QString& Value);

class lcQTreeWidgetColumnStretcher : public QObject
{
	Q_OBJECT

public:
	lcQTreeWidgetColumnStretcher(QTreeWidget* TreeWidget, int ColumnToStretch);

	bool eventFilter(QObject* Object, QEvent* Event) override;

private slots:
	void sectionResized(int LogicalIndex, int OldSize, int NewSize);

private:
	const int m_columnToStretch;
	bool m_interactiveResize;
	int m_stretchWidth;
};

class lcSmallLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	QSize sizeHint() const override
	{
		QFontMetrics FontMetrics(font());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
		const int Width = FontMetrics.horizontalAdvance(QLatin1Char('x')) * 10;
#else
		const int Width = FontMetrics.width(QLatin1Char('x')) * 10;
#endif

		return QLineEdit::sizeHint() - QSize(Width, 0);
	}
};

class lcTransformLineEdit : public lcSmallLineEdit
{
	Q_OBJECT

protected:
	bool event(QEvent* Event) override
	{
		if (Event->type() == QEvent::ShortcutOverride)
		{
			const QKeyEvent* KeyEvent = (QKeyEvent*)Event;
			const int Key = KeyEvent->key();

			if (KeyEvent->modifiers() == Qt::NoModifier && Key >= Qt::Key_A && Key <= Qt::Key_Z)
				Event->accept();

			switch (Key)
			{
			case Qt::Key_Down:
			case Qt::Key_Up:
			case Qt::Key_Left:
			case Qt::Key_Right:
			case Qt::Key_Home:
			case Qt::Key_End:
			case Qt::Key_PageUp:
			case Qt::Key_PageDown:
			case Qt::Key_Plus:
			case Qt::Key_Minus:
			case Qt::Key_Enter:
				Event->accept();
				break;
			}
		}

		return QLineEdit::event(Event);
	}
};

class lcStepValidator : public QIntValidator
{
	Q_OBJECT

public:
	lcStepValidator(lcStep Min, lcStep Max, bool AllowEmpty, QObject* Parent)
		: QIntValidator(1, INT_MAX, Parent), mMin(Min), mMax(Max), mAllowEmpty(AllowEmpty)
	{
	}

	QValidator::State validate(QString& Input, int& Pos) const override
	{
		if (mAllowEmpty && Input.isEmpty())
			return Acceptable;

		bool Ok;
		lcStep Step = Input.toUInt(&Ok);

		if (Ok)
			return (Step >= mMin && Step <= mMax) ? Acceptable : Invalid;

		return QIntValidator::validate(Input, Pos);
	}

protected:
	lcStep mMin;
	lcStep mMax;
	bool mAllowEmpty;
};

class lcElidableToolButton : public QToolButton
{
	Q_OBJECT

public:
	lcElidableToolButton(QWidget* Parent)
		: QToolButton(Parent)
	{
	}

	QSize sizeHint() const override
	{
		QSize Size = QToolButton::sizeHint();

		Size.setWidth(0);

		return Size;
	}

protected:
	void paintEvent(QPaintEvent*)
	{
		QStylePainter Painter(this);
		QStyleOptionToolButton Option;
		initStyleOption(&Option);

		QRect Button = style()->subControlRect(QStyle::CC_ToolButton, &Option, QStyle::SC_ToolButton, this);
		int Frame = style()->proxy()->pixelMetric(QStyle::PixelMetric::PM_DefaultFrameWidth, &Option, this);
		Button = Button.adjusted(Frame, Frame, -Frame, -Frame);

		QFontMetrics Metrics(font());
		QString ElidedText = Metrics.elidedText(text(), Qt::ElideMiddle, Button.width());

		Option.text = ElidedText;
		Painter.drawComplexControl(QStyle::CC_ToolButton, Option);
	}
};

class lcPieceIdStringModel : public QAbstractListModel
{
	Q_OBJECT

public:
	lcPieceIdStringModel(lcModel* Model, QObject* Parent);

	QModelIndex Index(PieceInfo* Info) const;
	std::vector<bool> GetFilteredRows(const QString& FilterText) const;

	int rowCount(const QModelIndex& Parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& Index, int Role = Qt::DisplayRole) const override;

protected:
	std::vector<PieceInfo*> mSortedPieces;
};

class lcPieceIdPickerPopup : public QWidget
{
	Q_OBJECT

public:
	lcPieceIdPickerPopup(PieceInfo* Current, QWidget* Parent);

signals:
	void PieceIdSelected(PieceInfo* Info);

protected slots:
	void Accept();
	void Reject();
	void PartPicked(PieceInfo* Info);

protected:
	void showEvent(QShowEvent* ShowEvent) override;
	void Close();

	lcPartSelectionWidget* mPartSelectionWidget = nullptr;
	PieceInfo* mInitialPart = nullptr;
};

class lcTrainTrackPickerPopup : public QWidget
{
	Q_OBJECT

public:
	lcTrainTrackPickerPopup(QWidget* Parent, const lcTrainTrackInfo* TrainTrackInfo);

	PieceInfo* GetPickedTrainTrack() const
	{
		return mPickedTrainTrack;
	}

protected slots:
	void Accept();
	void Reject();

protected:
	void showEvent(QShowEvent* ShowEvent) override;
	void Close();

	lcPartSelectionListView* mPartSelectionListView = nullptr;
	PieceInfo* mPickedTrainTrack = nullptr;
};

PieceInfo* lcShowTrainTrackPopup(QWidget* Parent, const lcTrainTrackInfo* TrainTrackInfo);

class lcColorDialogPopup : public QWidget
{
	Q_OBJECT

public:
	lcColorDialogPopup(const QColor& InitialColor, QWidget* Parent);

signals:
	void ColorSelected(QColor Color);

protected slots:
	void Accept();
	void Reject();

protected:
	void Close();

	QColorDialog* mColorDialog = nullptr;
};
