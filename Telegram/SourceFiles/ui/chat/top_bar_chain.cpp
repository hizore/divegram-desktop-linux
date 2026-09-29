/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging system.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/chat/top_bar_chain.h"

#include <rpl/variable.h>

#include "styles/style_window.h"

namespace Ui::Chat {

namespace {

rpl::variable<bool> &ChainState() {
	// Intentionally leaked, so that late subscribers never touch
	// a destroyed variable.
	static auto *state = new rpl::variable<bool>(false);
	return *state;
}

} // namespace

rpl::producer<bool> TopBarChain() {
	return ChainState().value();
}

void SetTopBarChain(bool enabled) {
	ChainState() = enabled;
}

bool TopBarChainEnabled() {
	return ChainState().current();
}

QColor TopBarGlassTint() {
	const auto base = st::windowBg->c;
	return QColor(base.red(), base.green(), base.blue(), 214);
}

} // namespace Ui::Chat
