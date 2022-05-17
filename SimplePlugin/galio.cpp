#include "galio.h"
#include "../plugin_sdk/plugin_sdk.hpp"

namespace galio
{
#define Q_DRAW_COLOR (MAKE_COLOR ( 62, 129, 237, 255 ))  //Red Green Blue Alpha
#define W_DRAW_COLOR (MAKE_COLOR ( 227, 203, 20, 255 ))  //Red Green Blue Alpha
#define E_DRAW_COLOR (MAKE_COLOR ( 235, 12, 223, 255 ))  //Red Green Blue Alpha
#define R_DRAW_COLOR (MAKE_COLOR ( 224, 77, 13, 255 ))   //Red Green Blue Alpha

#define DRAW_SAFE_ALLIES (MAKE_COLOR (0, 255, 0, 255))

	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;

#define GALIO_Q_RANGE 825
#define GALIO_W_RANGE 175
#define GALIO_W2_RANGE 350
#define GALIO_E_RANGE 650
#define GALIO_LVL1_R_RANGE 4000
#define GALIO_LVL2_R_RANGE 4750
#define GALIO_LVL3_R_RANGE 5500


	namespace draw_settings
	{
		TreeEntry* draw_range_q = nullptr;
		TreeEntry* draw_range_w = nullptr;
		TreeEntry* draw_range_e = nullptr;
		TreeEntry* draw_range_r = nullptr;

		TreeEntry* draw_safeable_allies = nullptr;
	}

	namespace misc_settings
	{
		TreeEntry* q_on_cc = nullptr;
		TreeEntry* e_gapclose = nullptr;
	}

	namespace farm_settings
	{
		TreeEntry* use_q = nullptr;
		TreeEntry* q_minion_hit = nullptr;
		TreeEntry* use_spells_mana = nullptr;
	}

	namespace harrass_settings
	{
		TreeEntry* use_q = nullptr;

		TreeEntry* use_spells_when_mana = nullptr;
	}

	namespace combo_settings
	{
		TreeEntry* use_q = nullptr;
		TreeEntry* use_w = nullptr;
		TreeEntry* use_e = nullptr;

		TreeEntry* range_slider = nullptr;

		TreeEntry* r_min_hp_percent = nullptr;

		TreeEntry* r_min_enemies = nullptr;

		TreeEntry* R_Key = nullptr;

		//whitelist?
		std::map<uint16_t, TreeEntry*> use_r_on;
	}

	TreeTab* main_tab = nullptr;

	game_object_script SelectedAlly = nullptr;

