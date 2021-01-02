/* MODEL */

#include "trigger.h"
#include "TriggerVisitor.h"

#include "../view/utilunit.h"
#include "../util/helper.h"
#include "../util/utilio.h"
#include "../util/Buffer.h"
#include "../util/settings.h"
#include "../view/utilui.h"
#include "scen.h"

#include <vector>
#include <algorithm>

using std::vector;

extern class Scenario scen;

/* Triggers, Conditions, and Effects */

#undef PLAYER1_INDEX
#define PLAYER1_INDEX 1

Trigger::Trigger()
//:	state(1), loop(0), u1(0), u2(0), obj(0), obj_order(0), display_order(-1), obj_str_id(0)
:	state(1), loop(0), u1(0), obj(0), obj_order(0), display_order(-1), obj_str_id(0)
{
	memset(name, 0, sizeof(name));
}

Trigger::Trigger(const Trigger &t) // TODO: we can use the compiler's version
:	state(t.state), loop(t.loop), u1(t.u1), obj(t.obj), obj_order(t.obj_order), obj_str_id(t.obj_str_id),
	description(t.description), effects(t.effects), conds(t.conds), display_order(t.display_order)
{
	strcpy(name, t.name);
}

Trigger::Trigger(Buffer& buffer)
{
	/* We use std::vectors here for RAII. */
	using std::vector;

	int num;

	buffer.read(&state, 18);
	description.read(buffer, sizeof(long));
	buffer.reads(name, sizeof(long));

	/* effects */

	buffer.read(&num, sizeof(long));
	vector<Effect> effects;				// temporarily holds Effects before we get to order info
	effects.reserve(num);

	for (int i = 0; i < num; i++)
		effects.push_back(Effect(buffer));

	while (num--)
	{
		long index;
		buffer.read(&index, sizeof(index));
		this->effects.push_back(effects[index]);
	}

	effects.clear(); // free the memory early

	/* conditions */

	buffer.read(&num, sizeof(long));
	vector<Condition> conditions;
	conditions.reserve(num);

	for (int i = 0; i < num; i++)
		conditions.push_back(Condition(buffer));

	while (num--)
	{
		long index;
		buffer.read(&index, sizeof(index));
		this->conds.push_back(conditions[index]);
	}

	display_order = -1;
}

std::string Trigger::getIDName(bool both)
{
    std::string name("");
    if (setts.showdisplayorder && setts.showtrigids || both) {
        if (display_order == id) {
            name.append("<").append(toString<long>(id).append(")"));
        } else {
            name.append("<").append(toString<long>(display_order).append(",").append(toString<long>(id)).append(")"));
        }
    } else {
        if (setts.showdisplayorder) {
            name.append("<").append(toString<long>(display_order).append(">"));
        } else if (setts.showtrigids) {
            name.append("(").append(toString<long>(id).append(")"));
        }
    }
    return name;
}

inline std::string UnicodeToANSI(const std::wstring& str)
{
	char*  pElementText;
	int    iTextLen;
	iTextLen = WideCharToMultiByte(CP_ACP, 0,
		str.c_str(),
		-1,
		nullptr,
		0,
		nullptr,
		nullptr);
 
	pElementText = new char[iTextLen + 1];
	memset((void*)pElementText, 0, sizeof(char) * (iTextLen + 1));
	::WideCharToMultiByte(CP_ACP,
		0,
		str.c_str(),
		-1,
		pElementText,
		iTextLen,
		nullptr,
		nullptr);
 
	std::string strText;
	strText = pElementText;
	delete[] pElementText;
	return strText;
}

/*
 * DON'T LET AOKTS GET RECURSIVE WITH TRIGGER HINTS (if one trigger's
 * name displays the name of another)
 */
