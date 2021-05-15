#include "impqt.h"

#include <QRegularExpression>
#include <QMap>
#include <QLabel>
#include <QPainter>


//Define some operations, and, or, not.
const QString andOpr = " and ";
const QString orOpr = " or ";
const QString notOpr = " not ";


//Testing data.
const QString g_input = R"(a=10;
b=20;
c=30;
if a == 10 then
    b = 30;
else
    b=40;
endif
while a>0 do
    a = a-1;
endwhile
b=20;
d=b;)";


struct KsStruct
{
	QString toFormual() const {
		if (!condition.isEmpty())
			return QString("pc=%1 %2 pc'=%3 %4 %5")
			.arg(preLable).arg(andOpr).arg(postLable).arg(andOpr).arg(condition);
		else
			return QString("pc=%1 %2 pc'=%3")
			.arg(preLable).arg(andOpr).arg(postLable);
	}

	QString preLable;
	QString postLable;
	QString condition;
};

using KsStructs = QVector<KsStruct>;


QVector<QString> getStatesFromKS(const KsStructs& kss) {
	QVector<QString> states;
	for (const auto& v : kss) {
		if (!states.contains(v.preLable))
			states.push_back(v.preLable);
	}

	for (const auto& v : kss) {
		if (!states.contains(v.postLable))
			states.push_back(v.postLable);
	}
	return states;
}

//Widget for painting KS graph.
class KsStructsWidget: public QLabel {
public:
	KsStructsWidget(const KsStructs& kss) {
		_kss = kss;
		_labels = getStatesFromKS(kss);
	}

protected:
	void paintEvent(QPaintEvent*);

private:
	QVector<QString> _labels;
	QMap<QString, QRectF> _labelsGem;
	KsStructs _kss;
};

void calcVertexes(double start_x, double start_y, double end_x, double end_y, double& x1, double& y1, double& x2, double& y2)
{
	double arrow_lenght_ = 10;//箭头长度，一般固定
	double arrow_degrees_ = 0.5;//箭头角度，一般固定

	double angle = atan2(end_y - start_y, end_x - start_x) + 3.1415926;

	x1 = end_x + arrow_lenght_ * cos(angle - arrow_degrees_);//求得箭头点1坐标
	y1 = end_y + arrow_lenght_ * sin(angle - arrow_degrees_);
	x2 = end_x + arrow_lenght_ * cos(angle + arrow_degrees_);//求得箭头点2坐标
	y2 = end_y + arrow_lenght_ * sin(angle + arrow_degrees_);
}

void KsStructsWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);

	painter.drawText(QPoint(20, 20), "Kripke Structure graph:");

	int n = 0;
	QSize size(60, 40);
	int yPos = 0;
	int col = 0;
	while (n < _labels.size()) {
		++col;
		yPos += 80;
		for (int n1 = 0; n1 < col; ++n1) {
			if (n >= _labels.size()) {
				break;
			}
			QRect rect(n1 * 80, yPos, size.width(), size.height());
			painter.drawEllipse(rect);
			painter.drawText(rect, Qt::AlignCenter, _labels.at(n));
			_labelsGem[_labels.at(n)] = rect;
			++n;
		}
	}

	for (const auto& v : _kss) {
		if (v.preLable == v.postLable)
			continue;
		double x1, y1, x2, y2;
		QPointF start = _labelsGem.value(v.preLable).center();
		start.setY(start.y());
		QPointF end = _labelsGem.value(v.postLable).center();
		end.setY(end.y() - 10);
		calcVertexes(start.x(), start.y(), end.x(), end.y(), x1, y1, x2, y2);
		painter.drawLine(start, end);
		painter.drawLine(end, { x1, y1 });
		painter.drawLine(end, { x2, y2 });
	}
}


//combine two statements with 'and' operation.
QString toNotCondition(const QString& condition) {
	return QString("(not %1)").arg(condition);
}