	void on_update()
	{
		if (myhero->is_dead())
		{
			return;
		}

		if (r->level() == 1 && r->range() != GALIO_LVL1_R_RANGE)
		{
			r->set_range(GALIO_LVL1_R_RANGE);
		}
		if (r->level() == 2 && r->range() != GALIO_LVL2_R_RANGE)
		{
			r->set_range(GALIO_LVL2_R_RANGE);
		}
		if (r->level() == 3 && r->range() != GALIO_LVL3_R_RANGE)
		{
			r->set_range(GALIO_LVL3_R_RANGE);
		}

		if (misc_settings::q_on_cc->get_bool() && q->is_ready())
		{
			auto target = target_selector->get_target(q, damage_type::magical);

			if (target != nullptr && target->get_distance(myhero) <= q->range())
			{
				if (target->has_buff_type({ buff_type::Slow,buff_type::Stun,buff_type::Knockup }))
				{
					q->cast(target);
					return;
				}
			}
		}

		if (misc_settings::e_gapclose->get_bool() && e->is_ready())
		{
			auto target = target_selector->get_target(e, damage_type::magical);

			if (target != nullptr && target->get_distance(myhero) <= e->range() - 100)
			{
				if (target->is_casting_interruptible_spell() >= 2 && !target->is_under_enemy_turret())
				{
					e->cast(target);
					return;
				}
			}
		}

		if (r->is_ready() && combo_settings::R_Key->get_bool())
		{
			if (SelectedAlly != nullptr && SelectedAlly->get_distance(myhero) <= r->range())
			{
				r->cast(SelectedAlly);
				return;
			}
		}

		if (!r->is_ready() && SelectedAlly != nullptr)
		{
			SelectedAlly = nullptr;
			return;
		}

		if (orbwalker->can_move(0.05f))
		{
			if (orbwalker->combo_mode())
			{
				if (q->is_ready() && combo_settings::use_q->get_bool())
				{
					auto target = target_selector->get_target(q, damage_type::magical);

					if (target != nullptr && target->get_distance(myhero) < q->range())
					{
						auto pred = q->get_prediction(target, false);

						if (pred.hitchance >= hit_chance::high)
						{
							q->cast(pred.get_cast_position());
							return;
						}
					}
				}

				if (w->is_ready() && combo_settings::use_w->get_bool())
				{
					auto target = target_selector->get_target(w, damage_type::magical);

					if (target != nullptr && target->get_distance(myhero) <= w->charged_min_range - 5)
					{
						w->start_charging();
						return;
					}
				}

				if (w->is_charging())
				{
					auto targ = target_selector->get_target(w, damage_type::magical);
					if (targ != nullptr && targ->get_distance(myhero) > w->range() - 5)
					{
						w->cast();
						return;
					}
				}

				if (w->charged_percentage() >= 95)
				{
					auto targ = target_selector->get_target(w, damage_type::magical);

					if (targ != nullptr && targ->get_distance(myhero) <= w->charged_max_range)
					{
						w->cast();
						return;
					}
				}

				if (e->is_ready() && combo_settings::use_e->get_bool())
				{
					auto target = target_selector->get_target(e, damage_type::magical);

					if (target != nullptr && target->get_distance(myhero) < e->range() - 50 && !target->is_under_enemy_turret())
					{
						e->cast(target->get_position());
						return;
					}
				}

				if (r->is_ready() && draw_settings::draw_safeable_allies->get_bool() && SelectedAlly == nullptr)
				{
					for (auto&& ally : entitylist->get_ally_heroes())
					{
						if (/*ally->is_valid_target(r->range()) && */ ally->get_id() != myhero->get_id())
						{
							if (combo_settings::use_r_on[ally->get_id()]->get_bool() && ally->get_distance(myhero) <= r->range())
							{
								if (ally->count_enemies_in_range(combo_settings::range_slider->get_int()) >= combo_settings::r_min_enemies->get_int()
									&& ally->get_health_percent() <= combo_settings::r_min_hp_percent->get_int())
								{
									SelectedAlly = ally;
									return;
								}
								else
								{
									SelectedAlly = nullptr;
									return;
								}
							}
						}
					}
				}
			}

			if (orbwalker->mixed_mode()) //harass
			{
				if (q->is_ready() && harrass_settings::use_q->get_bool() && myhero->get_mana_percent() > harrass_settings::use_spells_when_mana->get_int())
				{
					auto target = target_selector->get_target(q, damage_type::magical);

					if (target != nullptr && target->get_distance(myhero) < q->range())
					{
						auto pred = q->get_prediction(target, false);

						if (pred.hitchance >= hit_chance::high)
						{
							q->cast(pred.get_cast_position());
							return;
						}
					}
				}
			}

			if (orbwalker->lane_clear_mode())
			{
				if (farm_settings::use_q->get_bool() && myhero->get_mana_percent() > farm_settings::use_spells_mana->get_int())
				{
					auto laneMinions = entitylist->get_enemy_minions();
					laneMinions.erase(std::remove_if(laneMinions.begin(), laneMinions.end(), [](game_object_script x)
					{
						return !x->is_valid_target(q->range());
					}), laneMinions.end());

					//distance
					std::sort(laneMinions.begin(), laneMinions.end(), [](game_object_script a, game_object_script b)
					{
						return a->get_position().distance(myhero->get_position()) < b->get_position().distance(myhero->get_position());
					});

					if (!laneMinions.empty())
					{
						if (q->is_ready())
						{
							if (q->cast_on_best_farm_position(farm_settings::q_minion_hit->get_int()))
							{
								return;
							}
						}
					}
				}
			}
		}
	}