std::string Trigger::getName(bool tip, bool limitlen, int recursion)
{
    std::ostringstream ss;
    std::ostringstream tempss;
    std::string result;
    const char * text = "";
    const char * chat_text = "";
    const char * name = "";
    const char * sound_name = "";
    std::string c_unit_type_name;
    std::string e_unit_type_name;
    std::string e_killed_units;
    std::string e_damaged_units;
    std::string e_set_hp_units;
    std::string activated_name;
    std::string deactivated_name;


    if (!tip) {
	    ss << this->name;
	    goto theendnotext;
	}

    if (tip && effects.size() == 0) {
	    //ss << this->name;
        goto theendnotext;
    }

    bool c_own = false;
    bool c_own_fewer = false;
    bool c_in_area = false;
    bool c_object_destroyed = false;
    bool c_has_gold = false;
    bool c_x_units_killed = false;
    bool c_x_buildings_razed = false;
    bool c_has_unit_type = false;
    bool e_create_unit = false;
    bool e_remove_unit = false;
    bool e_has_unit_type = false;
    bool e_buff = false;
    bool e_nerf = false;
    bool e_rename = false;
    bool e_kill_object = false;
    bool e_change_ownership = false;
    bool e_activate = false;
    bool e_deactivate = false;
    int c_chance = 100;
    int c_n_owned = 0;
    int c_n_not_owned = 0;
    int e_n_activated = 0;
    int e_n_deactivated = 0;
    bool e_has_text = false;
    int vanquished_unit = -1;
    int activated = -1;
    int deactivated = -1;
    int amount = 0;
    int victims = 0;
    int razings = 0;
    int food_total = 0;
    int wood_total = 0;
    int stone_total = 0;
    int gold_total = 0;
    int player_decimated = -1;
    int convertee = -1;
    int player = -1;
    int deceased = -1;
    int killer = -1;
    int buyer = -1;
    int earner = -1;
    int giver = -1;
    int receiver = -1;
    int e_res_type = -1;
    int e_set_hp_value = 0;
    int timer = -1;
    int n_conds = 0;
    int n_effects = 0;
    bool victory = false;
    bool victor[9];
    bool defeat = false;
    bool e_chat = false;
    bool e_sound = false;
    bool e_research = false;
    bool e_task = false;
    Condition * last_cond = NULL;
    Effect * last_effect = NULL;

    for (int i = 0; i < 9; i++) {
        victor[i] = false;
    }

	// conditions
	for (vector<Condition>::iterator iter = conds.begin(); iter != conds.end(); ++iter) {
	    if ((scen.game == AOHD || scen.game == AOF || scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) && iter->type == ConditionType::Chance_HD) {
	        c_chance *= (iter->amount / 100.0f);
	        continue;
	    }
		if ((scen.game == UP || scen.game == ETP) && iter->type == ConditionType::Chance_UP) {
	        c_chance *= (iter->amount / 100.0f);
	        continue;
	    }
	    n_conds++;
	    last_cond = &(*iter);
	    switch (iter->type) {
	    case ConditionType::OwnObjects:
	        {
	            player = iter->player;
	            c_own = true;
	            c_n_owned = iter->amount;
	            c_has_unit_type = iter->pUnit && iter->pUnit->id();
	            if (c_has_unit_type) {
                    std::string unitname(UnicodeToANSI(iter->pUnit->name()));
                    c_unit_type_name = std::string(unitname.begin(), unitname.end());
                }
            }
	        break;
	    case ConditionType::OwnFewerObjects:
	        {
	            player = iter->player;
	            c_own_fewer = true;
	            c_n_not_owned = iter->amount;
	            c_has_unit_type = iter->pUnit && iter->pUnit->id();
	            if (c_has_unit_type) {
                    std::string unitname(UnicodeToANSI(iter->pUnit->name()));
                    c_unit_type_name = std::string(unitname.begin(), unitname.end());
                }
            }
	        break;
	    case ConditionType::ObjectsInArea:
	        {
	            player = iter->player;
	            c_in_area = true;
	            c_n_owned = iter->amount;
	            c_has_unit_type = iter->pUnit && iter->pUnit->id();
	            if (c_has_unit_type) {
                    std::string unitname(UnicodeToANSI(iter->pUnit->name()));
                    c_unit_type_name = std::string(unitname.begin(), unitname.end());
                }
            }
	        break;
	    case ConditionType::DestroyObject:
	        c_object_destroyed = true;
	        vanquished_unit = iter->object;
	        break;
	    case ConditionType::AccumulateAttribute:
	        switch (iter->res_type) {
	        case 3:
	            c_has_gold = true;
	            player = iter->player;
	            break;
	        case 20:
	            c_x_units_killed = true;
	            killer = iter->player;
	            victims = iter->amount;
	            break;
	        case 43:
	            c_x_buildings_razed = true;
	            killer = iter->player;
	            razings = iter->amount;
	            break;
	        }
	        break;
	    case ConditionType::Timer:
	        if (iter->timer > 0 && iter->timer > timer) {
	            timer = iter->timer;
	        }
	        break;
	    case ConditionType::PlayerDefeated:
	        defeat = true;
	        deceased = iter->player;
	        break;
	    }
    }

	// each effect
	for (vector<Effect>::iterator iter = effects.begin(); iter != effects.end(); ++iter) {
	    n_effects++;
	    last_effect = &(*iter);
	    if (strlen(iter->text.c_str()) > 1) {
	        e_has_text = true;
	        text = iter->text.c_str();
	    }
	    switch (iter->type) {
	    case EffectType::ResearchTechnology:
	        e_research = true;
	        break;
	    case EffectType::SendChat:
	        if (strlen(iter->text.c_str()) > 1) {
	            e_chat = true;
	            chat_text = iter->text.c_str();
	        } else {
	            n_effects--;
	        }
	        break;
	    case EffectType::Sound:
	        if (strlen(iter->sound.c_str()) > 1) {
	            e_sound = true;
	            sound_name = iter->sound.c_str();
	        } else {
	            n_effects--;
	        }
	        break;
        case EffectType::SendTribute:
            amount = iter->amount;
            giver = iter->s_player;
            receiver = iter->t_player;
            e_res_type = iter->res_type;

            if (receiver == 0 && giver > 0) {
                amount = -amount;
                int tmp = receiver;
                receiver = giver;
                giver = tmp;
            } else if (giver == 0 && receiver > 0) {
            }

            switch (e_res_type) {
            case 0:
                food_total += amount;
                break;
            case 1:
                wood_total += amount;
                break;
            case 2:
                stone_total += amount;
                break;
            case 3:
                gold_total += amount;
                break;
            }

            if (giver == 0) {
                if (amount < 0 && receiver > 0) {
                    buyer = receiver;
                } else if (amount > 0 && receiver > 0) {
                    earner = receiver;
                }
            }
            break;
        case EffectType::ActivateTrigger:
            e_activate = true;
            e_n_activated ++;
            //activated = scen.t_order[iter->trig_index];
            activated = iter->trig_index;
            break;
        case EffectType::DeactivateTrigger:
            e_deactivate = true;
            e_n_deactivated ++;
            deactivated = iter->trig_index;
            break;
	    case EffectType::CreateObject:
	        {
	            player = iter->s_player;
	            e_create_unit = true;
	            e_has_unit_type = iter->pUnit && iter->pUnit->id();
	            if (e_has_unit_type) {
                    std::string unitname(UnicodeToANSI(iter->pUnit->name()));
                    e_unit_type_name = std::string(unitname.begin(), unitname.end());
                }
            }
	        break;
	    case EffectType::TaskObject:
	        e_task = true;
	        break;
	    case EffectType::DeclareVictory:
	        if (iter->s_player > 0) {
	            victor[iter->s_player] = true;
	            victory = true;
	        }
	        break;
	    case EffectType::KillObject:
	        {
	            e_killed_units = iter->selectedUnits();
	            e_kill_object = true;
	            player_decimated = iter->s_player;
	            e_has_unit_type = iter->pUnit && iter->pUnit->id();
	            if (e_has_unit_type) {
                    std::string unitname(UnicodeToANSI(iter->pUnit->name()));
                    e_unit_type_name = std::string(unitname.begin(), unitname.end());
                }
            }
	        break;
	    case EffectType::RemoveObject:
	        {
	            player = iter->s_player;
	            e_remove_unit = true;
	            e_has_unit_type = iter->pUnit && iter->pUnit->id();
	            if (e_has_unit_type) {
                    std::string unitname(UnicodeToANSI(iter->pUnit->name()));
                    e_unit_type_name = std::string(unitname.begin(), unitname.end());
                }
            }
	        break;
	    case EffectType::ChangeOwnership:
	        {
	            convertee = iter->s_player;
	            e_change_ownership = true;
	            e_has_unit_type = iter->pUnit && iter->pUnit->id();
	            if (e_has_unit_type) {
                    std::string unitname(UnicodeToANSI(iter->pUnit->name()));
                    e_unit_type_name = std::string(unitname.begin(), unitname.end());
                }
            }
	        break;
	    case EffectType::ChangeObjectName:
	        e_rename = true;
	        name = text;
	        break;
        case EffectType::DamageObject:
	        e_damaged_units = iter->selectedUnits();
	        if (e_kill_object && e_killed_units.compare(e_damaged_units) == 0)  {
	            e_set_hp_units = e_damaged_units;
	            e_set_hp_value = -iter->amount;
	            break;
	        }
        case EffectType::ChangeObjectHP:
        case EffectType::ChangeObjectAttack:
        case EffectType::ChangeSpeed_UP:
        case EffectType::ChangeRange_UP:
        case EffectType::ChangeMeleArmor_UP:
        case EffectType::ChangePiercingArmor_UP:
            amount = iter->amount;
            if (iter->type == 24) {
                amount = -amount;
            }
            if (amount < 0) {
	            e_nerf = true;
	        } else if (amount > 0) {
	            e_buff = true;
	        }
            break;
	    }
	}

	bool e_earn_food = food_total > 0;
	bool e_lose_food = food_total < 0;
	bool e_earn_wood = wood_total > 0;
	bool e_lose_wood = wood_total < 0;
	bool e_earn_stone = stone_total > 0;
	bool e_lose_stone = stone_total < 0;
	bool e_earn_gold = gold_total > 0;
	bool e_lose_gold = gold_total < 0;
	bool e_earn_resource = e_earn_food || e_earn_wood || e_earn_stone || e_earn_gold;
	bool e_lose_resource = e_lose_food || e_lose_wood || e_lose_stone || e_lose_gold;
	bool e_only_1_activated = e_n_activated == 1;
    bool only_one_cond = n_conds == 1;
    bool only_one_effect = n_effects == 1;
	bool only_one_cond_or_effect = (n_effects + n_conds == 1);

    if (recursion > 0 && activated >= 0 && activated < scen.triggers.size()) {
        tempss << "{" << scen.triggers.at(activated).getName(setts.showtrigfunction, true, recursion - 1) << "}";
    } else {
        if (activated >= 0 && activated < scen.triggers.size()) {
            tempss << scen.triggers.at(activated).getIDName(true);
        } else {
            tempss << "(<无效> ID " << activated << ")";
        }
    }
    activated_name = tempss.str();
    tempss.clear();
    tempss.str("");

    if (recursion > 0 && deactivated >= 0 && deactivated < scen.triggers.size()) {
        tempss << "{" << scen.triggers.at(deactivated).getName(setts.showtrigfunction, true, recursion - 1) << "}";
    } else {
        if (deactivated >= 0 && deactivated < scen.triggers.size()) {
            tempss << scen.triggers.at(deactivated).getIDName(true);
        } else {
            tempss << "(<无效> ID " << activated << ")";
        }
    }
    deactivated_name = tempss.str();

	if (c_chance != 100) {
	    ss << c_chance << "% 的概率，";
	}

    if (only_one_cond && only_one_effect) {
        ss << "若" << last_cond->getName(setts.displayhints, NameFlags::LIMITLEN);
        ss << " 则" << last_effect->getName(setts.displayhints, NameFlags::LIMITLEN, recursion - 1);
        goto theendnotext;
    }

    if (only_one_cond_or_effect && only_one_cond) {
        ss << last_cond->getName(setts.displayhints, NameFlags::LIMITLEN);
        goto theendnotext;
    }

    if (only_one_cond_or_effect && only_one_effect) {
        ss << last_effect->getName(setts.displayhints, NameFlags::LIMITLEN, recursion - 1);
        goto theendnotext;
    }

    if (only_one_cond_or_effect && only_one_cond) {
        ss << last_cond->getName(setts.displayhints, NameFlags::LIMITLEN);
        goto theendnotext;
    }

    if (timer > 0) {
        if (this->loop) {
            ss << "每 " << time_string(timer,false) << " ";
        } else {
            ss << "经过 " << time_string(timer,false) << " ";
        }
    }

    if (c_in_area && c_has_gold && e_buff && e_lose_gold) {
        ss << "以 " << -gold_total << " 黄金购买强化";
        goto theend;
    }

    if (c_in_area && c_has_gold && e_create_unit && e_lose_gold) {
        switch (buyer) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << buyer;
        }
        ss << " 以 " << -gold_total << " 黄金购买 " << e_unit_type_name;
        goto theend;
    }

    if (c_in_area && c_has_gold && e_research && e_lose_gold) {
        switch (buyer) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << buyer;
        }
        ss << " 以 " << -gold_total << " 黄金购买科技";
        goto theend;
    }

    if (c_own_fewer && e_create_unit && e_has_unit_type) {
        switch (player) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << player;
        }
        ss << " 产生定量 " << e_unit_type_name;
        goto theend;
    }

    if (c_x_units_killed) {
        ss << "当 ";
        switch (killer) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << killer;
        }
        ss << " 杀敌 " << victims << " 奖励";
        if (e_earn_resource) {
            switch (e_res_type) {
            case 0:
                ss << " " << food_total << " 食物";
                break;
            case 1:
                ss << " " << wood_total << " 木材";
                break;
            case 2:
                ss << " " << stone_total << " 石料";
                break;
            case 3:
                ss << " " << gold_total << " 黄金";
                break;
            }
        }
        goto theend;
    }

    if (c_x_buildings_razed) {
        ss << "当 ";
        switch (killer) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << killer;
        }
        ss << " 摧毁 " << razings << " 奖励";
        if (e_earn_resource) {
            switch (e_res_type) {
            case 0:
                ss << " " << food_total << " 食物";
                break;
            case 1:
                ss << " " << wood_total << " 木材";
                break;
            case 2:
                ss << " " << stone_total << " 石料";
                break;
            case 3:
                ss << " " << gold_total << " 黄金";
                break;
            }
        }
        goto theend;
    }

    if (c_object_destroyed) {
        ss << "消灭 " << get_unit_full_name(vanquished_unit);
        if (e_earn_resource) {
            switch (earner) {
            case -1:
                ss << "玩家?";
                break;
            case 0:
                ss << "盖亚";
                break;
            default:
                ss << "玩家" << earner;
            }
            ss << " 获得 ";
            switch (e_res_type) {
            case 0:
                ss << " " << food_total << " 食物";
                break;
            case 1:
                ss << " " << wood_total << " 木材";
                break;
            case 2:
                ss << " " << stone_total << " 石料";
                break;
            case 3:
                ss << " " << gold_total << " 黄金";
                break;
            }
        }
        goto theend;
    }

    if (c_own && c_has_unit_type && e_earn_resource) {
        ss << c_n_owned << " " << c_unit_type_name << " 为 ";
        switch (player) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << player;
        }
		ss << " 生成";
        switch (e_res_type) {
        case 0:
            ss << " " << food_total << " 食物";
            break;
        case 1:
            ss << " " << wood_total << " 木材";
            break;
        case 2:
            ss << " " << stone_total << " 石料";
            break;
        case 3:
            ss << " " << gold_total << " 黄金";
            break;
        }
        goto theend;
    }

    if ((c_own || c_in_area) && c_has_unit_type && this->loop && timer > 0 && e_lose_resource) {
        switch (player) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << player;
        }
        ss << " 的 " << c_n_owned << " " << c_unit_type_name << " 维护消耗 ";
        switch (e_res_type) {
        case 0:
            ss << " " << -food_total << " 食物";
            break;
        case 1:
            ss << " " << -wood_total << " 木材";
            break;
        case 2:
            ss << " " << -stone_total << " 石料";
            break;
        case 3:
            ss << " " << -gold_total << " 黄金";
            break;
        }
        goto theend;
    }

    if (only_one_cond && timer == -1) {
        ss << "若" << last_cond->getName(setts.displayhints, NameFlags::LIMITLEN) << " ";
        if (c_own) {
            goto theendnotext;
        }
    } else {
        if (defeat) {
            ss << "玩家" << deceased << " 断开连接";
            goto theend;
        }
    }

    if (e_only_1_activated && activated >= 0) {
        if (e_has_text) {
            ss << trim(std::string(text)) << ". ";
        } else if (e_create_unit) {
            ss << "产生 " << e_unit_type_name << " ";
        } else if (deactivated >= 0) {
            ss << "关闭 " << deactivated_name << " ";
        }

        ss << "=> " << activated_name;
        goto theendnotext;
    }

    if (victory) {
        for (int i = 0; i < 9; i++) {
            if (victor[i])
                ss << "玩家" << i << " ";
        }
        ss << "宣布胜利";
        goto theend;
    }

    if (c_in_area && e_remove_unit) {
        switch (player) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << player;
        }
        ss << " 以单位购买";
        goto theend;
    }
    //if (c_has_gold) {
    //    ss << "has gold";
    //}
    //if (e_create_unit) {
    //    ss << "create_unit";
    //}
    //if (e_lose_gold) {
    //    ss << "lose gold";
    //}

    if (e_change_ownership) {
        ss << "改权 ";
        switch (convertee) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << convertee;
        }
        ss << " " << (e_has_unit_type?e_unit_type_name:"单位");
        goto theend;
    }

    if (!e_set_hp_units.empty()) {
        ss << "设定  " << e_set_hp_units << " 生命值为 " << e_set_hp_value;
        goto theend;
    }

    if (e_kill_object) {
        ss << "摧毁 ";
        switch (player_decimated) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << player_decimated;
        }
        ss << " " << (e_has_unit_type?e_unit_type_name:"单位");
        goto theend;
    }

    if (e_create_unit) {
        ss << "产生 ";
        switch (player) {
        case -1:
            ss << "玩家?";
            break;
        case 0:
            ss << "盖亚";
            break;
        default:
            ss << "玩家" << player;
        }
        ss << " " << (e_has_unit_type?e_unit_type_name:"单位");
        goto theend;
    }

    if (e_nerf) {
        ss << "弱化单位";
        goto theend;
    }

    if (e_buff) {
        ss << "强化单位";
        goto theend;
    }

    if (e_research) {
        ss << "研究";
        goto theend;
    }

    if (e_remove_unit) {
        ss << "移除物件";
        goto theend;
    }

    if (e_task) {
        ss << "指派物件";
        goto theend;
    }

    if (e_sound) {
        ss << "播放音效 " << " 「" << trim(std::string(sound_name)) << "」";
        goto theend;
    }

    if (e_rename) {
        ss << "改名为 " << " 「" << trim(std::string(text)) << "」";
        goto theendnotext;
    }

    if (e_chat) {
        ss << "发送讯息 " << " 「" << trim(std::string(chat_text)) << "」";
        goto theendnotext;
    }

    if (only_one_effect) {
        ss << last_effect->getName(setts.displayhints, NameFlags::LIMITLEN, recursion - 1);
        goto theendnotext;
    }

    if (n_effects > 0)
        ss << "...";
    goto theend;
