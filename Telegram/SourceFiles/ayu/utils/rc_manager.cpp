// This is the source code of DiveGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/utils/rc_manager.h"

#include <algorithm>

#include <QJsonArray>
#include <qjsondocument.h>
#include <QTimer>

namespace {

constexpr auto kPrimaryUrl = "https://update.ayugram.one/rc/current/desktop2";
constexpr auto kExteraUrl = "https://api.exteragram.app/api/v1/profiles/compact";
constexpr auto kFetchTimeout = 15 * 1000;

}

std::unordered_set<ID> default_developers = {
	139303278, 168769611, 668557709, 880708503, 963080346, 1156270028, 1282540315, 1348136086, 1374434073, 1752394339,
	1773117711, 2135966128, 5079320635, 5118627360, 5184725450, 5330087923, 5800413909, 6007644928, 7380551229,
	7738913005, 7818249287, 8083933640, 8512951856
};

std::unordered_set<ID> default_channels = {
	1172503281, 1434550607, 1524581881, 1559501352, 1571726392, 1632728092, 1725670701, 1754537498, 1794457129,
	1815864846, 1877362358, 1905581924, 1947958814, 1976430343, 2130395384, 2331068091, 2401498637, 2562664432,
	2564770112, 2685666919, 3116497667, 3212977677, 3572293253
};

void RCManager::start() {
	DEBUG_LOG(("RCManager: starting"));
	_manager = std::make_unique<QNetworkAccessManager>();

	makeRequest();

	_timer = new QTimer(this);
	connect(_timer, &QTimer::timeout, this, &RCManager::makeRequest);
	_timer->start(60 * 60 * 1000); // 1 hour
}

void RCManager::makeRequest() {
	_retryAttempted = false;
	sendRequest();
}

void RCManager::sendRequest() {
	if (!_manager) {
		return;
	}

	const auto url = QString::fromLatin1(_useExteraFallback ? kExteraUrl : kPrimaryUrl);
	LOG(("RCManager: requesting map"));

	clearSentRequest();

	auto request = QNetworkRequest(QUrl(url));
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setTransferTimeout(kFetchTimeout);
	_reply = _manager->get(request);
	connect(_reply,
			&QNetworkReply::finished,
			[=]
			{
				gotResponse();
			});
	connect(_reply,
			&QNetworkReply::errorOccurred,
			[=](auto e)
			{
				gotFailure(e);
			});
}

bool RCManager::tryRetryWithExteraFallback() {
	if (_retryAttempted || _useExteraFallback) {
		return false;
	}
	LOG(("RCManager: switching to extera fallback endpoint"));
	_useExteraFallback = true;
	_retryAttempted = true;
	sendRequest();
	return true;
}

void RCManager::gotResponse() {
	if (!_reply) {
		return;
	}

	const auto response = _reply->readAll();
	clearSentRequest();

	if (!handleResponse(response)) {
		LOG(("RCManager: Error bad map size: %1").arg(response.size()));
		gotFailure(QNetworkReply::UnknownContentError);
	}
}

bool RCManager::handleResponse(const QByteArray &response) {
	try {
		return applyResponse(response);
	} catch (...) {
		LOG(("RCManager: Failed to apply response"));
		return false;
	}
}