	void on_draw()
	{
		if (myhero->is_dead())
		{
			return;
		}

		if (q->is_ready() && draw_settings::draw_range_q->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), q->range(), Q_DRAW_COLOR);
		}

		if (w->is_ready() && draw_settings::draw_range_w->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), w->range(), W_DRAW_COLOR);
		}

		if (e->is_ready() && draw_settings::draw_range_e->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), e->range(), E_DRAW_COLOR);
		}

		if (r->is_ready() && draw_settings::draw_range_r->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), r->range(), R_DRAW_COLOR);
		}

		if (r->is_ready() && draw_settings::draw_safeable_allies->get_bool())
		{
			if (SelectedAlly != nullptr)
			{
				vector myHeroPoint = myhero->get_position();
				renderer->world_to_screen(myHeroPoint, myHeroPoint);

				std::string allyName = "Press Key to safe " + SelectedAlly->get_model();
				const char* pressR = allyName.c_str();

				draw_manager->add_text_on_screen({ myHeroPoint.x,myHeroPoint.y + 50 }, DRAW_SAFE_ALLIES, 20, pressR);
			}
		}
	}

	void load()
	{
		q = plugin_sdk->register_spell(spellslot::q, GALIO_Q_RANGE);
		w = plugin_sdk->register_spell(spellslot::w, GALIO_W_RANGE);
		e = plugin_sdk->register_spell(spellslot::e, GALIO_E_RANGE);
		r = plugin_sdk->register_spell(spellslot::r, GALIO_LVL1_R_RANGE);

		q->set_skillshot(.25f, 120, 1400, {}, skillshot_type::skillshot_circle);
		w->set_charged(GALIO_W_RANGE, GALIO_W2_RANGE, 2.0f);

		main_tab = menu->create_tab("galio", "OuijAIO - Galio");
		main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
		{
			auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo Settings");
			{
				combo_settings::use_q = combo->add_checkbox(myhero->get_model() + ".comboUseQ", "Use Q", true, true);
				combo_settings::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());

				combo_settings::use_w = combo->add_checkbox(myhero->get_model() + ".comboUseW", "Use W", true, true);
				combo_settings::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());

				combo_settings::use_e = combo->add_checkbox(myhero->get_model() + ".comboUseE", "Use E", true, true);
				combo_settings::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());

				auto combo_use_r = combo->add_tab("combo.r", "R Config");

				combo_settings::r_min_enemies = combo_use_r->add_slider(myhero->get_model() + ".comboMinEnemies", "Minimum Enemies around Ally", 2, 1, 5, true);
				combo_settings::r_min_hp_percent = combo_use_r->add_slider(myhero->get_model() + ".comboMinHP", "Minimum HP %", 35, 0, 90, true);
				combo_settings::range_slider = combo_use_r->add_slider(myhero->get_model() + ".comboRangeCheck", "Ally Range Check", 500, 0, 1200, true);

				for (auto&& ally : entitylist->get_ally_heroes())
				{
					if (ally->get_id() != myhero->get_id())
					{
						combo_settings::use_r_on[ally->get_id()] = combo_use_r->add_checkbox("galio.use_on." + ally->get_model(), "R on " + ally->get_model(), true, false);
						combo_settings::use_r_on[ally->get_id()]->set_texture(ally->get_square_icon_portrait());
					}
				}

				combo_settings::R_Key = combo->add_hotkey("Galio.RTapmode", "R on Tap", TreeHotkeyMode::Hold, 0x52, false);
			}

			auto miscStuff = main_tab->add_tab(myhero->get_model() + ".misc", "Misc");
			{
				misc_settings::q_on_cc = miscStuff->add_checkbox("Galio.miscQCC", "Auto Q on CC", true, true);
				misc_settings::e_gapclose = miscStuff->add_checkbox("Galio.miscEonGap", "Auto E on Interruptable", true, true);;
			}

			auto draw = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings");
			{
				draw_settings::draw_range_q = draw->add_checkbox(myhero->get_model() + ".drawQ", "Draw Q", true);
				draw_settings::draw_range_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());

				draw_settings::draw_range_w = draw->add_checkbox(myhero->get_model() + ".drawW", "Draw W", true);
				draw_settings::draw_range_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());

				draw_settings::draw_range_e = draw->add_checkbox(myhero->get_model() + ".drawE", "Draw E", true);
				draw_settings::draw_range_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());

				draw_settings::draw_range_r = draw->add_checkbox(myhero->get_model() + ".drawR", "Draw R", true);
				draw_settings::draw_range_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());

				draw_settings::draw_safeable_allies = draw->add_checkbox("Galio.drawAllies", "Draw Text for Safe", true);
				draw_settings::draw_safeable_allies->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
			}

			auto harass = main_tab->add_tab("Galio.harass", "Harass");
			{
				harrass_settings::use_q = harass->add_checkbox("Galio.harassQ", "Use Q", true, true);
				harrass_settings::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				harrass_settings::use_spells_when_mana = harass->add_slider("Galio.HarassMana", "If Mana >=", 40, 0, 100, true);
			}

			auto lane = main_tab->add_tab("Galio.laneclear", "Laneclear");
			{
				farm_settings::q_minion_hit = lane->add_slider("Galio.Laning.Q.MinionsHit", "Min Minions to hit", 2, 1, 5, true);
				farm_settings::use_q = lane->add_checkbox("Galio.Laning", "Use Q", true, true);
				farm_settings::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				farm_settings::use_spells_mana = lane->add_slider("Galio.LaningMana", "If Mana >=", 40, 0, 100, true);
			}
		}

		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_draw>::add_callback(on_draw);
	}

	void unload()
	{
		menu->delete_tab(main_tab);

		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);

		SelectedAlly = nullptr;

		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_update>::remove_handler(on_draw);
	}
}