QString lineLable(const QString& line) {
	return line.mid(0, line.indexOf(':'));
}

QString lableAdd(const QString& lable, int add) {
	QString num = lable.mid(1);
	return QString("L%1").arg(num.toInt() + add);
}

//Define three statements type here.
enum class StatementType
{
	Squence = 0,
	If = 1,
	While = 2
};


//Construct of main class.
ImpQt::ImpQt(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

	QFont font = ui.inputEdit->font();
	font.setPointSize(12);
	ui.inputEdit->setFont(font);
	ui.outputEdit->setFont(font);

    connect(ui.startBtn, &QPushButton::clicked, this, &ImpQt::onStart);

    //Testing data
    ui.inputEdit->setText(g_input);
}


QVector<KsStruct> parseSeqence( const QString &input ) {
	QVector<KsStruct> ksrs;
	QStringList list = input.split(QRegExp("\n+"), Qt::SkipEmptyParts);

	if (list.size() == 1) {
		ksrs.push_back({ lineLable(list.first()), lineLable(list.first()) });
	}
	
	for( int n = 1; n<list.size(); ++n )
		ksrs.push_back({ lineLable(list.at(n-1)), 
			lineLable(list.at(n)), 
			QString() });

	return ksrs;
}


KsStructs parseIf( const QString &input ) {
	KsStructs kss;
	QStringList list = input.split(QRegExp("\n+"), Qt::SkipEmptyParts);
	QString ifLable, elseLable, endLable, condition;
	QRegularExpression re("if(.+)then");
	QRegularExpressionMatch match = re.match(input);
	if (match.hasMatch()) {
		condition = match.captured(1).trimmed();
	}

	auto isEnd = [](const QString& line) {
		return line.contains("endif");
	};
	auto isIf = [](const QString& line) {
		return line.contains("if") && !line.contains("endif");
	};
	auto isElse = [](const QString& line) {
		return line.contains("else");
	};

	//Get position of if, else and end.
	int ifPos = 0, endPos = 0, elsePos = 0;
	for (int n = 0; n < list.size(); ++n) {
		QString v = list.at(n);
		if (isIf(v)) {
			ifLable = lineLable(v);
			ifPos = n;
		}
		else if (isElse(v)) {
			elseLable = lineLable(v);
			elsePos = n;
		}
		else if (isEnd(v)) {
			endLable = lineLable(v);
			endPos = n;
		}
	}

	if (0 == elsePos)
		elsePos = endPos;

	kss.push_back({ ifLable, lineLable(list.at(ifPos + 1)), condition });
	if (elsePos != endPos) {
		kss.push_back({ ifLable, elseLable, toNotCondition(condition) });
		kss.push_back({ lableAdd(elseLable, -1), endLable });
		kss.push_back({ elseLable, lableAdd(elseLable, 1) });
	}
	else {
		kss.push_back({ ifLable, endLable, toNotCondition(condition) });
	}
	kss.push_back({ lableAdd(endLable, -1), endLable });

	for (int n = 2; n < elsePos - 1; ++n) {
		kss.push_back({ lineLable(list.at(n-1)), lineLable(list.at(n)) });
	}

	for (int n = elsePos + 2; n < endPos - 1; ++n) {
		kss.push_back({ lineLable(list.at(n - 1)), lineLable(list.at(n)) });
	}

	return kss;
}

KsStructs parseWhile(const QString& input) {
	QStringList lines = input.split(QRegularExpression("\n+"), Qt::SkipEmptyParts);
	if (lines.size() < 2) {
		return KsStructs();
	}

	QString condition;
	KsStructs kss;
	QRegularExpression re("while(.+)do");
	QRegularExpressionMatch match = re.match(input);
	if (match.hasMatch()) {
		condition = match.captured(1).trimmed();
	}
	kss.push_back({ lineLable(lines.first()), lineLable(lines.at(1)), condition });
	kss.push_back({ lineLable(lines.first()), lineLable(lines.last()), QString("(not %1)").arg(condition) });

	for (int n = 2; n < lines.size(); ++n) {
		kss.push_back({ lineLable(lines.at(n-1)), lineLable(lines.at(n)), QString() });
	}

	return kss;
}

