﻿#include "equip_event.h"
#include "handle/handle/edit_handle.h"
#include "handle/handle/key_position_handle.h"
#include "handle/handle/name_handle.h"
#include "handle/handle/page_handle.h"
#include "setting/custom_setting.h"
#include "setting/mcm_setting.h"
#include "util/helper.h"
#include "util/string_util.h"

namespace event {
    equip_event* equip_event::get_singleton() {
        static equip_event singleton;
        return std::addressof(singleton);
    }

    void equip_event::sink() {
        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(get_singleton());
    }

    equip_event::event_result equip_event::ProcessEvent(const RE::TESEquipEvent* a_event,
        [[maybe_unused]] RE::BSTEventSource<RE::TESEquipEvent>* a_event_source) {
        if (!a_event || !a_event->actor || !a_event->actor->IsPlayerRef()) {
            return event_result::kContinue;
        }

        if (config::mcm_setting::get_draw_current_items_text()) {
            handle::name_handle::get_singleton()->init_names(util::helper::get_hand_assignment());
        }


        auto form = RE::TESForm::LookupByID(a_event->baseObject);

        if (!form) {
            return event_result::kContinue;
        }

        //add check if we need to block left
        if (config::mcm_setting::get_elder_demon_souls() && util::helper::is_two_handed(form)) {
            //is two handed, if equipped
            //hardcode left for now, cause we just need it there
            const auto key_handle = handle::key_position_handle::get_singleton();
            key_handle->set_position_lock(handle::position_setting::position_type::left, a_event->equipped ? 1 : 0);
            const auto page_handle = handle::page_handle::get_singleton();
            const auto page = page_handle->get_active_page_id_position(handle::position_setting::position_type::left);
            const auto setting = page_handle->get_page_setting(page, handle::position_setting::position_type::left);
            //use settings here
            setting->draw_setting->icon_transparency = a_event->equipped ?
                                                           config::mcm_setting::get_icon_transparency_blocked() :
                                                           config::mcm_setting::get_icon_transparency();
        }

        if (handle::edit_handle::get_singleton()->get_position() == handle::position_setting::position_type::total) {
            return event_result::kContinue;
        }

        if (const auto edit_handle = handle::edit_handle::get_singleton();
            edit_handle->get_position() != handle::position_setting::position_type::total &&
            config::mcm_setting::get_elder_demon_souls() && a_event->equipped) {
            data_ = edit_handle->get_hold_data();
            const auto item = util::helper::is_suitable_for_position(form, edit_handle->get_position());
            if (item->form) {
                data_.push_back(item);
                util::helper::write_notification(fmt::format("Added Item {}", form ? form->GetName() : "null"));
            } else {
                util::helper::write_notification(fmt::format("Ignored Item {}, because it did not fit the requirement",
                    form ? form->GetName() : "null"));
            }

            const auto pos_max = handle::page_handle::get_singleton()->get_highest_page_id_position(
                edit_handle->get_position());
            auto max = config::mcm_setting::get_max_page_count() - 1; //we start at 0 so count -1
            logger::trace("Max for Position {} is {}, already set before edit {}"sv,
                static_cast<uint32_t>(edit_handle->get_position()),
                max,
                pos_max);
            if (pos_max != -1) {
                max = config::mcm_setting::get_max_page_count() - pos_max;
            }

            if (data_.size() == max || max == 0) {
                edit_handle->set_hold_data(data_);
                util::helper::write_notification(fmt::format("Max Amount of {} Reached, rest will be Ignored",
                    max));
            }
            if (data_.size() > max) {
                util::helper::write_notification(fmt::format("Ignored Item {}", form ? form->GetName() : "null"));
            }
            edit_handle->set_hold_data(data_);
            logger::trace("Size is {}"sv, data_.size());
            data_.clear();
        }

        //edit for elder demon souls
        //right and left just weapons, left only one handed, right both
        //buttom consumables, scrolls, and such
        //top shouts, powers
        if (const auto edit_handle = handle::edit_handle::get_singleton();
            edit_handle->get_position() != handle::position_setting::position_type::total && !
            config::mcm_setting::get_elder_demon_souls()) {
            data_.clear();
            logger::trace("Player {} {}"sv, a_event->equipped ? "equipped" : "unequipped", form->GetName());
            //always
            const auto type = util::helper::get_type(form);
            if (type == handle::slot_setting::slot_type::empty || type ==
                handle::slot_setting::slot_type::weapon || type ==
                handle::slot_setting::slot_type::magic || type == handle::slot_setting::slot_type::shield) {
                data_ = util::helper::get_hand_assignment(form);
            }
            //just if equipped
            if (a_event->equipped) {
                const auto item = new data_helper();
                //magic, weapon, shield handled outside
                switch (type) {
                    case handle::slot_setting::slot_type::empty:
                        item->form = nullptr;
                        item->type = type;
                        data_.push_back(item);
                        break;
                    case handle::slot_setting::slot_type::shout:
                    case handle::slot_setting::slot_type::power:
                    case handle::slot_setting::slot_type::consumable:
                    case handle::slot_setting::slot_type::armor:
                    case handle::slot_setting::slot_type::scroll:
                    case handle::slot_setting::slot_type::misc:
                        item->form = form;
                        item->type = type;
                        data_.push_back(item);
                        break;
                }

                if (type == handle::slot_setting::slot_type::consumable) {
                    const auto obj = form->As<RE::AlchemyItem>();
                    RE::PlayerCharacter::GetSingleton()->AddObjectToContainer(obj, nullptr, 1, nullptr);
                }
            }
            edit_handle->set_hold_data(data_);
            data_.clear();
        }

        return event_result::kContinue;
    }
}
