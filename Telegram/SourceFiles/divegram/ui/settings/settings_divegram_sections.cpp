#include "divegram/ui/settings/settings_divegram_sections.h"

#include "divegram/ui/settings/settings_divegram.h"
#include "ayu/ayu_settings.h"
#include "ayu/ui/settings/settings_ayu_utils.h"
#include "base/variant.h"
#include "core/version.h"
#include "lang_auto.h"
#include "main/main_session.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/vertical_list.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;

namespace {

void AddDescription(
		not_null<Ui::VerticalLayout*> container,
		const QString &text) {
	container->add(
		object_ptr<Ui::FlatLabel>(
			container,
			rpl::single(text),
			st::boxDividerLabel),
		st::defaultBoxDividerLabelPadding);
}

not_null<Button*> AddSettingToggleWithDescription(
		not_null<Ui::VerticalLayout*> container,
		rpl::producer<QString> text,
		const QString &description,
		BoolGetter getter,
		BoolSetter setter) {
	const auto result = AddSettingToggle(container, std::move(text), getter, setter);
	AddDescription(container, description);
	return result;
}

not_null<Button*> AddToggleWithDescription(
		not_null<Ui::VerticalLayout*> container,
		rpl::producer<QString> text,
		const QString &description,
		Fn<bool()> getter,
		Fn<void(bool)> setter) {
	const auto result = AddToggle(container, std::move(text), getter, setter);
	AddDescription(container, description);
	return result;
}

const auto kGeneralMeta = BuildHelper({
	.id = DiveGramGeneral::Id(),
	.parentId = DiveGramMain::Id(),
	.title = &tr::lng_divegram_section_general,
	.icon = &st::menuIconShowAll,
}, [](SectionBuilder &builder) {
	builder.addSkip();

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				tr::lng_divegram_general_about(),
				st::boxLabel),
			.align = style::al_top,
		};
	});

	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.add([](const BuildContext &ctx) {
		v::match(ctx, [](const SearchContext &) {
		}, [&](const WidgetContext &wctx) {
			const auto container = wctx.container;
			const auto session = &wctx.controller->session();

			AddSubsectionTitle(container, tr::lng_divegram_ghost_mode());

			auto &ghost = AyuSettings::ghost(session);

			AddToggleWithDescription(
				container,
				tr::lng_divegram_ghost_active(),
				tr::lng_divegram_ghost_active_about(tr::now),
				[&ghost] { return ghost.isGhostModeActive(); },
				[&ghost](bool v) { ghost.setGhostModeEnabled(v); });

			AddCollapsibleToggle(
				container,
				tr::lng_divegram_hide_activity(),
				std::vector<NestedEntry>{
					NestedEntry{
						tr::lng_divegram_hide_read_receipts(tr::now),
						[&ghost] { return !ghost.sendReadMessages(); },
						[&ghost](bool v) { ghost.setSendReadMessages(!v); },
						[&ghost] { return ghost.sendReadMessagesLocked(); },
						[&ghost](bool v) { ghost.setSendReadMessagesLocked(v); },
					},
					NestedEntry{
						tr::lng_divegram_hide_read_stories(tr::now),
						[&ghost] { return !ghost.sendReadStories(); },
						[&ghost](bool v) { ghost.setSendReadStories(!v); },
						[&ghost] { return ghost.sendReadStoriesLocked(); },
						[&ghost](bool v) { ghost.setSendReadStoriesLocked(v); },
					},
					NestedEntry{
						tr::lng_divegram_hide_online(tr::now),
						[&ghost] { return !ghost.sendOnlinePackets(); },
						[&ghost](bool v) { ghost.setSendOnlinePackets(!v); },
						[&ghost] { return ghost.sendOnlinePacketsLocked(); },
						[&ghost](bool v) { ghost.setSendOnlinePacketsLocked(v); },
					},
					NestedEntry{
						tr::lng_divegram_hide_upload(tr::now),
						[&ghost] { return !ghost.sendUploadProgress(); },
						[&ghost](bool v) { ghost.setSendUploadProgress(!v); },
						[&ghost] { return ghost.sendUploadProgressLocked(); },
						[&ghost](bool v) { ghost.setSendUploadProgressLocked(v); },
					},
					NestedEntry{
						tr::lng_divegram_offline_packet(tr::now),
						[&ghost] { return ghost.sendOfflinePacketAfterOnline(); },
						[&ghost](bool v) { ghost.setSendOfflinePacketAfterOnline(v); },
						[&ghost] { return ghost.sendOfflinePacketAfterOnlineLocked(); },
						[&ghost](bool v) { ghost.setSendOfflinePacketAfterOnlineLocked(v); },
					},
				},
				true,
				tr::lng_divegram_hide_activity_about(tr::now));

			AddToggleWithDescription(
				container,
				tr::lng_divegram_mark_read_actions(),
				tr::lng_divegram_mark_read_actions_about(tr::now),
				[&ghost] { return ghost.markReadAfterAction(); },
				[&ghost](bool v) { ghost.setMarkReadAfterAction(v); });

			AddToggleWithDescription(
				container,
				tr::lng_divegram_ghost_stories(),
				tr::lng_divegram_ghost_stories_about(tr::now),
				[&ghost] { return ghost.suggestGhostModeBeforeViewingStory(); },
				[&ghost](bool v) { ghost.setSuggestGhostModeBeforeViewingStory(v); });

			AddSkip(container);
			AddDivider(container);
			AddSkip(container);

			AddSubsectionTitle(container, tr::lng_divegram_anti_recall());

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_save_deleted(),
				tr::lng_divegram_save_deleted_about(tr::now),
				&AyuSettings::saveDeletedMessages,
				&AyuSettings::setSaveDeletedMessages);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_semi_transparent(),
				tr::lng_divegram_semi_transparent_about(tr::now),
				&AyuSettings::semiTransparentDeletedMessages,
				&AyuSettings::setSemiTransparentDeletedMessages);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_store_history(),
				tr::lng_divegram_store_history_about(tr::now),
				&AyuSettings::saveMessagesHistory,
				&AyuSettings::setSaveMessagesHistory);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_save_bots(),
				tr::lng_divegram_save_bots_about(tr::now),
				&AyuSettings::saveForBots,
				&AyuSettings::setSaveForBots);
		});
	});
});

} // namespace

