#include "Condition.h"
#include <sstream>
#include "TriggerVisitor.h"

#include "../view/utilunit.h"
#include "../util/utilio.h"
#include "../util/Buffer.h"
#include "../util/helper.h"
#include "../util/settings.h"

static const long MAXFIELDS=18;

const long Condition::defaultvals[MAXFIELDS] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1 };

Condition::Condition()
:	ECBase(CONDITION),
	pUnit(NULL),
	pTech(NULL),
	valid_since_last_check(true)
{
    size = MAXFIELDS;
    memcpy(&amount, defaultvals, sizeof(defaultvals));
}

// Can only delegate (chain) constructors since C++11
Condition::Condition(Buffer& b)
:	ECBase(CONDITION),
	pUnit(NULL),
	pTech(NULL),
	valid_since_last_check(true)
{
    memcpy(&amount, defaultvals, sizeof(defaultvals));
    ClipboardType::Value cliptype;
    b.read(&cliptype, sizeof(cliptype));
    if (cliptype != ClipboardType::CONDITION)
        throw bad_data_error("条件检测值不正确。");

    b.read(&type, sizeof(type));
    b.read(&size, sizeof(size));

    if (size >= 0) {
        long * flatdata = new long[size];
        b.read(flatdata, sizeof(long) * size);
        memcpy(&amount, flatdata, sizeof(long) * (size<=MAXFIELDS?size:MAXFIELDS));
        delete[] flatdata;
    }

    if (size >= 5)
        pUnit = esdata.units.getByIdSafe(unit_cnst);
    if (size >= 7)
	    pTech = esdata.techs.getByIdSafe(tech_cnst);

	check_and_save();
}