theend:
    if (e_has_text) {
        ss << " 「" << trim(std::string(text)) << "」";
    }

theendnotext:
    result = ss.str();
	
	std::string temptrig = this->name;
	if ( result.length() >= 197-( temptrig.length() ) ) {
		result = result.substr( 0,197-( temptrig.length() ) ) + "...";
	}

	return result;
    //return limitlen?result.substr(0,200-(temptrig.length())):result;
}

bool compare_effect_nametip(const Effect& first,
				  const Effect& second)
{
    //make change ownership act like it is prefixed with create (so that
    //create comes first

    std::string s = std::string(first.getName(true));
    std::string t = std::string(second.getName(true));

    // UP only -- change speed must come after create
    if (first.type == 30 && scen.ver2 == SV2_AOC_SWGB) {
        s.insert (0, "创建");
    }

    switch (first.type) {
    case (long)EffectType::RemoveObject: // must come before create (clear way for spawn)
        s.insert (0, "创建 ");
        break;
    case (long)EffectType::KillObject: // must come after create (hawk explosions)
    case (long)EffectType::DamageObject:
    case (long)EffectType::ChangeObjectName:
    case (long)EffectType::ChangeObjectHP: // change stats must come after create (hawk explosions)
    case (long)EffectType::ChangeObjectAttack:
    case (long)EffectType::ChangeRange_UP:
    case (long)EffectType::ChangeMeleArmor_UP:
    case (long)EffectType::ChangePiercingArmor_UP:
    case (long)EffectType::ChangeOwnership: // must come after create (non-convertible gaia)
        s.insert (0, "创建");
    }

    switch (second.type) {
    case (long)EffectType::RemoveObject: // must come before create (clear way for spawn)
        s.insert (0, "创建 ");
        break;
    case 14: // kill must come after create (hawk explosions)
    case (long)EffectType::DamageObject:
    case (long)EffectType::ChangeObjectName:
    case (long)EffectType::ChangeObjectHP: // change stats must come after create (hawk explosions)
    case (long)EffectType::ChangeObjectAttack:
    case (long)EffectType::ChangeRange_UP:
    case (long)EffectType::ChangeMeleArmor_UP:
    case (long)EffectType::ChangePiercingArmor_UP:
    case (long)EffectType::ChangeOwnership: // must come after create (non-convertible gaia)
        t.insert (0, "创建");
    }

	// Windows doesn't support POSIX strcasecmp().
	int sort = _stricmp(s.c_str(),
		t.c_str());

	return (sort < 0);
}