rpl::producer<QString> DiveGramGeneral::title() {
	return tr::lng_divegram_section_general();
}

DiveGramGeneral::DiveGramGeneral(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void DiveGramGeneral::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kGeneralMeta.build);
	Ui::ResizeFitChild(this, content);
}

namespace {

const auto kAppearanceMeta = BuildHelper({
	.id = DiveGramAppearance::Id(),
	.parentId = DiveGramMain::Id(),
	.title = &tr::lng_divegram_section_appearance,
	.icon = &st::menuIconPalette,
}, [](SectionBuilder &builder) {
	builder.addSkip();

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				tr::lng_divegram_appearance_about(),
				st::boxLabel),
			.align = style::al_top,
		};
	});

	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.add([](const BuildContext &ctx) {
		v::match(ctx, [](const SearchContext &) {
		}, [&](const WidgetContext &wctx) {
			const auto container = wctx.container;

			AddSubsectionTitle(container, tr::lng_divegram_interface());

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_streamer_mode(),
				tr::lng_divegram_streamer_mode_about(tr::now),
				&AyuSettings::streamerMode,
				&AyuSettings::setStreamerMode);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_material_switches(),
				tr::lng_divegram_material_switches_about(tr::now),
				&AyuSettings::materialSwitches,
				&AyuSettings::setMaterialSwitches);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_remove_tail(),
				tr::lng_divegram_remove_tail_about(tr::now),
				&AyuSettings::removeMessageTail,
				&AyuSettings::setRemoveMessageTail);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_simple_quotes(),
				tr::lng_divegram_simple_quotes_about(tr::now),
				&AyuSettings::simpleQuotesAndReplies,
				&AyuSettings::setSimpleQuotesAndReplies);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_info_icons(),
				tr::lng_divegram_info_icons_about(tr::now),
				&AyuSettings::replaceBottomInfoWithIcons,
				&AyuSettings::setReplaceBottomInfoWithIcons);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_unlimited_stickers(),
				tr::lng_divegram_unlimited_stickers_about(tr::now),
				&AyuSettings::unlimitedRecentStickers,
				&AyuSettings::setUnlimitedRecentStickers);

			AddDivider(container);
			AddSkip(container);

			AddSubsectionTitle(container, tr::lng_divegram_content());

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_filter_zalgo(),
				tr::lng_divegram_filter_zalgo_about(tr::now),
				&AyuSettings::filterZalgo,
				&AyuSettings::setFilterZalgo);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_disable_backgrounds(),
				tr::lng_divegram_disable_backgrounds_about(tr::now),
				&AyuSettings::disableCustomBackgrounds,
				&AyuSettings::setDisableCustomBackgrounds);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_disable_ads(),
				tr::lng_divegram_disable_ads_about(tr::now),
				&AyuSettings::disableAds,
				&AyuSettings::setDisableAds);
		});
	});
});

} // namespace

