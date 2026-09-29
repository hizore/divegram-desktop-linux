// DiveGram Plus — implementation of the plastic "glass" premium panel.
//
// Copyright @Radolyn, 2026
#include "divegram/ui/settings/divegram_plus.h"

#include "ayu/ui/ayu_logo.h"
#include "ui/effects/liquid_glass.h"
#include "lang/lang_keys.h"
#include "ui/painter.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/vertical_list.h"
#include "styles/style_basic.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_premium.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"

#include <rpl/producer.h>
#include <rpl/range.h>

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <algorithm>

namespace Divegram::UiPlus {
namespace {

constexpr auto kBurstDuration = crl::time(1200);
constexpr auto kGoldenAngle = 2.39996322972865332;
constexpr auto kPi = 3.14159265358979323846;

[[nodiscard]] QColor Mix(const QColor &a, const QColor &b, double k) {
	return QColor(
		int(a.red() + (b.red() - a.red()) * k),
		int(a.green() + (b.green() - a.green()) * k),
		int(a.blue() + (b.blue() - a.blue()) * k),
		int(a.alpha() + (b.alpha() - a.alpha()) * k));
}

[[nodiscard]] double EaseOutCubic(double t) {
	const auto u = 1. - t;
	return 1. - u * u * u;
}

[[nodiscard]] QColor White(int alpha) {
	return QColor(255, 255, 255, alpha);
}

[[nodiscard]] QRectF InterpolateRect(
		const QRectF &from,
		const QRectF &to,
		float64 progress) {
	return QRectF(
		from.x() + (to.x() - from.x()) * progress,
		from.y() + (to.y() - from.y()) * progress,
		from.width() + (to.width() - from.width()) * progress,
		from.height() + (to.height() - from.height()) * progress);
}

void PaintTelegramPlane(
		QPainter &p,
		const QRectF &r,
		const QColor &color) {
	p.save();
	p.translate(r.center());
	p.scale(r.width() / 24., r.height() / 24.);
	p.translate(-12., -12.);
	auto path = QPainterPath();
	path.moveTo(2.0, 11.7);
	path.lineTo(21.5, 2.2);
	path.lineTo(17.2, 21.0);
	path.lineTo(12.8, 14.5);
	path.lineTo(7.2, 20.0);
	path.lineTo(8.1, 13.9);
	path.closeSubpath();
	p.setBrush(color);
	p.setPen(Qt::NoPen);
	p.drawPath(path);
	p.restore();
}

void PaintCheck(QPainter &p, const QRectF &r, const QColor &color) {
	auto path = QPainterPath();
	path.moveTo(r.left(), r.top() + r.height() * 0.5);
	path.lineTo(r.left() + r.width() * 0.35, r.bottom() - r.height() * 0.1);
	path.lineTo(r.right(), r.top() + r.height() * 0.15);
	p.setPen(QPen(
		color,
		std::max(1., r.width() * 0.16),
		Qt::SolidLine,
		Qt::RoundCap,
		Qt::RoundJoin));
	p.setBrush(Qt::NoBrush);
	p.drawPath(path);
}

void PaintSparkle(
		QPainter &p,
		const QPointF &center,
		double size,
		const QColor &color) {
	const auto s = size;
	auto path = QPainterPath();
	path.moveTo(center.x(), center.y() - s);
	path.quadTo(center.x(), center.y(), center.x() + s, center.y());
	path.quadTo(center.x(), center.y(), center.x(), center.y() + s);
	path.quadTo(center.x(), center.y(), center.x() - s, center.y());
	path.quadTo(center.x(), center.y(), center.x(), center.y() - s);
	p.setPen(Qt::NoPen);
	p.setBrush(color);
	p.drawPath(path);
}

void PaintTextTopLeft(
		QPainter &p,
		const QString &text,
		const style::font &font,
		const QColor &color,
		const QPointF &topLeft) {
	p.setFont(font);
	p.setPen(color);
	p.drawText(
		QRectF(topLeft, QSizeF(1.e9, 1.e9)),
		Qt::AlignTop | Qt::AlignLeft,
		text);
}

class TariffRow final : public Ui::AbstractButton {
public:
	TariffRow(not_null<Ui::RpWidget*> parent, const TariffInfo &info)
	: AbstractButton(parent)
	, info(info) {
		setFixedHeight(72);
	}

