// DiveGram Plus — purchase panel for the Premium settings section.
#pragma once

#include "ui/rp_widget.h"
#include "ui/effects/animations.h"

#include <rpl/producer.h>
#include <rpl/variable.h>

#include <crl/crl.h>

#include <QBrush>
#include <QColor>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QString>
#include <QVector>

#include <vector>

namespace Ui {
class VerticalLayout;
} // namespace Ui

namespace Divegram::UiPlus {

struct TariffInfo {
	QString name;
	QString oldPrice; // empty => no strikethrough
	QString price;
	QString discount; // empty => no badge
};

[[nodiscard]] std::vector<TariffInfo> Tariffs();
[[nodiscard]] QColor PlusColor();
[[nodiscard]] QGradientStops PlusGradientStops();

// Full-size overlay that paints a radial ripple + particle burst.
class BurstOverlay final : public Ui::RpWidget {
public:
	BurstOverlay(QWidget *parent);

	void burst(const QPointF &origin);
	void stopBurst();
	[[nodiscard]] bool bursting() const;

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	[[nodiscard]] double burstProgress() const;

	Ui::Animations::Basic _animation;
	QPointF _origin;
	crl::time _started = -1;
	crl::time _seed = 0;
};

// Vertical liquid-glass toggle with two round buttons.
class GlassToggle final : public Ui::RpWidget {
public:
	GlassToggle(QWidget *parent);

	[[nodiscard]] rpl::variable<int> &selected();
	void setSelected(int value, bool animated = true);
	void setScale(float64 scale);
	void setFade(float64 fade);

	[[nodiscard]] QRectF toggleRect() const;
	[[nodiscard]] QPointF buttonCenter(int value) const;

protected:
	void paintEvent(QPaintEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;

private:
	[[nodiscard]] int valueAt(const QPoint &position) const;
	void paintRoundIcon(QPainter &p, int value, const QRectF &r);
	[[nodiscard]] QPointF buttonCenterDesign(int value) const;
	[[nodiscard]] QRectF pillRect(int value) const;

	rpl::variable<int> _value = 0;
	Ui::Animations::Simple _pill;
	Ui::Animations::Simple _pop;
	QRectF _pillFrom;
	QRectF _pillTo;
	float64 _scale = 1.;
	float64 _fade = 1.;
	int _press = -1;
};

// Fills the DiveGram Plus tab content (perks + tariffs).
void FillDiveGramPlusPage(not_null<Ui::VerticalLayout*> container);

} // namespace Divegram::UiPlus