bool RCManager::applyResponse(const QByteArray &response) {
	auto error = QJsonParseError{0, QJsonParseError::NoError};
	const auto document = QJsonDocument::fromJson(response, &error);
	if (error.error != QJsonParseError::NoError) {
		LOG(("RCManager: Failed to parse JSON, error: %1"
		).arg(error.errorString()));
		return false;
	}
	if (!document.isObject()) {
		LOG(("RCManager: not an object received in JSON"));
		return false;
	}
	const auto root = document.object();

	const auto developers = root.value("developers").toArray();
	const auto officialChannels = root.value("officialChannels").toArray();
	const auto supporters = root.value("supporters").toArray();
	const auto supporterChannels = root.value("supporterChannels").toArray();
	const auto customBadges = root.value("customBadges").toArray();

	// Parse into locals first: a truncated or schema-changed payload must not
	// wipe the compiled-in defaults while it is still being validated.
	auto parsedDevelopers = decltype(_developers){};
	auto parsedOfficialChannels = decltype(_officialChannels){};
	auto parsedSupporters = decltype(_supporters){};
	auto parsedSupporterChannels = decltype(_supporterChannels){};
	auto parsedCustomBadges = decltype(_customBadges){};

	for (const auto &developer : developers) {
		if (const auto id = developer.toVariant().toLongLong()) {
			parsedDevelopers.insert(id);
		}
	}

	for (const auto &channel : officialChannels) {
		if (const auto id = channel.toVariant().toLongLong()) {
			parsedOfficialChannels.insert(id);
		}
	}

	for (const auto &supporter : supporters) {
		if (const auto id = supporter.toVariant().toLongLong()) {
			parsedSupporters.insert(id);
		}
	}

	for (const auto &channel : supporterChannels) {
		if (const auto id = channel.toVariant().toLongLong()) {
			parsedSupporterChannels.insert(id);
		}
	}

	for (const auto &badge : customBadges) {
		if (!badge.isObject()) {
			continue;
		}
		const auto obj = badge.toObject();
		const auto id = obj.value("id").toVariant().toLongLong();
		if (!id) {
			continue;
		}
		const auto badgeObj = obj.value("badge");
		if (!badgeObj.isObject()) {
			continue;
		}
		const auto badgeData = badgeObj.toObject();
		CustomBadge customBadge;
		if (const auto emojiStatusId = badgeData.value("documentId").toVariant().toLongLong()) {
			customBadge.emojiStatusId = EmojiStatusId(emojiStatusId);
		} else {
			continue;
		}
		if (const auto text = badgeData.value("text").toString(); !text.isEmpty()) {
			customBadge.text = text;
		}
		parsedCustomBadges[id] = customBadge;
	}

	if (const auto donateUsername = root.value("donateUsername"); donateUsername.isString()) {
		if (auto value = donateUsername.toString(); !value.isEmpty()) {
			// The value is remote-controlled and ends up as a clickable
			// t.me link in the support box, so accept only a plain username
			// instead of trusting the payload shape.
			if (value.startsWith('@')) {
				value.remove(0, 1);
			}
			const auto valid = !value.isEmpty()
				&& (value.size() <= 64)
				&& std::ranges::all_of(value, [](const QChar c) {
					return c.isLetterOrNumber() || (c == u'_');
				});
			if (valid) {
				_donateUsername = '@' + value;
			} else {
				LOG(("RCManager: ignoring invalid donateUsername"));
			}
		}
	}
	if (const auto donateAmountUsd = root.value("donateAmountUsd"); donateAmountUsd.isString()) {
		if (const auto value = donateAmountUsd.toString(); !value.isEmpty()) {
			_donateAmountUsd = value;
		}
	}
	if (const auto donateAmountTon = root.value("donateAmountTon"); donateAmountTon.isString()) {
		if (const auto value = donateAmountTon.toString(); !value.isEmpty()) {
			_donateAmountTon = value;
		}
	}
	if (const auto donateAmountRub = root.value("donateAmountRub"); donateAmountRub.isString()) {
		if (const auto value = donateAmountRub.toString(); !value.isEmpty()) {
			_donateAmountRub = value;
		}
	}

	// Only adopt the lists when the payload actually carries something. An
	// empty object is a truncated or schema-changed answer, not a request to
	// drop every official badge.
	if (parsedDevelopers.empty() && parsedOfficialChannels.empty()
		&& parsedSupporters.empty() && parsedSupporterChannels.empty()
		&& parsedCustomBadges.empty()) {
		LOG(("RCManager: response carried no usable entries, keeping defaults"));
		return false;
	}

	_developers = std::move(parsedDevelopers);
	_officialChannels = std::move(parsedOfficialChannels);
	_supporters = std::move(parsedSupporters);
	_supporterChannels = std::move(parsedSupporterChannels);
	_customBadges = std::move(parsedCustomBadges);

	initialized = true;

	LOG(("RCManager: Loaded %1 developers, %2 official channels"
	).arg(_developers.size()).arg(_officialChannels.size()));

	return true;
}

void RCManager::gotFailure(QNetworkReply::NetworkError e) {
	LOG(("RCManager: Error %1").arg(e));
	if (tryRetryWithExteraFallback()) {
		LOG(("RCManager: retrying request with extera fallback endpoint"));
		return;
	}
	LOG(("RCManager: no retry left for failed request"));
	if (const auto reply = base::take(_reply)) {
		reply->deleteLater();
	}
}

void RCManager::clearSentRequest() {
	const auto reply = base::take(_reply);
	if (!reply) {
		return;
	}
	disconnect(reply, &QNetworkReply::finished, nullptr, nullptr);
	disconnect(reply, &QNetworkReply::errorOccurred, nullptr, nullptr);
	reply->abort();
	reply->deleteLater();
}

RCManager::~RCManager() {
	clearSentRequest();
	_manager = nullptr;
}