rpl::producer<QString> DiveGramAppearance::title() {
	return tr::lng_divegram_section_appearance();
}

DiveGramAppearance::DiveGramAppearance(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void DiveGramAppearance::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kAppearanceMeta.build);
	Ui::ResizeFitChild(this, content);
}

namespace {

const auto kPrivacyMeta = BuildHelper({
	.id = DiveGramPrivacy::Id(),
	.parentId = DiveGramMain::Id(),
	.title = &tr::lng_divegram_section_privacy,
	.icon = &st::menuIconLock,
}, [](SectionBuilder &builder) {
	builder.addSkip();

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				tr::lng_divegram_privacy_about(),
				st::boxLabel),
			.align = style::al_top,
		};
	});

	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.add([](const BuildContext &ctx) {
		v::match(ctx, [](const SearchContext &) {
		}, [&](const WidgetContext &wctx) {
			const auto container = wctx.container;

			AddSubsectionTitle(container, tr::lng_divegram_privacy());

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_hide_unread(),
				tr::lng_divegram_hide_unread_about(tr::now),
				&AyuSettings::hideNotificationCounters,
				&AyuSettings::setHideNotificationCounters);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_hide_badges(),
				tr::lng_divegram_hide_badges_about(tr::now),
				&AyuSettings::hideNotificationBadge,
				&AyuSettings::setHideNotificationBadge);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_hide_stories(),
				tr::lng_divegram_hide_stories_about(tr::now),
				&AyuSettings::disableStories,
				&AyuSettings::setDisableStories);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_hide_premium(),
				tr::lng_divegram_hide_premium_about(tr::now),
				&AyuSettings::hidePremiumStatuses,
				&AyuSettings::setHidePremiumStatuses);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_hide_blocked(),
				tr::lng_divegram_hide_blocked_about(tr::now),
				&AyuSettings::hideFromBlocked,
				&AyuSettings::setHideFromBlocked);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_hide_all_chats(),
				tr::lng_divegram_hide_all_chats_about(tr::now),
				&AyuSettings::hideAllChatsFolder,
				&AyuSettings::setHideAllChatsFolder);

			AddDivider(container);
			AddSkip(container);

			AddSubsectionTitle(container, tr::lng_divegram_system());

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_filters_enabled(),
				tr::lng_divegram_filters_enabled_about(tr::now),
				&AyuSettings::filtersEnabled,
				&AyuSettings::setFiltersEnabled);

			AddSettingToggleWithDescription(
				container,
				tr::lng_divegram_crash_reports(),
				tr::lng_divegram_crash_reports_about(tr::now),
				&AyuSettings::crashReporting,
				&AyuSettings::setCrashReporting);
		});
	});
});

} // namespace

rpl::producer<QString> DiveGramPrivacy::title() {
	return tr::lng_divegram_section_privacy();
}

DiveGramPrivacy::DiveGramPrivacy(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void DiveGramPrivacy::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kPrivacyMeta.build);
	Ui::ResizeFitChild(this, content);
}

} // namespace Settings
