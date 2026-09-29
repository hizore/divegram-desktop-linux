/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging system.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <rpl/variable.h>

#include <QtGui/QColor>

namespace Ui::Chat {

// Bars that are shown under the chat top bar (pinned messages, translate,
// contact status, group call, etc.) can be merged into a single glass
// column attached to the top bar pill. When it is enabled, those bars
// must not paint their own background or shadow: the column is painted
// by the history widget instead.
[[nodiscard]] rpl::producer<bool> TopBarChain();
void SetTopBarChain(bool enabled);
[[nodiscard]] bool TopBarChainEnabled();

// Tint painted under the glass effect of the top bar pill and of the
// chain, so that text stays readable over any chat background.
[[nodiscard]] QColor TopBarGlassTint();

} // namespace Ui::Chat
