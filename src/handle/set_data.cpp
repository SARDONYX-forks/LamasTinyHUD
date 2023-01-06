﻿#include "set_data.h"
#include "key_position.h"
#include "page_handle.h"
#include "data/data_helper.h"
#include "setting/mcm_setting.h"
#include "util/constant.h"
#include "util/string_util.h"

namespace handle {
    using mcm = config::mcm_setting;

    void set_data::set_slot_data() {
        logger::trace("Setting handlers ..."sv);

        auto key_pos = key_position::get_singleton();
        key_pos->init_key_position_map();

        //set empty for each position, it will be overwritten if it is configured
        for (auto i = 0; i < static_cast<int>(page_setting::position::total); ++i) {
            set_empty_slot(i, key_pos);
        }

        logger::trace("continue with overwriting data from configuration ..."sv);

        set_slot(page_setting::position::top,
            mcm::get_top_selected_item_form(),
            mcm::get_top_type(),
            mcm::get_top_hand_selection(),
            mcm::get_top_slot_action(),
            mcm::get_top_selected_item_form_left(),
            mcm::get_top_type_left(),
            mcm::get_top_slot_action_left(),
            key_pos);

        set_slot(page_setting::position::right,
            mcm::get_right_selected_item_form(),
            mcm::get_right_type(),
            mcm::get_right_hand_selection(),
            mcm::get_right_slot_action(),
            mcm::get_right_selected_item_form_left(),
            mcm::get_right_type_left(),
            mcm::get_right_slot_action_left(),
            key_pos);

        set_slot(page_setting::position::bottom,
            mcm::get_bottom_selected_item_form(),
            mcm::get_bottom_type(),
            mcm::get_bottom_hand_selection(),
            mcm::get_bottom_slot_action(),
            mcm::get_bottom_selected_item_form_left(),
            mcm::get_bottom_type_left(),
            mcm::get_bottom_slot_action_left(),
            key_pos);

        set_slot(page_setting::position::left,
            mcm::get_left_selected_item_form(),
            mcm::get_left_type(),
            mcm::get_left_hand_selection(),
            mcm::get_left_slot_action(),
            mcm::get_left_selected_item_form_left(),
            mcm::get_left_type_left(),
            mcm::get_left_slot_action_left(),
            key_pos);

        logger::trace("done setting. return."sv);
    }

    void set_data::set_empty_slot(int a_pos, key_position*& a_key_pos) {
        logger::trace("setting empty config for position {}"sv, a_pos);
        std::vector<data_helper*> data;
        const auto item = new data_helper();
        item->form = nullptr;
        item->action_type = slot_setting::acton_type::default_action;
        item->type = util::selection_type::unset;
        data.push_back(item);

        page_handle::get_singleton()->init_page(0,
            static_cast<page_setting::position>(a_pos),
            data,
            mcm::get_hud_slot_position_offset(),
            mcm::get_hud_key_position_offset(),
            slot_setting::hand_equip::total,
            config::mcm_setting::get_icon_opacity(),
            a_key_pos);
    }

    void set_data::set_slot(page_setting::position a_pos,
        const uint32_t a_form,
        uint32_t a_type,
        uint32_t a_hand,
        uint32_t a_action,
        const uint32_t a_form_left,
        uint32_t a_type_left,
        const uint32_t a_action_left,
        key_position*& a_key_pos) {
        const auto form = RE::TESForm::LookupByID(a_form);
        const auto form_left = RE::TESForm::LookupByID(a_form_left);
        if (form == nullptr && form_left) return;

        auto hand = static_cast<slot_setting::hand_equip>(a_hand);
        std::vector<data_helper*> data;

        logger::trace("start working data hands {} ..."sv, a_hand);


        slot_setting::acton_type action;
        if (a_action == a_action_left) {
            action = static_cast<slot_setting::acton_type>(a_action);
        } else {
            action = slot_setting::acton_type::default_action;
            logger::warn("action type {} differ from action type left {}, setting both to {}"sv,
                a_action,
                a_action_left,
                static_cast<uint32_t>(action));
        }


        if (form != nullptr) {
            const auto type = static_cast<util::selection_type>(a_type);

            if (type != util::selection_type::magic && type != util::selection_type::weapon && type !=
                util::selection_type::shield) {
                hand = slot_setting::hand_equip::total;
            }

            if (type == util::selection_type::shield) {
                logger::warn("Equipping shield on the Right hand might fail, or hand will be empty"sv);
            }

            logger::trace("start building data pos {}, form {}, type {}, action {}, hand {}"sv,
                static_cast<uint32_t>(a_pos),
                util::string_util::int_to_hex(form),
                static_cast<int>(type),
                static_cast<uint32_t>(action),
                static_cast<uint32_t>(hand));

            const auto item = new data_helper();
            item->form = form;
            item->type = type;
            item->action_type = action;
            item->left = false;
            data.push_back(item);
        }


        logger::trace("checking if we need to build a second data set, already got {}"sv, data.size());

        if (const auto type_left = static_cast<util::selection_type>(a_type_left);
            (type_left == util::selection_type::magic || type_left == util::selection_type::weapon || type_left ==
             util::selection_type::shield) && hand == slot_setting::hand_equip::single) {
            logger::trace("start building data pos {}, form {}, type {}, action {}, hand {}"sv,
                static_cast<uint32_t>(a_pos),
                util::string_util::int_to_hex(form_left),
                static_cast<int>(type_left),
                static_cast<uint32_t>(action),
                static_cast<uint32_t>(hand));

            const auto item_left = new data_helper();
            item_left->form = form_left;
            item_left->type = type_left;
            item_left->action_type = action;
            item_left->left = true;
            data.push_back(item_left);
        }
        logger::trace("build data, calling handler, data size {}"sv, data.size());

        if (!data.empty()) {
            page_handle::get_singleton()->init_page(0,
                a_pos,
                data,
                config::mcm_setting::get_hud_slot_position_offset(),
                config::mcm_setting::get_hud_key_position_offset(),
                hand,
                config::mcm_setting::get_icon_opacity(),
                a_key_pos);
        }
    }
}