std::string Condition::areaName() const {
    std::ostringstream convert;
    if (valid_full_map()) {
        convert << "整个地图";
    } else {
        if (valid_area_location()) {
            convert << "位于 (" << area.left << "," << area.top << ") ";
        } else {
            convert << "位于 (" << area.left << "," << area.bottom << ") 起的 [" << area.right - area.left + 1 << "x" << area.top - area.bottom + 1 << "] 区域内";
        }
    }
    return convert.str();
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

inline std::string unitTypeName(const UnitLink *pUnit) {
    std::ostringstream convert;
    if (pUnit && pUnit->id()) {
        std::string unitname(UnicodeToANSI(pUnit->name()));
        std::string un(unitname.begin(), unitname.end());
        convert << un;
    } else {
        convert << "单位";
    }
    return convert.str();
}

std::string Condition::selectedUnits() const {
    std::ostringstream convert;
    bool class_selected = group >= 0 && group < NUM_GROUPS - 1;
    bool unit_type_selected = utype >= 0 && utype < NUM_UTYPES - 1;
    if (player >= 0)
        convert << playerPronoun(player) << " ";
    if (pUnit && pUnit->id()) {
        convert << unitTypeName(pUnit);
    } else if (class_selected || unit_type_selected) { // account for groups[0]={-1,None}
        if (class_selected && unit_type_selected) {
            convert << "种属" << groups[group + 1].name;
            convert << "的" << utypes[utype + 1].name;
			convert << "单位";
        } else if (class_selected) {
            convert << groups[group + 1].name;
			convert << "种属";
        } else {
            convert << utypes[utype + 1].name;
			convert << "单位";
        }
    } else {
        convert << "单位";
    }
    if (!(object == -1 && !valid_unit_id(object))) {
        convert << " " << object << " (" << get_unit_full_name(object) << ")";
    }
    return convert.str();
}

std::string Condition::getName(bool tip, NameFlags::Value flags, int recursion) const
{
    if (!tip) {
	    return (type < scen.pergame->max_condition_types) ? types[type] : "<未知>!";
	} else {
	    std::string stype = std::string("");
        std::ostringstream convert;

        if (type == ConditionType::None) {
            // Let this act like a separator
            convert << "                                                                                    ";
            stype.append(convert.str());
            goto end;
        }

	    if ( (scen.game == UP || scen.game == ETP) && reserved == -256 || (scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) && reverse_hd == 1)
	        convert << "未满足" ;

        switch (type) {
        case ConditionType::BringObjectToArea:
            convert << get_unit_full_name(object) << " " << object;
            convert << " 到达" << areaName();
            stype.append(convert.str());
            break;
        case ConditionType::BringObjectToObject:
            convert << get_unit_full_name(object) << " " << object << " 接近 " << get_unit_full_name(u_loc) << " " << u_loc;
            stype.append(convert.str());
            break;
        case ConditionType::OwnObjects:
        case ConditionType::OwnFewerObjects:
        case ConditionType::ObjectsInArea:
            convert << areaName();
            switch (type) {
            case ConditionType::OwnObjects:
            case ConditionType::ObjectsInArea:
				convert << "有";
                if (amount == 0) {
                    //convert << "any number of";
                } else {
                    convert << "至少" << amount << " 个";
                }
                break;
            case ConditionType::OwnFewerObjects:
                if (amount == 0) {
                    convert << "没有";
                } else {
                    convert << "不多于" << amount << " 个";
                }
                break;
            }
            convert << " " << selectedUnits();
            stype.append(convert.str());
            break;
        case ConditionType::DestroyObject:
            convert << get_unit_full_name(object) << " 被消灭";
            stype.append(convert.str());
            break;
        case ConditionType::CaptureObject:
            convert << playerPronoun(player) << " 捕获 " << get_unit_full_name(object);
            stype.append(convert.str());
            break;
        case ConditionType::AccumulateAttribute:
            switch (res_type) {
            case 0: // Food accumulated
                convert << playerPronoun(player) << " 累积 " << amount << " 食物";
                break;
            case 1: // Wood accumulated
                convert << playerPronoun(player) << " 累积 " << amount << " 木材";
                break;
            case 2: // Stone accumulated
                convert << playerPronoun(player) << " 累积 " << amount << " 石料";
                break;
            case 3: // Gold accumulated
                convert << playerPronoun(player) << " 累积 " << amount << " 黄金";
                break;
            case 20: // Units killed
                convert << playerPronoun(player) << " 杀敌 " << amount << " 名";
                break;
            case 44: // Kill ratio
                if (amount == 0) {
                    convert << playerPronoun(player) << " 杀敌等于损失";
                } else if (amount == 1) {
                    convert << playerPronoun(player) << " 杀敌超过损失 1 单位";
                } else if (amount > 0) {
                    convert << playerPronoun(player) << " 杀敌超过损失 " << amount << " 单位";
                } else if (amount == -1) {
                    convert << playerPronoun(player) << " 损失超过杀敌 1 单位";
                } else {
                    convert << playerPronoun(player) << " 损失超过杀敌 " << -amount << " 单位";
                }
                break;
            default:
                //convert << types_short[type];
                if (res_type >= 0) {
                    const Link * list = esdata.resources.head();
	                for (int i=0; list; list = list->next(), i++)
	                {
		                if (i == res_type) {
                            std::string resname(UnicodeToANSI(list->name()));
		                    convert << playerPronoun(player) << " 的";
                            convert << std::string( resname.begin(), resname.end());
                            convert << " ≥ " << amount;
		                    break;
		                }
	                }
	            }
                break;
            }
            stype.append(convert.str());
            break;
        case ConditionType::ResearchTehcnology:
            if (valid_technology_spec()) {
                convert << playerPronoun(player) << " 已研发科技 ";
                std::string techname(UnicodeToANSI(pTech->name()));
                convert << std::string( techname.begin(), techname.end());
            } else {
                convert << " <无效> ";
            }
            stype.append(convert.str());
            break;
        case ConditionType::ResearchingTechnology:
            if (pTech && pTech->id()) {
                convert << playerPronoun(player) << " 正在研发科技 ";
                std::string techname(UnicodeToANSI(pTech->name()));
                convert << std::string( techname.begin(), techname.end());
            } else {
                convert << " <无效> ";
            }
            stype.append(convert.str());
            break;
        case ConditionType::Timer:
            convert << "时间经过 " << time_string(timer);
            stype.append(convert.str());
            break;
        case ConditionType::ObjectSelected:
            convert << "选择单位 " << object;
            convert << " (" << get_unit_full_name(object) << ")";
            stype.append(convert.str());
            break;
        case ConditionType::AISignal:
            switch (ai_signal) {
            case -1034:
                convert << "单人模式 / 允许作弊";
                break;
            case -1036:
                convert << "起始资源：标准";
                break;
            case -1039:
                convert << "弑君模式";
                break;
            case -1040:
                convert << "死亡竞赛";
                break;
            case -70850:
                convert << "单击驻扎模式";
                break;
            default:
                if (ai_signal >= -518 && ai_signal <= -7) {
                    int signal = ai_signal + 518;
                    int taunt_player = signal / 64;
                    int taunt_set_id = signal % 64;
                    convert << "玩家 " << taunt_player + 1 << " 送出嘲讽 " << taunt_set[taunt_set_id];
                } else if (ai_signal >= -774 && ai_signal <= -519) {
                    convert << "达成 AI 目标 " << ai_signal + 774;
                } else {
                    convert << "AI 信号 " << ai_signal;
                }
            }
            stype.append(convert.str());
            break;
        case ConditionType::ObjectVisible:
            convert << "物件 " << object;
            convert << " (" << get_unit_full_name(object) << ") 可视";
            stype.append(convert.str());
            break;
        case ConditionType::PlayerDefeated:
            if (player == 0) {
                convert << "盖亚";
            } else {
                convert << playerPronoun(player);
            }
            convert << " 被击败";
            stype.append(convert.str());
            break;
        case ConditionType::ObjectHasTarget:
            convert << object << " (" << get_unit_full_name(object) << ") ";
            if (null_location_unit()) {
                convert << "已有目标";
            } else {
                convert << "目标为 " << u_loc << " (" << get_unit_full_name(u_loc) << ")";
            }
            stype.append(convert.str());
            break;
        case ConditionType::UnitsGarrisoned:
            convert << object << " (" << get_unit_full_name(object) << ") 驻扎了 " << amount << " 个单位";
            stype.append(convert.str());
            break;
        case ConditionType::DifficultyLevel:
            convert << "难度设定为 ";
            switch (amount) {
            case DifficultyLevel::Hardest:
                convert << "极难";
                break;
            case DifficultyLevel::Hard:
                convert << "难";
                break;
            case DifficultyLevel::Moderate:
                convert << "中等";
                break;
            case DifficultyLevel::Standard:
                convert << "标准";
                break;
            case DifficultyLevel::Easiest:
                convert << "最易";
                break;
            }
            stype.append(convert.str());
            break;
        case ConditionType::UnitsQueuedPastPopCap_SWGB:
            if (valid_player()) {
                convert << playerPronoun(player) << " has " << amount << " units queued past the pop cap";
            } else {
                convert << " <无效> ";
            }
            stype.append(convert.str());
            break;
        case ConditionType::OwnFewerFoundations_SWGB: // Chance_HD://Civilization_UP:
            switch (scen.game) {
            case AOHD:
            case AOF:
            case AOHD4:
            case AOF4:
            case AOHD6:
            case AOF6:
                convert << "随机概率" << amount << "% ";
                stype.append(convert.str());
                break;
			case ETP:
			case UP:{
					std::string civname(UnicodeToANSI(esdata.civs.getByIdSafe(amount)->name()));
					convert << playerPronoun(player) << "为" << std::string( civname.begin(), civname.end()) << "文明";
					stype.append(convert.str());
					break;
				}
            case SWGB:
            case SWGBCC:
                if (valid_player()) {
                    convert << playerPronoun(player) << " has ";
                }
                if (amount == 0) {
                    convert << "no";
                } else {
                    convert << "at most " << amount;
                }
                convert << " " << selectedUnits();
                if (amount != 1) {
                    convert << " foundations";
                } else {
                    convert << " foundation";
                }
                convert << areaName();
                stype.append(convert.str());
                break;
            default:
                convert << amount << "<未知条件>";
                stype.append(convert.str());
            }
            break;
		case ConditionType::Chance_UP://SelectedObjectsInArea_SWGB:
			switch (scen.game) {
			case ETP:
			case UP:
				convert << "随机概率" << amount << "% ";
                stype.append(convert.str());
                break;
			default:
				convert << amount << "<未知条件>";
                stype.append(convert.str());
			}
			break;
        default:
            stype.append((type < scen.pergame->max_condition_types) ? types_short[type] : "<未知>!");
        }

end:
        return flags&NameFlags::LIMITLEN?stype.substr(0,MAX_CHARS):stype;
    }
}

int Condition::getPlayer() const
{
	return player;
}

void Condition::setPlayer(int pplayer)
{
	player = pplayer;
}

bool Condition::get_valid_since_last_check() {
    return valid_since_last_check;
}

bool Condition::check_and_save()
{
    return valid_since_last_check = check();
}

inline bool Condition::valid_player() const {
    return player >= 0 && player <= 8;
}

inline bool Condition::valid_full_map() const {
    return (area.left == -1 && area.right == -1 && area.bottom == -1 && area.top == -1);
}

inline bool Condition::valid_partial_map() const {
    return (area.left >=  0 && area.right >= area.left && area.bottom >=  0 && area.top >= area.bottom);
}

inline bool Condition::valid_area() const {
    return valid_full_map() || valid_partial_map();
}

inline bool Condition::valid_area_location() const {
    return area.left == area.right && area.top == area.bottom;
}

inline bool Condition::valid_unit_spec() const {
    // utype >= 0
	return pUnit != NULL && pUnit->id() >= 0;
}

inline bool Condition::valid_technology_spec() const {
	return pTech != NULL && pTech->id() >= 0;
}

inline bool Condition::null_object() const {
	return object == -1;
}

inline bool Condition::valid_object() const {
	return valid_unit_id(object);
}

inline bool Condition::null_location_unit() const {
	return u_loc == -1;
}

inline bool Condition::valid_location_unit() const {
	return valid_unit_id(u_loc);
}

bool Condition::check() const
{
    if (type < 1 || type >= scen.pergame->max_condition_types) {
        return false;
    }

	switch (type)
	{
	case ConditionType::BringObjectToArea:
		return (valid_unit_id(object) && valid_partial_map());

	case ConditionType::BringObjectToObject:
		return (valid_unit_id(object) && valid_location_unit());

	case ConditionType::ObjectHasTarget:
		return (valid_unit_id(object) && (null_location_unit() || valid_location_unit()));

	case ConditionType::OwnObjects:
	case ConditionType::OwnFewerObjects:
		return (player >= 0 && amount >= 0 && valid_area());

	case ConditionType::OwnFewerFoundations_SWGB: // Chance_HD://Civilization_UP:
	    if (scen.game == AOHD || scen.game == AOF || scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) {
	        return (amount >= 0 && amount <= 100);
	    }
		else { 
			if (scen.game == UP || scen.game == ETP){
				return (amount >= 0 && player >= 0);
			}
			else {
				return (player >= 0 && amount >= 0 && valid_area());
			}
		}
	case ConditionType::ObjectsInArea:
	case ConditionType::Chance_UP://SelectedObjectsInArea_SWGB
		if (scen.game == UP || scen.game == ETP) {
	        return (amount >= 0 && amount <= 100);
	    }
	case ConditionType::PoweredObjectsInArea_SWGB:
		return (valid_area() && amount >= 0);

	case ConditionType::DestroyObject:
		return (valid_unit_id(object));

	case ConditionType::CaptureObject:
		return (valid_unit_id(object) && player >= 0);

	case ConditionType::AccumulateAttribute:
		return (player >= 0 && res_type >= 0);

	case ConditionType::ResearchTehcnology:
	case ConditionType::ResearchingTechnology:
		return (player >= 0 && valid_technology_spec());

	case ConditionType::Timer:
		return (timer >= 0);

	case ConditionType::ObjectSelected:
		return (valid_unit_id(object));

	case ConditionType::AISignal:
		return (true);

	case ConditionType::PlayerDefeated:
	case ConditionType::UnitsQueuedPastPopCap_SWGB:
		return (player >= 0);

	case ConditionType::ObjectVisible:
	case ConditionType::ObjectNotVisible:
		return (valid_unit_id(object));

	//ConditionType::ResearchingTechnology above

	case ConditionType::UnitsGarrisoned:
		return (valid_unit_id(object) && amount >= 0);

	case ConditionType::DifficultyLevel:
		return (amount >= 0);

	//ConditionType::OwnFewerFoundations shares with OwnObjects above
	//ConditionType::SelectedObjectsInArea shares with ObjectsInArea above
	//ConditionType::PoweredObjectsInArea shares with ObjectsInArea above
	//ConditionType::UnitsQueuedPastPopCap shares with PlayerDefeated above

	default:
		return false;
	}
}

void Condition::read(FILE *in)
{
    memcpy(&amount, defaultvals, sizeof(defaultvals));

    readbin(in, &type);
    readbin(in, &size);

    if (size < 0 || size > MAXFIELDS)
        throw bad_data_error("条件长度不正确。");

	size_t read = fread(&amount, sizeof(long), size, in);
	if (read != (size_t)size)
	{
		throw bad_data_error(
			(feof(in)) ? "Early EOF" : "stream error");
	}

    if (size >= 5)
        pUnit = esdata.units.getByIdSafe(unit_cnst);
    if (size >= 7)
	    pTech = esdata.techs.getByIdSafe(tech_cnst);

	check_and_save();
}

void Condition::write(FILE *out)
{
    compress();

	writebin(out, &type);
	writebin(out, &size);

    if (size >= 5)
        unit_cnst = pUnit ? pUnit->id() : -1;
    if (size >= 7)
	    tech_cnst = pTech ? pTech->id() : -1;

    if (size >= 0 && size <= MAXFIELDS) {
	    fwrite(&amount, sizeof(long) * size, 1, out);
    }
}

/* Used forBuffer operations (i.e., copy & paste) */
void Condition::tobuffer(Buffer &b)// const (make it const when unit_cnst gets set elsewhere)
{
    ClipboardType::Value cliptype = ClipboardType::CONDITION;
	b.write(&cliptype, sizeof(cliptype));

	b.write(&type, sizeof(type));
	b.write(&size, sizeof(size));

    if (size >= 5)
        unit_cnst = pUnit ? pUnit->id() : -1;
    if (size >= 7)
	    tech_cnst = pTech ? pTech->id() : -1;

    if (size >= 0 && size <= MAXFIELDS) {
	    b.write(&amount, sizeof(long) * size);
    }
}

void Condition::compress()
{
    size = MAXFIELDS;
    for (int i = size - 1; i >= 0; i--) {
        if (*(&amount + i) == defaultvals[i]) {
            size--;
        } else {
            if (setts.intense)
                printf_log("condition size %d.\n",size);
            break;
        }
    }
}

void Condition::accept(TriggerVisitor& tv)
{
	tv.visit(*this);
}

const char *Condition::types_aok[] =
{
	"无",
	"将物件带到指定区域",
	"将物件带给指定物件",
	"拥有物件数量大于等于",
	"拥有物件数量小于等于",
	"区域内的物件大于等于",
	"消灭物件",
	"捕获物件",
	"堆积属性",
	"研究科技",
	"计时器",
	"已选择物件",
	"收到AI信号",
	"玩家被击败",
	"物件已有目标",
	"物件可视",
	"物件不可视",
	"正研发科技",
	"已驻扎单位",
	"游戏难度",
};

const char *Condition::types_up[] =
{
	"无",
	"将物件带到指定区域",
	"将物件带给指定物件",
	"拥有物件数量大于等于",
	"拥有物件数量小于等于",
	"区域内的物件大于等于",
	"消灭物件",
	"捕获物件",
	"堆积属性",
	"研究科技",
	"计时器",
	"已选择物件",
	"收到AI信号",
	"玩家被击败",
	"物件已有目标",
	"物件可视",
	"物件不可视",
	"正研发科技",
	"已驻扎单位",
	"游戏难度",
	"文明",
	"随机百分比",
};

const char *Condition::types_etp[] =
{
	"无",
	"将物件带到指定区域",
	"将物件带给指定物件",
	"拥有物件数量大于等于",
	"拥有物件数量小于等于",
	"区域内的物件大于等于",
	"消灭物件",
	"捕获物件",
	"堆积属性",
	"研究科技",
	"计时器",
	"已选择物件",
	"收到AI信号",
	"玩家被击败",
	"物件已有目标",
	"物件可视",
	"物件不可视",
	"正研发科技",
	"已驻扎单位",
	"游戏难度",
	"文明",
	"随机百分比",
	"地基数量小于等于",
	"废墟数量小于等于",
	"对战设定",
	"触发激活",
	"计算机玩家",
	"变量值等于",
	"变量值大于",
	"科技状态",
};

const char *Condition::types_swgb[] =
{
	"无",
	"将物件带到指定区域",
	"将物件带给指定物件",
	"拥有物件数量大于等于",
	"拥有物件数量小于等于",
	"区域内的物件大于等于",
	"消灭物件",
	"捕获物件",
	"堆积属性",
	"研究科技",
	"计时器",
	"已选择物件",
	"收到AI信号",
	"玩家被击败",
	"物件已有目标",
	"物件可视",
	"物件不可视",
	"正研发科技",
	"已驻扎单位",
	"游戏难度",
	"拥有地基数量小于等于",
	"选择区域内的物件",
};

const char *Condition::types_cc[] =
{
	"无",
	"将物件带到指定区域",
	"将物件带给指定物件",
	"拥有物件数量大于等于",
	"拥有物件数量小于等于",
	"区域内的物件大于等于",
	"消灭物件",
	"捕获物件",
	"堆积属性",
	"研究科技",
	"计时器",
	"已选择物件",
	"收到AI信号",
	"玩家被击败",
	"物件已有目标",
	"物件可视",
	"物件不可视",
	"正研发科技",
	"已驻扎单位",
	"游戏难度",
	"拥有地基数量小于等于",
	"选择区域内的物件",
	"Powered Objects in Area",
	"Units Queued Past Pop Cap",
};

const char *Condition::types_short_aok[] =
{
	"",
	"抵达区域",
	"带给物件",
	"拥有大于",
	"拥有少于",
	"区域内",
	"消灭",
	"捕获",
	"堆积",
	"研究",
	"计时",
	"选择",
	"AI 信号",
	"击败",
	"已有目标",
	"可视",
	"不可视",
	"正研究",
	"已驻扎",
	"难度",
	"随机",
};

const char *Condition::types_short_up[] =
{
	"",
	"抵达区域",
	"带给物件",
	"拥有大于",
	"拥有少于",
	"区域内",
	"消灭",
	"捕获",
	"堆积",
	"研究",
	"计时",
	"选择",
	"AI 信号",
	"击败",
	"已有目标",
	"可视",
	"不可视",
	"正研究",
	"已驻扎",
	"难度",
	"文明",
	"随机",
};

const char *Condition::types_short_etp[] =
{
	"",
	"抵达区域",
	"带给物件",
	"拥有大于",
	"拥有少于",
	"区域内",
	"消灭",
	"捕获",
	"堆积",
	"研究",
	"计时",
	"选择",
	"AI 信号",
	"击败",
	"已有目标",
	"可视",
	"不可视",
	"正研究",
	"已驻扎",
	"难度",
	"文明",
	"随机",
	"地基少于",
	"废墟少于",
	"对战设定",
	"触发激活",
	"计算机玩家",
	"变量等于",
	"变量大于",
	"科技状态",
};

const char *Condition::types_short_swgb[] =
{
	"",
	"Arrived",
	"At Object",
	"Own",
	"Own Fewer",
	"In Area",
	"Destroyed",
	"Captured",
	"Accumulated",
	"Researched",
	"Time",
	"Selected",
	"AI Signal",
	"Defeated",
	"Targetting",
	"Visible",
	"Not Visible",
	"Researching",
	"Garrisoned",
	"Difficulty",
	"Fewer Foundations",
	"Selected in Area",
};

const char *Condition::types_short_cc[] =
{
	"",
	"Arrived",
	"At Object",
	"Own",
	"Own Fewer",
	"In Area",
	"Destroyed",
	"Captured",
	"Accumulated",
	"Researched",
	"Time",
	"Selected",
	"AI Signal",
	"Defeated",
	"Targetting",
	"Visible",
	"Not Visible",
	"Researching",
	"Garrisoned",
	"Difficulty",
	"Fewer Foundations",
	"Selected in Area",
	"Powered in Area",
	"Queued Past Pop Cap",
};

const char *Condition::virtual_types_aok[] = {
    "",
    "难度：极难",
    "难度：难",
    "难度：中等",
    "难度：标准",
    "难度：最易",
};

const char *Condition::virtual_types_aoc[] = {
    "",
    "单人模式 / 允许作弊",
    "嘲讽音效",
    "AI 脚本目标",
    "难度：极难",
    "难度：难",
    "难度：中等",
    "难度：标准",
    "难度：最易",
    "起始时代：标准",
    "起始资源：标准",
    "弑君模式",
    "死亡竞赛",
    //"单击进驻模式",
};

const char *Condition::virtual_types_aohd[] = {
    "",
    "难度：极难",
    "难度：难",
    "难度：中等",
    "难度：标准",
    "难度：最易",
};

const char *Condition::virtual_types_aof[] = {
    "",
    "难度：极难",
    "难度：难",
    "难度：中等",
    "难度：标准",
    "难度：最易",
};

const char *Condition::virtual_types_up[] = {
    "",
    "单人模式 / 允许作弊",
    "嘲讽音效",
    "AI 脚本目标",
    "难度：极难",
    "难度：难",
    "难度：中等",
    "难度：标准",
    "难度：最易",
    "起始时代：标准",
    "起始资源：标准",
    "弑君模式",
    "死亡竞赛",
    //"单击进驻模式",
};

const char *Condition::virtual_types_etp[] = {
    "",
    "单人模式 / 允许作弊",
    "嘲讽音效",
    "AI 脚本目标",
    "难度：极难",
    "难度：难",
    "难度：中等",
    "难度：标准",
    "难度：最易",
    "起始时代：标准",
    "起始资源：标准",
    "弑君模式",
    "死亡竞赛",
    //"单击进驻模式",
};

const char *Condition::virtual_types_cc[] = {
    "",
    "难度：极难",
    "难度：难",
    "难度：中等",
    "难度：标准",
    "难度：最易",
};

const char *Condition::virtual_types_swgb[] = {
    "",
    "难度：极难",
    "难度：难",
    "难度：中等",
    "难度：标准",
    "难度：最易",
};

const char *Condition::taunt_set[] = {
    "1, 2, 3 or 4",
    "5, 6, 7 or 8",
    "9, 10, 11 or 12",
    "13, 14, 15 or 16",
    "17, 18, 19 or 20",
    "21, 22, 23 or 24",
    "25, 26, 27 or 28",
    "29, 30, 31 or 32",
    "33, 34, 35 or 36",
    "37, 38, 39 or 40",
    "41, 42, 43 or 44",
    "45, 46, 47 or 48",
    "49, 50, 51 or 52",
    "53, 54, 55 or 56",
    "57, 58, 59 or 60",
    "61, 62, 63 or 64",
    "65, 66, 67 or 68",
    "69, 70, 71 or 72",
    "73, 74, 75 or 76",
    "77, 78, 79 or 80",
    "81, 82, 83 or 84",
    "85, 86, 87 or 88",
    "89, 90, 91 or 92",
    "93, 94, 95 or 96",
    "97, 98, 99 or 100",
    "101, 102, 103 or 104",
    "105, 106, 107 or 108",
    "109, 110, 111 or 112",
    "113, 114, 115 or 116",
    "117, 118, 119 or 120",
    "121, 122, 123 or 124",
    "125, 126, 127 or 128",
    "129, 130, 131 or 132",
    "133, 134, 135 or 136",
    "137, 138, 139 or 140",
    "141, 142, 143 or 144",
    "145, 146, 147 or 148",
    "149, 150, 151 or 152",
    "153, 154, 155 or 156",
    "157, 158, 159 or 160",
    "161, 162, 163 or 164",
    "165, 166, 167 or 168",
    "169, 170, 171 or 172",
    "173, 174, 175 or 176",
    "177, 178, 179 or 180",
    "181, 182, 183 or 184",
    "185, 186, 187 or 188",
    "189, 190, 191 or 192",
    "193, 194, 195 or 196",
    "197, 198, 199 or 200",
    "201, 202, 203 or 204",
    "205, 206, 207 or 208",
    "209, 210, 211 or 212",
    "213, 214, 215 or 216",
    "217, 218, 219 or 220",
    "221, 222, 223 or 224",
    "225, 226, 227 or 228",
    "229, 230, 231 or 232",
    "233, 234, 235 or 236",
    "237, 238, 239 or 240",
    "241, 242, 243 or 244",
    "245, 246, 247 or 248",
    "249, 250, 251 or 252",
    "253, 254, 255 or 256"
};

const char** Condition::types;
const char** Condition::types_short;
const char** Condition::virtual_types;