bool compare_condition_nametip(const Condition& first,
				  const Condition& second)
{
	// Windows doesn't support POSIX strcasecmp().
	int sort = _stricmp(first.getName(true).c_str(),
		second.getName(true).c_str());

	return (sort < 0);
}

void Trigger::sort_conditions()
{
    std::sort(this->conds.begin(), this->conds.end(), compare_condition_nametip);
}

void Trigger::sort_effects()
{
    std::sort(this->effects.begin(), this->effects.end(), compare_effect_nametip);
}

void Trigger::read(FILE *in)
{
	using std::vector;

	long n_effects, n_conds;

	// TODO: read in as struct
	readbin(in, &state); // enabled/disabled
	readbin(in, &loop);  // looping
	readbin(in, &u1);    // unknown. trim trigger?
	//readbin(in, &u2);    // unknown. trim trigger?
	if (setts.intense) {
	    show_binrep(u1);
	    //swapByteOrder(u1);
	    //swapByteOrder(u2);
	    //printf_log("trigger unknown: %u\n", u1);
	    //printf_log("trigger unknown: %u\n", u2);
	}
	readbin(in, &obj);
	readbin(in, &obj_order);
	readbin(in, &obj_str_id);

	description.read(in, sizeof(long));

	// number of characters (including null)
    unsigned long len;
	readbin(in, &len);
	if (len > MAX_TRIGNAME) {
	    throw std::length_error(toString<int>(len).c_str());
	}
	readbin(in, name, len);
	name[len] = '\0'; // Scenario strings are not always null-terminated.

	//read effects
	readbin(in, &n_effects);
	if (setts.intense) {
	    printf_log("nEffects: ");
	    show_binrep(n_effects);
	}

	if (n_effects > 0)
	{
		// construct them all so we can directly call read()
		vector<Effect> effects(n_effects);
		//effects.resize(n_effects);

		for (int i = 0; i < n_effects; i++)
		{
			try
			{
			    effects[i].parent_trigger_id = id;
				effects[i].read(in);
				//this->effects[i].read(in);
			}
			catch (std::exception &)
			{
				printf_log("Effect %d invalid.\n", i);
				throw;
			}
		}

		while (n_effects--)
		{
			long order = readval<long>(in);
	        if (setts.intense) {
	            printf_log("effect order: %d\n", order);
	        }
	        //show_binrep(order);
			// I keep the effects in the proper order in memory, unlike AOK.
			this->effects.push_back(effects[order]);
		}
	}

	//if (u1 != 76548) {

	//read conditions
	readbin(in, &n_conds);
	if (setts.intense) {
	    printf_log("nConds: ");
	    show_binrep(n_conds);
	}

	if (n_conds > 0)
	{
		// construct them all so we can directly call read()
		vector<Condition> conditions(n_conds);

		for (int i = 0; i < n_conds; i++)
		{
			try
			{
				conditions[i].read(in);
			}
			catch (std::exception &)
			{
				printf_log("Condition %d invalid.\n", i);
				throw;
			}
		}

		while (n_conds--)
		{
			long order = readval<long>(in);
			conds.push_back(conditions[order]);
		}
	}
}