	void setSelected(bool value) {
		if (_selected == value) {
			return;
		}
		_selected = value;
		update();
	}

	[[nodiscard]] bool selected() const {
		return _selected;
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		auto hq = PainterHighQualityEnabler(p);

		const auto &plus = PlusColor();
		const auto r = rect().marginsRemoved(QMargins(1, 1, 1, 1));
		auto path = QPainterPath();
		path.addRoundedRect(r, 14., 14.);
		p.fillPath(path, _selected
			? Mix(st::windowBg->c, plus, 0.12)
			: st::windowBg->c);
		p.setPen(QPen(_selected ? plus : st::windowSubTextFg->c, 1.5));
		p.drawPath(path);

		const auto left = r.left() + 14;
		const auto top = r.top() + 14;
		const auto textLeft = left + 26;
		const auto nameWidth = st::semiboldFont->width(info.name);
		PaintTextTopLeft(
			p,
			info.name,
			st::semiboldFont,
			_selected ? plus : st::windowBoldFg->c,
			QPointF(textLeft, top));

		const auto priceWidth = st::semiboldFont->width(info.price);
		const auto priceHeight = st::semiboldFont->height;
		const auto priceCenterX = r.left() + (r.width() - priceWidth) / 2.;
		const auto priceTop = r.top() + (r.height() - priceHeight) / 2.;
		PaintTextTopLeft(
			p,
			info.price,
			st::semiboldFont,
			_selected ? plus : st::windowBoldFg->c,
			QPointF(priceCenterX, priceTop));

		const auto hasOld = !info.oldPrice.isEmpty();
		const auto oldTop = r.bottom() - st::normalFont->height - 8;
		const auto oldRight = hasOld
			? textLeft + st::normalFont->width(info.oldPrice)
			: textLeft;
		if (hasOld) {
			PaintTextTopLeft(
				p,
				info.oldPrice,
				st::normalFont,
				st::windowSubTextFg->c,
				QPointF(textLeft, oldTop));
			p.setPen(QPen(st::windowSubTextFg->c, 1.2));
			p.drawLine(
				QPointF(textLeft, oldTop + st::normalFont->ascent * 0.55),
				QPointF(oldRight, oldTop + st::normalFont->ascent * 0.55));
		}

		if (!info.discount.isEmpty()) {
			const auto text = u"-"_q + info.discount;
			const auto badgeW = st::normalFont->width(text) + 14;
			const auto priceRect = QRectF(
				priceCenterX - 6,
				priceTop - 2,
				priceWidth + 12,
				priceHeight + 4);
			auto badgeLeft = textLeft + nameWidth + 8;
			auto badgeTop = top + (st::semiboldFont->height - 18.) / 2.;
			if (QRectF(badgeLeft, badgeTop, badgeW, 18.).intersects(priceRect)) {
// Move the tag down so it doesn't cover the price.
				badgeLeft = hasOld ? oldRight + 8 : textLeft;
				badgeTop = oldTop;
			}
			if (badgeLeft + badgeW > r.right() - 8) {
				badgeLeft = std::max(textLeft, r.right() - 8 - badgeW);
			}
			auto badge = QRectF(
				badgeLeft,
				badgeTop,
				badgeW,
				18);
			auto badgePath = QPainterPath();
			badgePath.addRoundedRect(badge, 9., 9.);
			p.setPen(Qt::NoPen);
			p.setBrush(plus);
			p.drawPath(badgePath);
			PaintTextTopLeft(
				p,
				text,
				st::normalFont,
				White(255),
				QPointF(badge.left() + 7, badge.top() + 2));
		}

		if (_selected) {
			PaintCheck(p, QRectF(left + 2, top + 2, 16, 16), plus);
		}
	}

public:
	TariffInfo info;

private:
	bool _selected = false;
};

struct TariffState {
	std::vector<TariffRow*> rows;
	int selected = 0;
};

} // namespace

std::vector<TariffInfo> Tariffs() {
	return std::vector<TariffInfo>{
		{
			.name = tr::lng_divegram_plus_tariff_month(tr::now),
			.oldPrice = {},
			.price = u"199 \u20BD"_q,
			.discount = {},
		},
		{
			.name = tr::lng_divegram_plus_tariff_year(tr::now),
			.oldPrice = u"1200 \u20BD"_q,
			.price = u"660 \u20BD"_q,
			.discount = u"45%"_q,
		},
		{
			.name = tr::lng_divegram_plus_tariff_forever(tr::now),
			.oldPrice = u"1611 \u20BD"_q,
			.price = u"676,7 \u20BD"_q,
			.discount = u"58%"_q,
		},
	};
}

QColor PlusColor() {
	return QColor(0x7C, 0x3A, 0xED);
}

QGradientStops PlusGradientStops() {
	return QGradientStops{
		{ 0.00, QColor(0x3B, 0x0A, 0x6F) },
		{ 0.45, QColor(0x7C, 0x3A, 0xED) },
		{ 1.00, QColor(0xA7, 0x8B, 0xFA) },
	};
}

BurstOverlay::BurstOverlay(QWidget *parent)
: Ui::RpWidget(parent)
, _animation([=] {
	update();
	if (burstProgress() >= 1.) {
		stopBurst();
	}
}) {
	setAttribute(Qt::WA_TransparentForMouseEvents);
	hide();
}

double BurstOverlay::burstProgress() const {
	return (_started < 0)
		? 1.
		: std::clamp(
			(crl::now() - _started) / double(kBurstDuration),
			0.,
			1.);
}

void BurstOverlay::burst(const QPointF &origin) {
	_origin = origin;
	_started = crl::now();
	_seed = crl::now();
	show();
	raise();
	_animation.start();
}

void BurstOverlay::stopBurst() {
	if (_started < 0) {
		return;
	}
	_started = -1;
	_animation.stop();
	hide();
}

bool BurstOverlay::bursting() const {
	return (_started >= 0);
}

void BurstOverlay::paintEvent(QPaintEvent *e) {
	QPainter p(this);
	auto hq = PainterHighQualityEnabler(p);

	const auto progress = burstProgress();
	if (progress >= 1.) {
		return;
	}
	const auto eased = EaseOutCubic(progress);
	const auto maxDim = std::hypot(width(), height());
	const auto plus = PlusColor();
	const auto fade = 1. - progress;

	const auto glowRadius = maxDim * (0.15 + 0.45 * eased);
	auto glow = QRadialGradient(_origin, glowRadius);
	glow.setColorAt(0., QColor(
		plus.red(),
		plus.green(),
		plus.blue(),
		int(150 * fade * fade)));
	glow.setColorAt(0.5, QColor(0xFF, 0xD5, 0x4E, int(70 * fade * fade)));
	glow.setColorAt(1., QColor(plus.red(), plus.green(), plus.blue(), 0));
	p.setPen(Qt::NoPen);
	p.setBrush(glow);
	p.drawEllipse(_origin, glowRadius, glowRadius);

	for (auto i = 0; i != 4; ++i) {
		const auto local = std::clamp(
			(progress - i * 0.11) / 0.75,
			0.,
			1.);
		if (local <= 0. || local >= 1.) {
			continue;
		}
		const auto ringEased = EaseOutCubic(local);
		const auto ringRadius = std::max(4., maxDim * 0.6 * ringEased);
		const auto ringAlpha = int(170 * (1. - local) * fade);
		p.setPen(QPen(
			i % 2 == 0
				? QColor(plus.red(), plus.green(), plus.blue(), ringAlpha)
				: QColor(0xFF, 0xD5, 0x4E, ringAlpha),
			0.5 + 3.0 * (1. - local)));
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(_origin, ringRadius, ringRadius);
	}

	const auto shock = std::clamp(progress / 0.5, 0., 1.);
	const auto shockRadius = std::max(8., maxDim * 0.75 * EaseOutCubic(shock));
	const auto shockAlpha = int(150 * (1. - shock) * fade);
	p.setPen(QPen(QColor(
		plus.red(),
		plus.green(),
		plus.blue(),
		shockAlpha), 1.5 + 7.0 * (1. - shock)));
	p.setBrush(Qt::NoBrush);
	p.drawEllipse(_origin, shockRadius, shockRadius);

	const auto count = 120;
	for (auto i = 0; i != count; ++i) {
		const auto angle = i * kGoldenAngle + _seed * 0.00001;
		const auto speed = 0.55 + (i % 7) * 0.11;
		const auto travelled = speed * eased * maxDim * 0.55;
		const auto drift = 0.35 * progress * progress * maxDim * 0.12;
		const auto pos = _origin + QPointF(
			cos(angle) * travelled,
			sin(angle) * travelled + drift);
		const auto alpha = int(255 * fade * fade * (0.55 + (i % 5) * 0.1));
		const auto gold = (i % 4 == 0);
		const auto color = gold
			? QColor(0xFF, 0xD5, 0x4E, alpha)
			: (i % 4 == 1)
			? QColor(0xFF, 0xFF, 0xFF, alpha)
			: QColor(plus.red(), plus.green(), plus.blue(), alpha);
		const auto size = 1.2 + (i % 3) * 0.8;
		if (gold) {
			auto tail = QLinearGradient(pos, _origin);
			const auto tailStart = std::min(0.9, 1. - 0.5 * progress);
			tail.setColorAt(tailStart, QColor(
				0xFF,
				0xD5,
				0x4E,
				int(alpha * 0.55)));
			tail.setColorAt(1., QColor(0xFF, 0xD5, 0x4E, 0));
			p.setPen(QPen(tail, size * 0.9));
			p.drawLine(pos, _origin);
		}
		if (i % 6 == 0) {
			PaintSparkle(p, pos, size * 2.4, color);
		} else {
			p.setPen(Qt::NoPen);
			p.setBrush(color);
			p.drawEllipse(pos, size, size);
		}
	}

	const auto coreAlpha = int(255 * (1. - EaseOutCubic(std::min(1., progress * 1.6))));
	if (coreAlpha > 0) {
		p.setPen(Qt::NoPen);
		p.setBrush(White(coreAlpha));
		p.drawEllipse(_origin, 5., 5.);
	}
}

GlassToggle::GlassToggle(QWidget *parent)
: Ui::RpWidget(parent) {
	_pill.setCallback([=] { update(); });
	resize(52, 102);
	setCursor(Qt::PointingHandCursor);
	_pillFrom = _pillTo = pillRect(0);
}

rpl::variable<int> &GlassToggle::selected() {
	return _value;
}

void GlassToggle::setScale(float64 scale) {
	scale = std::clamp(scale, 0.3, 1.);
	if (std::abs(scale - _scale) < 0.001) {
		return;
	}
	_scale = scale;
	resize(
		int(std::round(52 * _scale)),
		int(std::round(102 * _scale)));
	update();
}

void GlassToggle::setFade(float64 fade) {
	fade = std::clamp(fade, 0., 1.);
	if (std::abs(fade - _fade) < 0.01) {
		return;
	}
	_fade = fade;
	setAttribute(Qt::WA_TransparentForMouseEvents, _fade < 0.05);
	setCursor(_fade < 0.05 ? Qt::ArrowCursor : Qt::PointingHandCursor);
	update();
}

void GlassToggle::setSelected(int value, bool animated) {
	value = std::clamp(value, 0, 1);
	if (_value.current() == value) {
		return;
	}
	const auto progress = animated ? _pill.value(1.) : 1.;
	_pillFrom = InterpolateRect(_pillFrom, _pillTo, progress);
	_pillTo = pillRect(value);
	_value = value;
	if (animated) {
		_pill.stop();
		_pill.start(
			[=] { update(); },
			0.,
			1.,
			st::slideDuration,
			anim::easeOutBack);
		_pop.stop();
		_pop.start([=] { update(); }, 0., 1., 380, anim::sineInOut);
	} else {
		_pillFrom = _pillTo;
		_pill.stop();
		_pop.stop();
		update();
	}
}

QRectF GlassToggle::toggleRect() const {
	return QRectF(5., 5., 52. - 10., 102. - 10.);
}

QPointF GlassToggle::buttonCenterDesign(int value) const {
	const auto r = toggleRect();
	const auto diameter = 40.;
	const auto dx = (52. - diameter) / 2.;
	const auto firstY = r.top() + 20.;
	const auto secondY = firstY + 40. + 10.;
	return QPointF(dx + diameter / 2., (value == 0) ? firstY : secondY);
}

QPointF GlassToggle::buttonCenter(int value) const {
	return buttonCenterDesign(value) * _scale;
}

QRectF GlassToggle::pillRect(int value) const {
	const auto center = buttonCenterDesign(value);
	return QRectF(
		center - QPointF(23., 23.),
		QSizeF(46., 46.));
}

int GlassToggle::valueAt(const QPoint &position) const {
	if (_fade < 0.05) {
		return -1;
	}
	const auto local = QPointF(position) / _scale;
	for (auto i = 0; i != 2; ++i) {
		const auto center = buttonCenterDesign(i);
		const auto dx = local.x() - center.x();
		const auto dy = local.y() - center.y();
		if (dx * dx + dy * dy <= 26. * 26.) {
			return i;
		}
	}
	return -1;
}

void GlassToggle::paintEvent(QPaintEvent *) {
	if (_fade < 0.01) {
		return;
	}
	QPainter p(this);
	auto hq = PainterHighQualityEnabler(p);
	p.setOpacity(p.opacity() * _fade);
	if (std::abs(_scale - 1.) > 0.001) {
		p.scale(_scale, _scale);
	}

	const auto r = toggleRect();
	auto path = QPainterPath();
	path.addRoundedRect(r, 22., 22.);
	auto bg = QLinearGradient(r.topLeft(), r.bottomLeft());
	bg.setColorAt(0., White(56));
	bg.setColorAt(0.5, White(22));
	bg.setColorAt(1., White(34));
	p.fillPath(path, bg);
	p.setPen(QPen(White(72), 1.));
	p.drawPath(path);

	auto shine = QLinearGradient(r.topLeft(), QPointF(r.left(), r.top() + 14));
	shine.setColorAt(0., White(66));
	shine.setColorAt(1., White(0));
	p.setPen(Qt::NoPen);
	p.setBrush(shine);
	p.drawRoundedRect(QRectF(r.left() + 3, r.top() + 3, r.width() - 6, 12), 8., 8.);

	const auto pill = InterpolateRect(_pillFrom, _pillTo, _pill.value(1.));
	if (!pill.isEmpty()) {
		Dialogs::LiquidGlass::PaintLiquidGlassPill(
			p,
			pill.toRect(),
			int(pill.height() / 2.));
	}

	for (auto i = 0; i != 2; ++i) {
		const auto center = buttonCenterDesign(i);
		paintRoundIcon(p, i, QRectF(center - QPointF(20., 20.), QSizeF(40., 40.)));
	}
}

void GlassToggle::paintRoundIcon(QPainter &p, int value, const QRectF &r) {
	auto hq = PainterHighQualityEnabler(p);
	const auto selected = (_value.current() == value);
	const auto center = r.center();
	const auto pop = _pop.value(1.);
	const auto popScale = 1. + 0.16 * std::sin(kPi * pop);

	QRectF rr = r;
	if (selected && popScale > 1.0001) {
		rr.setSize(r.size() * popScale);
		rr.moveCenter(center);
	}

	if (selected) {
		p.setPen(QPen(PlusColor(), 2.));
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(rr.adjusted(1., 1., -1., -1.));
	} else {
		p.setPen(QPen(White(56), 1.));
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(rr);
	}

	if (selected && pop > 0. && pop < 1.) {
		const auto flash = White(int(190 * (1. - pop)));
		p.setPen(QPen(flash, 2.));
		p.setBrush(Qt::NoBrush);
		const auto flashRect = r.adjusted(
			-int(4. * pop), -int(4. * pop), int(4. * pop), int(4. * pop));
		p.drawEllipse(flashRect);
	}

	const auto iconRect = rr.adjusted(9., 9., -9., -9.);
	if (value == 0) {
		p.save();
		p.setOpacity(selected ? 1. : 0.75);
		PaintTelegramPlane(p, iconRect, selected ? White(255) : White(210));
		p.restore();
	} else {
		auto logo = AyuAssets::currentAppLogo();
		if (logo.isNull()) {
			logo = AyuAssets::currentAppLogoPad();
		}
		if (!logo.isNull()) {
			const auto scaled = logo.scaled(
				iconRect.size().toSize(),
				Qt::KeepAspectRatio,
				Qt::SmoothTransformation);
			auto clip = QPainterPath();
			clip.addEllipse(iconRect);
			p.save();
			p.setClipPath(clip);
			p.drawImage(iconRect, scaled);
			p.restore();
		}
	}
}

void GlassToggle::mousePressEvent(QMouseEvent *e) {
	_press = valueAt(e->pos());
}

void GlassToggle::mouseReleaseEvent(QMouseEvent *e) {
	if (_press >= 0) {
		const auto now = valueAt(e->pos());
		if (now == _press && now >= 0 && now != _value.current()) {
			setSelected(now);
		}
	}
	_press = -1;
}

void FillDiveGramPlusPage(not_null<Ui::VerticalLayout*> container) {
	Ui::AddSkip(container, 10);

	container->add(
		object_ptr<Ui::FlatLabel>(
			container,
			tr::lng_divegram_plus_choose_tariff(),
			st::defaultSubsectionTitle),
		QMargins(24, 10, 24, 4));
	Ui::AddSkip(container, 4);

	auto state = std::make_shared<TariffState>();
	const auto list = Tariffs();
	state->rows.reserve(list.size());
	for (const auto &info : list) {
		auto widget = object_ptr<TariffRow>(container, info);
		const auto raw = widget.data();
		container->add(std::move(widget), QMargins(24, 0, 24, 10));
		raw->setClickedCallback([=, raw = raw] {
			const auto it = std::find(
				state->rows.begin(),
				state->rows.end(),
				raw);
			if (it == state->rows.end()) {
				return;
			}
			state->selected = int(it - state->rows.begin());
			for (auto i = 0; i != int(state->rows.size()); ++i) {
				state->rows[i]->setSelected(int(i) == state->selected);
			}
		});
		state->rows.push_back(raw);
	}
	if (!state->rows.empty()) {
		state->rows[0]->setSelected(true);
	}

	Ui::AddSkip(container, 2);
	Ui::AddDividerText(
		container,
		tr::lng_divegram_plus_funds_note());
	Ui::AddSkip(container, 8);

	Ui::AddDividerText(container, tr::lng_divegram_plus_what_includes());
	Ui::AddSkip(container, 6);

	struct Perk {
		rpl::producer<QString> title;
		rpl::producer<QString> about;
	};
	const auto perks = std::vector<Perk>{
		{
			tr::lng_divegram_plus_perk_badge(),
			tr::lng_divegram_plus_perk_badge_about(),
		},
		{
			tr::lng_divegram_plus_perk_channel(),
			tr::lng_divegram_plus_perk_channel_about(),
		},
		{
			tr::lng_divegram_plus_perk_priority(),
			tr::lng_divegram_plus_perk_priority_about(),
		},
		{
			tr::lng_divegram_plus_perk_site(),
			tr::lng_divegram_plus_perk_site_about(),
		},
	};
	for (auto perk : perks) {
		const auto title = container->add(
			object_ptr<Ui::FlatLabel>(
				container,
				std::move(perk.title),
				st::defaultFlatLabel),
			QMargins(24, 6, 24, 2));
		title->setAttribute(Qt::WA_TransparentForMouseEvents);
		container->add(
			object_ptr<Ui::FlatLabel>(
				container,
				std::move(perk.about),
				st::boxDividerLabel),
			QMargins(24, 0, 24, 10));
	}

	Ui::AddSkip(container, 4);
}

} // namespace Divegram::UiPlus