//Read all input words and parse them to sequence, if, and while statement
QVector<QPair<QString, StatementType>> toStatements(const QString &input) {

	QVector<QPair<QString, StatementType>> statememts;
	int statementPos = 0;
	int start = 0;
	int end = 0;
	//Find all if statements.

	while (start < input.length())
	{
		start = input.indexOf(QRegExp("L[^L]+(if|while)"), start);
		if (-1 == start) {
			QString statement = input.mid(statementPos, input.length() - statementPos);
			statememts.push_back({ statement, StatementType::Squence });
			break;
		}
		else {
			QString statement;
			if (start > statementPos) {
				statement = input.mid(statementPos, start - statementPos);
				statememts.push_back({ statement, StatementType::Squence });
			}

			end = input.indexOf(QRegExp("(endif|endwhile)"), start);
			bool isIf = 'i' == input.at(end+3);
			end = input.indexOf("L",end);
			if (end == -1) {
				end = input.length();
			}
			statement = input.mid(start, end - start);
			statememts.push_back({ statement, isIf ? StatementType::If : StatementType::While });
			start = end;
			statementPos = start;
		}
	}
	return statememts;
}


void ImpQt::onStart()
{
	//Do some clear works.
	ui.outputEdit->clear();

	QString input = ui.inputEdit->toPlainText();

	//Tag all lines
	QString inputWithTag = tagAllLines(input);

	ui.outputEdit->append("Labeled:");
	ui.outputEdit->append(inputWithTag);

	QVector<QPair<QString, StatementType>> statements = toStatements(inputWithTag);

	QVector<KsStructs> allKsStructs;
	for (const auto& v : statements) {
		if (StatementType::Squence == v.second) {
			allKsStructs.push_back(parseSeqence(v.first));
		}
		else if ( StatementType::If == v.second ) {
			allKsStructs.push_back(parseIf(v.first));
		}
		else if (StatementType::While == v.second) {
			allKsStructs.push_back(parseWhile(v.first));
		}
	}

	//Combine all segments to one
	KsStructs finalKsStructs;
	QString pre;
	for (const auto& ksStructs : allKsStructs) {
		if ( ksStructs.isEmpty() )
			continue;

		if (!pre.isEmpty()) {
			finalKsStructs.push_back({ pre, ksStructs.first().preLable, QString() });
		}
		pre = ksStructs.last().postLable;
		for (const auto& one : ksStructs) {
			finalKsStructs.push_back(one);
		}
	}

	ui.outputEdit->append("\n\nFormual:");
	for (const auto& v : finalKsStructs) {
		ui.outputEdit->append(v.toFormual());
	}

	ui.outputEdit->append("\n\nKripke struct:\nR={");
	for (const auto& v : finalKsStructs) {
		ui.outputEdit->append(QString("{%1,%2}").arg(v.preLable).arg(v.postLable));
	}

	ui.outputEdit->append("}\n\nS={");
	QVector<QString> states = getStatesFromKS(finalKsStructs);
	for (const auto& v : states) {
		ui.outputEdit->append(v);
	}
	ui.outputEdit->append("}\n\n");

	//Draw KS graph
	QWidget* widget = new KsStructsWidget(finalKsStructs);
	ui.scrollArea->setWidget(widget);
	widget->setGeometry(0, 0, ui.scrollArea->width(), ui.scrollArea->height());
}


QString ImpQt::tagAllLines( const QString &input )
{
	QString output;
	QStringList lines = input.split(QRegularExpression("\n+"), Qt::SkipEmptyParts);
	int index = 0;
	for (auto& v : lines) {
		output.append(QString("L%1: %2\n").arg(index++).arg(v));
	}
	return output;
}