void Trigger::write(FILE *out)
{
	unsigned long num, i;

	fwrite(&state, 18, 1, out);	//state, loop, u1, obj, obj_order, obj_str_id
	description.write(out, sizeof(long), true);

	// don't use writecs here
	num = strlen(name) + 1; // strlen is non-standard? includes '\0' in the count
	if (num > MAX_TRIGNAME_SAFE_HD)
	    num = MAX_TRIGNAME_SAFE_HD;
	    //throw std::length_error("Trigger's name is more than 126 characters long?");
	fwrite(&num, sizeof(long), 1, out);
	name[num] = '\0';                     // num includes null character but don't rely on that
	fwrite(name, sizeof(char), num, out); // string includes a null character. all is written

	//Effects

	num = effects.size();

	fwrite(&num, 4, 1, out);
	for (i = 0; i < num; i++)
		effects[i].write(out);

	for (i = 0; i < num; i++)
		fwrite(&i, 4, 1, out);

	//Conditions

	num = conds.size();

	fwrite(&num, 4, 1, out);
	for (i = 0; i < num; i++)
		conds[i].write(out);

	for (i = 0; i < num; i++)
		fwrite(&i, 4, 1, out);
}

void Trigger::tobuffer(Buffer& buffer)// const (make it const when unit_cnst gets set elsewhere)
{
	int i, num;

	// trigger
	buffer.write(&state, 18);
	description.write(buffer, sizeof(long));
	buffer.writes(name, sizeof(long));

	// effects
	num = effects.size();
	buffer.write(&num, sizeof(long));
	for (i = 0; i < num; i++)
		effects[i].tobuffer(buffer);

	// effects order
	for (i = 0; i < num; i++)
		buffer.write(&i, sizeof(long));

	// conditions
	num = conds.size();
	buffer.write(&num, sizeof(long));
	for (i = 0; i < num; i++)
		conds[i].tobuffer(buffer);

	// conditions order
	for (i = 0; i < num; i++)
		buffer.write(&i, sizeof(long));
}

class get_ec_player
{
public:
	get_ec_player()
		: _player(0) // assume GAIA at start
	{}

	int player() const
	{
		return _player;
	}

	void operator()(const ECBase& e)
	{
		int player = e.getPlayer();

		// If player is GAIA or -1, skip this effect.
		if (player > ECBase::GAIA_INDEX && ( player<'0'?player:player-'0' ) != (_player<'0'?_player:_player-'0') )
		{
			if (!_player) // no player assigned yet?
				_player = player;
			else
				_player = -1; // not the same player, error
		}
	}

private:
	int _player;
};

int Trigger::get_player()
{
	get_ec_player gep;
	gep = std::for_each(effects.begin(), effects.end(), gep);
	gep = std::for_each(conds.begin(), conds.end(), gep);

	return gep.player();
}

/**
 * Functor to send a TriggerVisitor to Conditions and Effects.
 */
class send_visitor
{
public:
	send_visitor(TriggerVisitor& tv)
		: _tv(tv)
	{}

	void operator()(Condition& c)
	{
		c.accept(_tv);
	}

	void operator()(Effect& e)
	{
		e.accept(_tv);
	}

private:
	TriggerVisitor& _tv;
};

void Trigger::accept(TriggerVisitor& tv)
{
	tv.visit(*this);

	send_visitor sv(tv);
	std::for_each(conds.begin(), conds.end(), sv);
	std::for_each(effects.begin(), effects.end(), sv);

	tv.visitEnd(*this);
}
