#include "Effect.h"
#include <sstream>
#include <algorithm>
#include "scen.h"
#include "TriggerVisitor.h"

#include "../view/utilunit.h"
#include "../util/utilio.h"
#include "../view/utilui.h"
#include "../util/Buffer.h"
#include "../util/helper.h"
#include "../util/settings.h"
#include "../util/cpp11compat.h"

using std::vector;

extern class Scenario scen;

static const long MAXFIELDS=24;

const long Effect::defaultvals[MAXFIELDS] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

Effect::Effect()
:	ECBase(EFFECT),
	pUnit(NULL),
	pTech(NULL),
	valid_since_last_check(true)
{
    size = MAXFIELDS;
    memcpy(&ai_goal, defaultvals, sizeof(defaultvals));
	memset(uids, -1, sizeof(uids));
}

// Can only delegate (chain) constructors since C++11
Effect::Effect(Buffer &b)
:	ECBase(EFFECT),
	pUnit(NULL),
	pTech(NULL),
	valid_since_last_check(true)
{
    memcpy(&ai_goal, defaultvals, sizeof(defaultvals));
    memset(uids, -1, sizeof(uids));

    ClipboardType::Value cliptype;
    b.read(&cliptype, sizeof(cliptype));
    if (cliptype != ClipboardType::EFFECT)
        throw bad_data_error("效果检测值不正确。");

    b.read(&type, sizeof(type));
    b.read(&size, sizeof(size));

    if (size >= 0) {
        long * flatdata = new long[size];
        b.read(flatdata, sizeof(long) * size);
        memcpy(&ai_goal, flatdata, sizeof(long) * (size<=MAXFIELDS?size:MAXFIELDS));
        delete[] flatdata;
    }

    if (size >= 7)
        pUnit = esdata.units.getByIdSafe(unit_cnst);
    if (size >= 10)
	    pTech = esdata.techs.getByIdSafe(tech_cnst);

	// non-flat data
	text.read(b, sizeof(long));
	sound.read(b, sizeof(long));
	if (num_sel > 0)
		b.read(uids, sizeof(uids));

	check_and_save();
}

void Effect::compress()
{
    if (pUnit)
        unit_cnst = pUnit->id();
    if (pTech)
        tech_cnst = pTech->id();

    size = MAXFIELDS;
    for (int i = size - 1; i >= 0; i--) {
        if (*(&ai_goal + i) == defaultvals[i]) {
            size--;
        } else {
            if (setts.intense)
                printf_log("effect size %d.\n",size);
            break;
        }
    }
}

/* Used forBuffer operations (i.e., copy & paste) */
void Effect::tobuffer(Buffer &b)// const (make it const when unit_cnst gets set elsewhere)
{
    ClipboardType::Value cliptype = ClipboardType::EFFECT;
	b.write(&cliptype, sizeof(cliptype));

	b.write(&type, sizeof(type));
	b.write(&size, sizeof(size));

    if (size >= 7)
        unit_cnst = pUnit ? pUnit->id() : -1;
    if (size >= 10)
	    tech_cnst = pTech ? pTech->id() : -1;

    if (size >= 0 && size <= MAXFIELDS) {
	    b.write(&ai_goal, sizeof(long) * (size<=MAXFIELDS?size:MAXFIELDS));
    }

	text.write(b, sizeof(long));
	sound.write(b, sizeof(long));
	if (num_sel > 0)
		b.write(uids, sizeof(uids));
}

void Effect::read(FILE *in)
{
    memcpy(&ai_goal, defaultvals, sizeof(defaultvals));
    memset(uids, -1, sizeof(uids));

    readbin(in, &type);
    readbin(in, &size);

    if (size < 0 || size > MAXFIELDS)
        throw bad_data_error("效果长度不正确。");

	size_t read = fread(&ai_goal, sizeof(long), size, in);
	if (read != (size_t)size)
	{
		throw bad_data_error(
			(feof(in)) ? "Early EOF" : "stream error");
	}

    if (size >= 7)
        pUnit = esdata.units.getByIdSafe(unit_cnst);
    if (size >= 10)
	    pTech = esdata.techs.getByIdSafe(tech_cnst);

	text.read(in, sizeof(long));
	sound.read(in, sizeof(long));

    // this helped me find why there was heap corruption
    // && num_sel < 22
    // Need a bigger number for max selected units
	if (num_sel > 0)
		readbin(in, uids, num_sel);

    // DODGY! should be removed as soon as bug is fixed
	if ((long)trig_index < 0) {
	    trig_index = -1;
	}

	check_and_save();
}

void Effect::write(FILE *out)
{
    compress();

	writebin(out, &type);
	writebin(out, &size);

    if (size >= 7)
        unit_cnst = pUnit ? pUnit->id() : -1;
    if (size >= 10)
	    tech_cnst = pTech ? pTech->id() : -1;

    if (size >= 0 && size <= MAXFIELDS) {
	    fwrite(&ai_goal, sizeof(long) * size, 1, out);
    }

	text.write(out, sizeof(long), true);
	sound.write(out, sizeof(long), true);
	if (num_sel > 0)
		fwrite(uids, sizeof(long), num_sel, out);
}

std::string pluralizeUnitType(std::string unit_type_name) {
    std::ostringstream convert;
    replaced(unit_type_name, " 组装的", "");
	/*中文不需要复数
    if (!unit_type_name.empty() && *unit_type_name.rbegin() != 's' && !replaced(unit_type_name, "man", "men")) {
        convert << unit_type_name << "s";
    } else {
        convert << unit_type_name;
    }
	*/
	convert << unit_type_name;

    return convert.str();
}

std::string Effect::areaName() const {
    std::ostringstream convert;
    if (valid_area()) {
        if (valid_partial_map()) {
            if (valid_area_location()) {
                convert << "位于 (" << area.left << "," << area.top << ") ";
            } else {
                convert << "位于 (" << area.left << "," << area.bottom << ") 起的 [" << area.right - area.left + 1 << "x" << area.top - area.bottom + 1 << "] 区域内";
            }
        }
    } else {
        convert << "位于 <无效区域> ";
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

std::string Effect::selectedUnits() const {
    std::ostringstream convert;
    bool class_selected = group >= 0 && group < NUM_GROUPS - 1;
    bool unit_type_selected = utype >= 0 && utype < NUM_UTYPES - 1;
    convert << areaName();
    if (s_player >= 0)
        convert << playerPronoun(s_player) << " ";
    if (pUnit && pUnit->id()) {
        if (num_sel > 1) {
            convert << pluralizeUnitType(unitTypeName(pUnit));
        } else {
            convert << unitTypeName(pUnit);
        }
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
	for (int i = 0; i < num_sel; i++) {
        convert << " " << uids[i] << " (" << get_unit_full_name(uids[i]) << ")";
	}
    return convert.str();
}

// functor used for getting a subset of an array of PlayerUnits
class playersunit_ucnst_equals
{
public:
	playersunit_ucnst_equals(UCNST cnst)
		: _cnst(cnst)
	{
	}

	bool operator()(const PlayersUnit& pu)
	{
	    bool sametype = false;
        if (pu.u) {
            const UnitLink * ul = pu.u->getType();
	        if (ul->id() == _cnst) {
	            sametype = true;
	        }
        }
		return (sametype);
	}

private:
	UCNST _cnst;
};

// functor used for getting a subset of an array of PlayerUnits
class playersunit_ucnst_notequals
{
public:
	playersunit_ucnst_notequals(UCNST cnst)
		: _cnst(cnst)
	{
	}

	bool operator()(const PlayersUnit& pu)
	{
	    bool difftype = false;
        if (pu.u) {
            const UnitLink * ul = pu.u->getType();
	        if (ul->id() != _cnst) {
	            difftype = true;
	        }
        }
		return (difftype);
	}

private:
	UCNST _cnst;
};

std::string AbilityType(int ability) //可修改能力列表
{
	std::string abilityname;
	switch ( ability ) {
		case 0: abilityname = "生命值";break;
		case 1: abilityname = "视野半径";break;
		case 2: abilityname = "驻守容量";break;
		case 3: abilityname = "面积半径 1";break;
		case 4: abilityname = "面积半径 2";break;
		case 5: abilityname = "移动速度";break;
		case 6: abilityname = "旋转速度";break;
		case 8: abilityname = "装甲值";break;
		case 9: abilityname = "攻击力";break;
		case 10: abilityname = "攻击间隔";break;
		case 11: abilityname = "命中率";break;
		case 12: abilityname = "最大射程";break;
		case 13: abilityname = "工作效率";break;
		case 14: abilityname = "携带资源量";break;
		case 15: abilityname = "基本装甲";break;
		case 16: abilityname = "投射物编号";break;
		case 17: abilityname = "图示";break;
		case 18: abilityname = "地形防御系数";break;
		case 19: abilityname = "弹道学";break;
		case 20: abilityname = "最小射程";break;
		case 21: abilityname = "储存资源量";break;
		case 22: abilityname = "波及半径";break;
		case 23: abilityname = "索敌半径";break;
		case 80: abilityname = "登舰气力回复速度";break;
		case 100: abilityname = "花费成本";break;
		case 101: abilityname = "建造时间";break;
		case 102: abilityname = "进驻发射箭数";break;
		case 103: abilityname = "食物成本";break;
		case 104: abilityname = "木材成本";break;
		case 105: abilityname = "黄金成本";break;
		case 106: abilityname = "石料成本";break;
		case 107: abilityname = "最大副投射物数量";break;
		case 108: abilityname = "驻守单位回复速率";break;
		case 30: abilityname = "接受进驻类型";break;
		case 31: abilityname = "接纳进驻资源";break;
		case 40: abilityname = "英雄模式";break;
		case 41: abilityname = "帧延迟";break;
		case 42: abilityname = "训练位置";break;
		case 43: abilityname = "训练按钮";break;
		case 44: abilityname = "攻击波及等级";break;
		case 45: abilityname = "自愈间隔";break;
		case 46: abilityname = "显示攻击力";break;
		case 47: abilityname = "显示射程";break;
		case 48: abilityname = "显示近战装甲";break;
		case 49: abilityname = "显示远程装甲";break;
		case 50: abilityname = "名称字串";break;
		case 51: abilityname = "介绍字串";break;
		case 53: abilityname = "地形限制";break;
		case 54: abilityname = "特殊能力";break;
		case 57: abilityname = "尸体单位";break;
		default: abilityname = "<未知能力>";break;
	}
	return abilityname;
}

std::string ArmorType(int armor) //可修改能力列表
{
	std::string armortypename;
	switch ( armor ) {
		case -1: armortypename = "<无>";break;
		case 0: armortypename = "<0>";break;
		case 1: armortypename = "步兵";break;
		case 2: armortypename = "龟船";break;
		case 3: armortypename = "远程";break;
		case 4: armortypename = "近战";break;
		case 5: armortypename = "战象";break;
		case 6: armortypename = "<6>";break;
		case 7: armortypename = "<7>";break;
		case 8: armortypename = "骑兵";break;
		case 9: armortypename = "<9>";break;
		case 10: armortypename = "<无用>";break;
		case 11: armortypename = "建筑物";break;
		case 12: armortypename = "<12>";break;
		case 13: armortypename = "防御建筑";break;
		case 14: armortypename = "猛兽";break;
		case 15: armortypename = "弓兵";break;
		case 16: armortypename = "船舰";break;
		case 17: armortypename = "冲车";break;
		case 18: armortypename = "树木";break;
		case 19: armortypename = "特色单位";break;
		case 20: armortypename = "攻城武器";break;
		case 21: armortypename = "标准建筑";break;
		case 22: armortypename = "城墙";break;
		case 23: armortypename = "火药";break;
		case 24: armortypename = "野猪";break;
		case 25: armortypename = "僧侣";break;
		case 26: armortypename = "城堡";break;
		case 27: armortypename = "长枪兵";break;
		case 28: armortypename = "骑射手";break;
		case 29: armortypename = "鹰勇士";break;
		case 30: armortypename = "骆驼";break;
		case 31: armortypename = "<31>";break;
		case 32: armortypename = "佣兵";break;
		case 33: armortypename = "风琴炮弹";break;
		case 34: armortypename = "渔船";break;
		case 35: armortypename = "马穆鲁克";break;
		default: armortypename = "<未知护甲>";break;
	}
	return armortypename;
}

std::string UpTranslate(std::string text, bool full, bool is_attribute) //up系列效果翻译 虽然想写得不那么硬 还是下次吧（
{
	std::ostringstream temp;	//原文
	std::string trans;			//译文
	char up_value_str[10];		//参数缓存
	int loc = 0;				//字节计数
	int i,j;
	char t = 0;					//行计数
	bool param_missed = 0;		//参数缺失记号
	int tech_attacktype;		//攻击护甲类型

	if(!is_attribute) {

	//==============================up-effect效果====================================================

		bool is_gaia;							//盖亚效果记号
		int tech_para[]={ 0, 0, 0, 0, 0, 0 };	//玩家，科技，对象，属性，数值，比例
		char tech_diplomacy;					//外交状态

		while( loc != text.npos + 10 && t < (full?9:3) ) {
			loc = text.find("up-effect ", loc) + 10;
			if ( loc == text.npos + 10 ) break;
			//==============================参数的取得====================================================
			for(j=0; j<6; j++) {
				while(!(text[loc] == '-' || ( text[loc] >= '0' && text[loc] <= '9' ))) {
					loc++;
					if (text[loc] == '\0' || text[loc] == '\n') {
						param_missed = 1;
						break;
					}
				}
				if (param_missed) {
					param_missed = 0;
					break;
				}
				for( i=0 ; i<10 ; i++)
					up_value_str[i] = '\0';
				for( i=0 ; text[loc + i] == '-' || ( text[loc + i] >= '0' && text[loc + i] <= '9' ) ; i++) {
					if ( i==10 ) { trans="<参数非法>";return trans; }
					up_value_str[i]=text[loc + i];
				}
				loc += i;
				tech_para[j] = atoi(up_value_str);
			}
			if (param_missed) temp << "<参数缺失>";
	
			//==============================参数的分析====================================================
			//判定盖亚
			if (tech_para[1] < 0) {
				is_gaia = 1;
				tech_para[1] = tech_para[1]*(-1) - 1;
			}
			else {
				is_gaia = 0;
			}
			//判定组队
			if (tech_para[1] > 9 && tech_para[1] < 40) {
				tech_diplomacy = tech_para[1] / 10;
				tech_para[1] %= 10;
			}
	
			//判定参数1=======================================================================================
			if (!tech_para[0]) {
				if (is_gaia) {
					temp << "盖亚";
					tech_para[0] = 9;
				}
				else
				{
					temp << "所有人";
				}
			}
			else {
				temp << "玩家" << tech_para[0];
			}
			//判定参数2=====================================================================================
			switch ( tech_diplomacy ) {
			case 1: temp << "和盟友";break;
			case 2: temp << "和中立";break;
			case 3: temp << "和敌军";break;
			default: break;
			}
			temp << " ";
			switch ( tech_para[1] ) {
			case 0:
			case 4:
			case 5: break;
			case 3: temp << "升级 ";break;
			case 2: 
				if (!tech_para[3]) {
					temp << "禁用 ";
					break;
				}
				else {
					temp << "启用 ";
					break;
				}
			case 1:
			case 6:
			case 100:
			case 101:
			case 103: break;
			case 102: temp << "禁用科技 ";break;
			case 7: 
				switch(tech_para[3]) {
				case 0: temp << "禁用";break;
				case 1: temp << "启用";break;
				case 2: temp << "强制启用";break;
				}
				temp << "科技 ";
				break;
			case 8: 
				switch(tech_para[3]) {
				case -1:
				case 0:
				case 1:
				case 2:
				case 3: temp << "设定 ";break;
				case -2:
				case 16384:
				case 16385:
				case 16386:
				case 16387: temp << "增加 ";break;
				}
				break;
			case 9: temp << "玩家名称 ";break;
			default: temp << "<未知> ";break;
			}
			//判定参数3=====================================================================================
			switch ( tech_para[1] ) {
			case 0:
			case 4:
			case 5://单位列表
			case 2:
				if ( tech_para[2] < 1000 && tech_para[2] >= 900 ) {
					temp << groups[tech_para[2] - 900 + 1].name << "种属 ";
				}
				else {
					temp << UnicodeToANSI(esdata.units.getByIdSafe(tech_para[2])->name()) << " ";
				}
				break;
			case 3:temp << UnicodeToANSI(esdata.units.getByIdSafe(tech_para[2])->name()) << " ";break;
			case 1:
			case 6://资源列表
				if (tech_para[2] >= 0) {
					const Link * list = esdata.resources.head();
		            for (int i=0; list; list = list->next(), i++)
		            {
						if (i == tech_para[2]) {
							std::string resname(UnicodeToANSI(list->name()));
							temp << std::string( resname.begin(), resname.end() );
							break;
			            }
		            }
		        }
				temp << " ";
				break;
			case 100:
			case 101:
			case 103:
			case 102:
			case 7://科技列表
			case 8: {
				std::string wtechname;
	            std::string techname;
	            wtechname = std::string(UnicodeToANSI(esdata.techs.getByIdSafe(tech_para[2])->name()));
				techname = std::string(wtechname.begin(), wtechname.end());
				temp << techname;
				   }
				temp << " ";
				break;
			case 9: break;
			default: temp << "<未知> ";break;
			}
			//判定参数4=====================================================================================
			switch ( tech_para[1] ) {
			case 0:
			case 4:
			case 5: {
				switch ( tech_para[3] ) {
				case 8:
				case 9:break;
				default: temp << AbilityType( tech_para[3] );
				}
					}
			case 1:
			case 103:
			case 9:break;
			case 100:
			case 101: {
				if (tech_para[3] >= 0) {
					const Link * list = esdata.resources.head();
		            for (int i=0; list; list = list->next(), i++) {
						if (i == tech_para[3]) {
							std::string resname(UnicodeToANSI(list->name()));
							temp << std::string( resname.begin(), resname.end() );
							break;
			            }
		            }
				}
					  }
					temp << " ";break;
			case 3: temp << "为 " << UnicodeToANSI(esdata.units.getByIdSafe(tech_para[3])->name()) << " ";//升级
			case 2://启用/禁用物件
			case 102://禁用科技
			case 7: break;//启用科技
			case 8://修改科技
				switch(tech_para[3]) {
				case -2:
				case -1:temp << " 研发时间";break;
				case 16384:
				case 0: temp << " 食物成本 ";break;
				case 16385:
				case 1: temp << " 木材成本 ";break;
				case 16386:
				case 2: temp << " 石料成本 ";break;
				case 16387:
				case 3: temp << " 黄金成本 ";break;
				}
				break;
			case 6: break;
			default: temp << "<未知> ";break;
			}
			//参数5=====================================================================================
			switch ( tech_para[1] ) {
			case 0:
			case 4:
			case 5: {
						if ( tech_para[3] == 9 || tech_para[3] == 8) {
							tech_attacktype = tech_para[4] / 256;
							tech_para[4] %= 256;
							temp << ArmorType(tech_attacktype);
							if ( tech_para[3] == 9 )
								temp << "攻击";
							else 
								temp << "护甲";
						}
						switch ( tech_para[1] ) {
						case 0:temp << " 设为 ";break;
						case 4:temp << " 增加 ";break;
						case 5:temp << " 乘以 ";break;
						}
						if (tech_para[5] != 2)temp << tech_para[4];
					} break;
			case 1:
				if ( tech_para[3] ) {
					temp << "增加 ";
					if (tech_para[5] != 2)temp << tech_para[4];
				}
				else {
					temp << "设为 ";
					if (tech_para[5] != 2)temp << tech_para[4];
				}
				break;
			case 6:temp << "乘以 "; if (tech_para[5] != 2)temp << tech_para[4];break;
			case 103:temp << "研发时间设为 ";
					 if (tech_para[5] != 2)
						 temp << tech_para[4] / 100 << "." << (tech_para[4]%100<10&&tech_para[4]%100>-10?"0":"") << (tech_para[4] % 100);
					 else 
						 temp << tech_para[4];
					 temp << "s";break;
			case 9:temp << "设为字串" << tech_para[4] << "号";break;
			case 100:temp << "成本设为 "; if (tech_para[5] != 2)temp << tech_para[4];break;
			case 101:temp << "成本增加 "; if (tech_para[5] != 2)temp << tech_para[4];break;
			case 7:
			case 102: break;
			case 8:temp << "为 "; if (tech_para[5] != 2)temp << tech_para[4];break;
			case 3:
			case 2: break;
			default: temp << "<未知>";break;
			}
			//参数6=====================================================================================
			if (tech_para[5] == 2)
				if (tech_para[1] == 103)
					temp << "<未知>";
				else if (tech_para[1] == 8 || tech_para[1] == 102 || tech_para[1] == 7 || tech_para[1] == 3 || tech_para[1] == 2) ;
				else 
					temp << tech_para[4] / 100 << "." << (tech_para[4]%100<10&&tech_para[4]%100>-10?"0":"") << (tech_para[4] % 100);
			t++;
			if (full)
				temp << "\r\n";
			else
				temp << " | ";
		}
	}
	else {

	//==============================up-attribute效果====================================================
		
		int tech_para[]={ 0, 0, 0, 0 };//模式，属性，数值，比例

		while( loc != text.npos + 13 && t < (full?9:3) ) {
			loc = text.find("up-attribute ", loc) + 13;
			if ( loc == text.npos + 13 ) break;
			//==============================参数的取得====================================================
			for(j=0; j<4; j++) {
				while(!(text[loc] == '-' || ( text[loc] >= '0' && text[loc] <= '9' ))) {
					loc++;
					if (text[loc] == '\0' || text[loc] == '\n') {
						param_missed = 1;
						break;
					}
				}
				if (param_missed) {
					param_missed = 0;
					break;
				}
				for( i=0 ; i<10 ; i++)
					up_value_str[i] = '\0';
				for( i=0 ; text[loc + i] == '-' || ( text[loc + i] >= '0' && text[loc + i] <= '9' ) ; i++) {
					if ( i==10 ) { trans="<参数非法>";return trans; }
					up_value_str[i]=text[loc + i];
				}
				loc += i;
				tech_para[j] = atoi(up_value_str);
			}
			if (param_missed) temp << "<参数缺失>";

			//判定参数1=====================================================================================
			switch ( tech_para[0] ) {
			case 0: //设定单位能力
			case 1: //增减单位能力
			case 2: break;//乘除单位能力
			default: temp << "<未知> ";break;
			}

			//判定参数2=====================================================================================
			switch ( tech_para[0] ) {
			case 0:
			case 1:
			case 2: {
				switch ( tech_para[1] ) {
				case 8:
				case 9:break;
				default: temp << AbilityType( tech_para[1] );
				}
					}
					break;
			default: temp << "<未知> ";break;
			}

			//参数3=====================================================================================
			switch ( tech_para[0] ) {
			case 0:
			case 1:
			case 2: {
						if ( tech_para[1] == 9 || tech_para[1] == 8) {
							tech_attacktype = tech_para[2] / 256;
							tech_para[2] %= 256;
							temp << ArmorType(tech_attacktype);
							if ( tech_para[1] == 9 )
								temp << "攻击";
							else 
								temp << "护甲";
						}
						switch ( tech_para[0] ) {
						case 0:temp << " 设为 ";break;
						case 1:temp << " 增加 ";break;
						case 2:temp << " 乘以 ";break;
						}
						if (tech_para[3] != 2)temp << tech_para[2];break;
					} break;
			default: temp << "<未知>";break;
			}
			//参数4=====================================================================================
			if (tech_para[3] == 2)
				temp << tech_para[2] / 100 << "." << (tech_para[2]%100<10&&tech_para[2]%100>-10?"0":"") << (tech_para[2] % 100);
			t++;
			if (full)
				temp << "\r\n";
			else
				temp << " | ";
		}
	}
	if ( t == (full?9:3) ) temp << " ...";
	trans = temp.str();
	return trans;
}

std::string Effect::getName(bool tip, NameFlags::Value flags, int recursion) const
{
    //printf_log("effect getName() parent trigger= %d.\n",parent_trigger_id);
    if (!tip) {
	    return (type < scen.pergame->max_effect_types) ? getTypeName(type, false) : "<未知>!";
	} else {
        std::string stype = std::string("");
        std::ostringstream convert;
        switch (type) {
        case EffectType::None:
            // Let this act like a separator
            convert << "                                                                                    ";
            stype.append(convert.str());
            break;
        case EffectType::ChangeDiplomacy:
            convert << playerPronoun(s_player);
			convert << " 更改对 ";
            convert << playerPronoun(t_player);
			convert << " 的外交关系为 ";
            switch (diplomacy) {
            case 0:
                convert << "联盟";
                break;
            case 1:
                convert << "中立";
                break;
            case 2:
                convert << "<无效外交关系>";
                break;
            case 3:
                convert << "敌对";
                break;
            }
            stype.append(convert.str());
            break;
        case EffectType::ResearchTechnology:
            {
                bool hastech = pTech && pTech->id();
                std::string wtechname;
                std::string techname;
                if (hastech) {
                    wtechname = std::string(UnicodeToANSI(pTech->name()));
                    techname = std::string(wtechname.begin(), wtechname.end());
                    if (panel >= 1 && panel <= 3) {
                        switch (panel) {
                        case 1:
                            convert << playerPronoun(s_player) << "启用科技" << techname;
                            break;
                        case 2:
                            convert << playerPronoun(s_player) << "禁用科技" << techname;
                            break;
                        case 3:
                            convert << playerPronoun(s_player) << "无视文明启用科技" << techname;
                            break;
                        case 5:
                            convert << playerPronoun(s_player) << "强行启用科技和按钮" << techname;
                            break;
                        }
                    } else {
                        convert << playerPronoun(s_player);
                        convert << "研究科技 " << techname;
                    }
                } else {
                    convert << " <无效> ";
                }
            }
            stype.append(convert.str());
            break;
        case EffectType::SendChat:
            switch (s_player) {
            case -1:
            case 0:
                convert << "向盖亚";
                break;
            default:
                convert << "向玩家" << s_player << " ";
            }
            convert << "送出讯息「" << text.c_str() << "」";
            stype.append(convert.str());
            break;
        case EffectType::Sound:
            convert << "播放音效 " << sound.c_str();
            stype.append(convert.str());
            break;
        case EffectType::SendTribute:
            {
                std::string amount_string;
                if (amount == TS_LONG_MAX || amount == TS_LONG_MIN) {
                    amount_string = "max";
                } else {
                    if (amount < 0) {
                        amount_string = longToString(-amount);
                    } else {
                        amount_string = longToString(amount);
                    }
                }
                if (amount >= 0) {
                    convert << playerPronoun(s_player);
                    if (t_player == 0) {
                        convert << "失去";
                    } else {
                        convert << "进贡";
                        convert << "玩家" << t_player;
                    }
                    convert << " " << amount_string;
                } else {
                    if (t_player == 0) {
                        if (res_type < 4) {
                            convert << "玩家" << s_player << "获得 " << amount_string;
                        } else {
                            convert << "玩家" << s_player << "获得 " << amount_string;
                        }
                    } else {
                        convert << "玩家" << t_player << "静默进贡";
                        convert << playerPronoun(s_player);
                        convert << " " << amount_string;
                    }
                }
                switch (res_type) {
                case 0: // Food
                    convert << "食物";
                    break;
                case 1: // Wood
                    convert << "木材";
                    break;
                case 2: // Stone
                    convert << "石料";
                    break;
                case 3: // Gold
                    convert << "黄金";
                    break;
                case 20: // Units killed
                    convert << "杀敌";
                    break;
                default:
                    //convert << types_short[type];
                    if (res_type >= 0) {
                        const Link * list = esdata.resources.head();
	                    for (int i=0; list; list = list->next(), i++)
	                    {
		                    if (i == res_type) {
                                std::string resname(UnicodeToANSI(list->name()));
                                convert << std::string( resname.begin(), resname.end());
		                        break;
		                    }
	                    }
	                }
                    break;
                }
                if (amount >= 0 && t_player > 0 && res_type < 4) {
                    convert << "（播放进贡音效）";
                }
                stype.append(convert.str());
            }
            break;
        case EffectType::ActivateTrigger:
        case EffectType::DeactivateTrigger:
            //stype.append(types_short[type]);
            if (trig_index != (unsigned)-1 && trig_index != (unsigned)-2) {
                if (trig_index < scen.triggers.size() && trig_index >= 0) {
                    if (trig_index == parent_trigger_id) {
                        switch (type) {
                        case EffectType::ActivateTrigger:
                            stype.append("重复");
                            break;
                        case EffectType::DeactivateTrigger:
                            stype.append("不再重复");
                            break;
                        }
                    } else {
                        if (scen.triggers.at(trig_index).loop) {
                            switch (type) {
                            case EffectType::ActivateTrigger:
                                stype.append("继续");
                                break;
                            case EffectType::DeactivateTrigger:
                                stype.append("暂停");
                                break;
                            }
                        } else {
                            switch (type) {
                            case EffectType::ActivateTrigger:
                                stype.append("激活");
                                break;
                            case EffectType::DeactivateTrigger:
                                stype.append("关闭");
                                break;
                            }
                        }
                        if (setts.showdisplayorder || setts.showtrigids) {
                            stype.append(" ").append(scen.triggers.at(trig_index).getIDName(true));
                        }
                        if (recursion > 0) {
                            stype.append(" ").append(scen.triggers.at(trig_index).getName(setts.showtrigfunction,true,recursion - 1));
                        }
                    }
                } else {
                    switch (type) {
                    case EffectType::ActivateTrigger:
                        stype.append("激活 ");
                        break;
                    case EffectType::DeactivateTrigger:
                        stype.append("关闭 ");
                        break;
                    }
                    stype.append("<?>");
                }
                //convert << trig_index;
                //stype.append(convert.str());
            }
            break;
        case EffectType::AIScriptGoal:
            switch (ai_goal) {
            default:
                if (ai_goal >= -258 && ai_goal <= -3) {
                    // AI Shared Goal
                    convert << "完成 AI 共享信号 " << ai_goal + 258;
                } else if (ai_goal >= 774 && ai_goal <= 1029) {
	                // Set AI Signal
                    convert << "发送 AI 信号(?) " << ai_goal - 774;
                } else {
                    convert << "发送 AI 信号 " << ai_goal;
                }
            }
            stype.append(convert.str());
            break;;
        case EffectType::ChangeView:
            if (location.x >= 0 && location.y >= 0 && s_player >= 1) {
				if ( (scen.game == UP || scen.game == ETP) && panel == 1) {
					convert << "设定玩家" << s_player << " 的视角为 (" << location.x << "," << location.y << ") ";
				}
				else {
					convert << "改变玩家" << s_player << " 的视角到 (" << location.x << "," << location.y << ") ";
				}
            } else {
                convert << " <无效> ";
            }
            stype.append(convert.str());
            break;
        case EffectType::UnlockGate:
        case EffectType::LockGate:
        case EffectType::KillObject:
        case EffectType::RemoveObject:
            switch (type) {
            case EffectType::UnlockGate:
                convert << "解锁";
                break;
            case EffectType::LockGate:
                convert << "锁定";
                break;
            case EffectType::KillObject:
                convert << "摧毁";
                break;
            case EffectType::RemoveObject:
                convert << "移除";
                break;
			}
			convert << selectedUnits();
            stype.append(convert.str());
			break;
        case EffectType::FreezeUnit:
            convert << "设" << selectedUnits() << "为";
			if (scen.game == UP || scen.game == ETP) {
				switch (panel) {
				case 1:
					convert << "进攻状态";
					break;
				case 2:
					convert << "防卫状态";
					break;
				case 3:
					convert << "坚守状态";
					break;
				case 4:
					convert << "不还击状态（无停止效果）";
					break;
				default:
					convert << "不还击状态";
					break;
				}
			} else {
				convert << "不还击状态";
			}
            stype.append(convert.str());
            break;
        case EffectType::Unload:
            convert << "卸载" << selectedUnits() << "到 (" << location.x << "," << location.y << ") ";
            stype.append(convert.str());
            break;

        case EffectType::StopUnit:
            if (panel >= 1 && panel <= 9) {
                convert << "将" << selectedUnits() << "编入" << panel << "号编队";
            } else {
                convert << "停止" << selectedUnits();
            }
            stype.append(convert.str());
            break;
        case EffectType::PlaceFoundation:
            convert << "放置" << playerPronoun(s_player);
            if (pUnit && pUnit->id()) {
                std::string unitname(UnicodeToANSI(pUnit->name()));
                std::string un(unitname.begin(), unitname.end());
                convert << "的" << un;
            } else {
                convert << " <无效效果> ";
            }
            convert << "地基于 (" << location.x << "," << location.y << ") ";
            stype.append(convert.str());
            break;
        case EffectType::CreateObject:
			if (panel == 1 || panel == 2) {
				convert << playerPronoun(s_player);
				if (panel == 1)
					convert << "启用";
				else
					convert << "禁用";
				if (pUnit && pUnit->id()) {
				    std::string unitname(UnicodeToANSI(pUnit->name()));
				    std::string un(unitname.begin(), unitname.end());
				    convert << un;
				} else {
				    convert << " <无效> ";
				}
			}
			else {
				convert << "产生";
				convert << playerPronoun(s_player);
				if (pUnit && pUnit->id()) {
				    std::string unitname(UnicodeToANSI(pUnit->name()));
				    std::string un(unitname.begin(), unitname.end());
				    convert << "的" << un;
				} else {
				    convert << " <无效> ";
				}
				convert << "于 (" << location.x << "," << location.y << ") ";
			}
            stype.append(convert.str());
            break;
        case EffectType::Patrol:
            try
            {
                // keep in mind multiple units can possess the same id, but
                // it only operates on the last farm with that id.
                //
                // s_player may not be selected. Therefore, go through all
                // the units in scenario.
                //
                // A gaia farm could be made infinite.
                //Player * p = scen.players + s_player;
                //for (vector<Unit>::const_iterator iter = p->units.begin(); iter != sorted.end(); ++iter) {
                //}

                if (num_sel > 0) {
                    std::vector<PlayersUnit> all (num_sel);
	                for (int i = 0; i < num_sel; i++) {
                        all[i] = find_map_unit(uids[i]);
	                }
                    std::vector<PlayersUnit> farms (num_sel);
                    std::vector<PlayersUnit> other (num_sel);

                    std::vector<PlayersUnit>::iterator it;
                    it = std::copy_if (all.begin(), all.end(), farms.begin(), playersunit_ucnst_equals(50) );
                    farms.resize(std::distance(farms.begin(),it));  // shrink container to new size

                    it = std::copy_if (all.begin(), all.end(), other.begin(), playersunit_ucnst_notequals(50) );
                    other.resize(std::distance(other.begin(),it));  // shrink container to new size

                    if (farms.size() > 0) {
                        convert << "再生 ";
                        for(std::vector<PlayersUnit>::iterator it = farms.begin(); it != farms.end(); ++it) {
                            convert << it->u->ident << " (" <<
                                get_unit_full_name(it->u->ident)
                                << ")" << " ";
                        }
	                }

                    if (other.size() > 0) {
                        if (!valid_location_coord() && null_location_unit()) {
                            convert << "停止 ";
                        } else {
                            convert << "巡逻 ";
                        }
                        for(std::vector<PlayersUnit>::iterator it = other.begin(); it != other.end(); ++it) {
                            convert << it->u->ident << " (" <<
                                get_unit_full_name(it->u->ident)
                                << ")" << " ";
                        }
	                }
	            } else {
                    convert << "巡逻 / 再生 " << selectedUnits();
	            }
                stype.append(convert.str());
            }
	        catch (std::exception& ex)
	        {
                stype.append(ex.what());
	        }
            break;
        case EffectType::ChangeOwnership:
			if (panel == 1 && (scen.game == UP || scen.game == ETP) ) {
				convert << "闪烁提示";
			}
			else {
				convert << playerPronoun(t_player) << "获得";
			}
			convert << selectedUnits();
            stype.append(convert.str());
            break;
        case EffectType::TaskObject:
            if (!valid_location_coord() && null_location_unit()) {
                convert << selectedUnits() << "停止动作";
                stype.append(convert.str());
                break;
            }
			if (panel == 1 && (scen.game == UP || scen.game == ETP) ) {
				convert << "传送";
			}
			else {
				convert << "指派";
			}
            convert << selectedUnits();
            if (valid_location_coord()) {
                convert << "到 (" << location.x << "," << location.y << ") ";
            } else {
                convert << "到物件" << uid_loc << " (" << get_unit_full_name(uid_loc) << ")";
            }
            stype.append(convert.str());
            break;
        case EffectType::DeclareVictory:
			convert << "玩家" << s_player;
			if (panel == 1 && (scen.game == UP || scen.game == ETP) ) {
				convert << " 投降";
			}
			else {
				convert << " 宣布胜利";
			}
            stype.append(convert.str());
            break;
        case EffectType::DisplayInstructions:
			if ( panel >=9 && disp_time >= 99999 && strstr(text.c_str(),"up-effect ") != NULL && strrchr(text.c_str(), ',') != NULL && *(strrchr(text.c_str(), ',') + 1) != '\0' ) {
				convert << "up-effect " << " |" << strrchr(text.c_str(), ',') + 2 << "| " << UpTranslate(text.c_str(),FALSE,FALSE);
			}
			else {
				convert << "显示信息「" << text.c_str() << "」";
			}
            stype.append(convert.str());
            break;
        case EffectType::ChangeObjectName:
			if ( panel >=1 && strrchr(text.c_str(), ',') != NULL && strstr(text.c_str(),"up-attribute ") != NULL && *(strrchr(text.c_str(), ',') + 1) != '\0' ) {
				convert << "up-attribute 对 " << selectedUnits() << " |" << strrchr(text.c_str(), ',') + 2 << "| " << UpTranslate(text.c_str(),FALSE,TRUE);
			}
			else {
				stype.append("改名为「");
				stype.append(text.c_str());
				stype.append("」");
				convert << " " << selectedUnits();
			}
            stype.append(convert.str());
            break;
        case EffectType::DamageObject:
            {
                if (amount == TS_LONG_MAX) {
                    convert << selectedUnits() << "生命值最小化";
                } else if (amount == TS_LONG_MIN) {
                    convert << "将" << selectedUnits() << "设为无敌";
                } else if (isFloorAmount()) {
                    convert << selectedUnits() << "英雄式回血" << (amount - TS_FLOAT_MIN) << "点（第一步/共两步）";
                } else if (isCeilAmount()) {
                    convert << selectedUnits() << "英雄式回血" << (TS_FLOAT_MAX - amount) << "点（第二步/共两步）";
                } else {
                    if (amount < 0) {
                        convert << "追加" << -amount << "点当前生命值到 " << selectedUnits();
                    } else {
                        convert << "损害" << amount << "点当前生命值到" << selectedUnits();
                    }
                }
                stype.append(convert.str());
            }
            break;
        case EffectType::ChangeObjectHP:
        case EffectType::ChangeObjectAttack:
			//if (panel > 0 && (scen.game == UP || scen.game == ETP) ) 
			switch (panel) {
			case 1: 
				if (scen.game == UP || scen.game == ETP) {
					convert << "设为 ";
					break;
				}
			case 2: 
				if (type == EffectType::ChangeObjectHP && (scen.game == UP || scen.game == ETP) ) {
					convert << "治疗 ";
					break;
				}
			default:
				if (amount > 0) {
				    convert << "+";
				}
				break;
			}
			convert << amount << " " << getTypeName(type, true);
			convert << " 到"  << selectedUnits();
            stype.append(convert.str());
            break;
        case EffectType::ChangeSpeed_UP: // SnapView_SWGB, AttackMove_HD
            switch (scen.game) {
			case ETP:
            case UP:
                if (amount > 0) {
                    convert << "+";
                }
                convert << amount << " " << getTypeName(type, true) << " 到"  << selectedUnits();
                stype.append(convert.str());
                break;
            case SWGB:
            case SWGBCC:
                if (location.x >= 0 && location.y >= 0 && s_player >= 1) {
                    convert << "snap view for p" << s_player << " to " << location.x << "," << location.y << ") ";
                } else {
                    convert << " <无效> ";
                }
                stype.append(convert.str());
                break;
            case AOHD:
            case AOF:
            case AOHD4:
            case AOF4:
            case AOHD6:
            case AOF6:
                if (!valid_location_coord() && null_location_unit()) {
                    convert << "停止" << selectedUnits();
                    stype.append(convert.str());
                    break;
                }

                convert << selectedUnits();
                if (valid_location_coord()) {
					convert << "攻击移动到 (" << location.x << "," << location.y << ") ";
                } else {
                    convert << "向" << uid_loc << " (" << get_unit_full_name(uid_loc) << ")";
					convert << "攻击移动";
                }
                stype.append(convert.str());
                break;
            default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<未知>!");
                break;
            }
            break;
        case EffectType::ChangeRange_UP: // DisableAdvancedButtons_SWGB, ChangeArmor_HD
        case EffectType::ChangeMeleArmor_UP: // ChangeRange_HD, EnableTech_SWGB
        case EffectType::ChangePiercingArmor_UP: // ChangeSpeed_HD, DisableTech_SWGB
            switch (scen.game) {
            case AOF:
            case AOHD:
	        case AOHD4:
	        case AOF4:
	        case AOHD6:
	        case AOF6:
			case ETP:
            case UP:
                if (amount > 0) {
                    convert << "+";
                }
                convert << amount << " " << getTypeName(type, true) << " 到"  << selectedUnits();
                stype.append(convert.str());
                break;
            case SWGB:
            case SWGBCC:
                {
                    switch (type) {
                    case EffectType::DisableAdvancedButtons_SWGB:
                        stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<未知>!");
                        break;
                    case EffectType::EnableTech_SWGB:
                    case EffectType::DisableTech_SWGB:
                        {
                            bool hastech = pTech && pTech->id();
                            std::wstring wtechname;
                            std::string techname;
                            if (hastech) {
                                wtechname = std::wstring(pTech->name());
                                techname = std::string(wtechname.begin(), wtechname.end());
                                switch (type) {
                                case EffectType::EnableTech_SWGB:
                                    convert << "允许 " << playerPronoun(s_player) << " 研发科技 " << techname;
                                    break;
                                case EffectType::DisableTech_SWGB:
                                    convert << "禁止 " << playerPronoun(s_player) << " 研发科技 " << techname;
                                    break;
                                }
                            } else {
                                convert << " <无效科技> ";
                            }
                        }
                        break;
                    }
                }
                stype.append(convert.str());
                break;
            default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<未知>!");
                break;
            }
            break;
	    case EffectType::EnableUnit_SWGB: // EffectType::HealObject_HD:
	        switch (scen.game) {
	        case AOHD:
	        case AOF:
	        case AOHD4:
	        case AOF4:
	        case AOHD6:
	        case AOF6:
                convert << selectedUnits() << "恢复" << amount << "点生命";
                stype.append(convert.str());
	            break;
	        default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<未知>!");
                break;
	        }
            break;
	    case EffectType::FlashUnit_SWGB:   // ChangeUnitStance_HD
	        switch (scen.game) {
	        case AOHD:
	        case AOF:
	        case AOHD4:
	        case AOF4:
	        case AOHD6:
	        case AOF6:
                convert << "设" << selectedUnits() << "为";
                switch (stance) {
                case -1:
                    convert << "空状态";
                    break;
                case 0:
                    convert << "进攻状态";
                    break;
                case 1:
                    convert << "防卫状态";
                    break;
                case 2:
                    convert << "坚守状态";
                    break;
                case 3:
                    convert << "不还击状态";
                    break;
                }
                stype.append(convert.str());
                break;
	        default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<未知>!");
                break;
	        }
	        break;
	    case EffectType::DisableUnit_SWGB: // TeleportObject_HD
	        switch (scen.game) {
	        case AOHD:
	        case AOF:
	        case AOHD4:
	        case AOF4:
	        case AOHD6:
	        case AOF6:
                convert << "将" << selectedUnits() << "之一传送到 (" << location.x << "," << location.y << ") ";
                stype.append(convert.str());
	            break;
	        default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<未知>!");
                break;
	        }
            break;
	    case EffectType::InputOff_CC:
	    case EffectType::InputOn_CC:
            stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<未知>!");
            break;
        default:
            stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<未知>!");
        }

        return flags&NameFlags::LIMITLEN?stype.substr(0,MAX_CHARS):stype;
    }
}

const char * Effect::getTypeName(size_t type, bool concise) const
{
	return concise?types_short[type]:types[type];
}

int Effect::getPlayer() const
{
	extern bool bysource;
	extern bool byauto;
	extern bool changemessage;
	extern bool delcurrent;
	char s_up[] = "up-effect 0"; //源玩家
	char i_up[] = "up-effect 0"; //目标玩家
	std::string text_up; // 显示信息内容
	int up_effect_loc; //查找up-effect效果
	int change_loc; //查找玩家变量
	text_up = (text).c_str();
	up_effect_loc = text_up.find("up-effect ", 0);
	
	if (up_effect_loc != text_up.npos && text_up[up_effect_loc + 10] != '0' && panel >=9 && disp_time >= 99999 )
		return text_up[up_effect_loc + 10];
	else {
		if (changemessage) {
		change_loc = text_up.find("<COLOR>", 0);
		if (change_loc != text_up.npos) {
			delcurrent = 1;
			return 0;
		}
		change_loc = text_up.find("[%p]", 0);
		if (change_loc != text_up.npos) {
			delcurrent = 1;
			return 0;
		}
		}
		else delcurrent = 0;
		if (bysource && !byauto || (t_player == -1 || t_player == 0) && byauto)
			return s_player;
		else
			return t_player;
	}
}

void Effect::setPlayer(int player)
{
	extern bool bysource;
	extern bool byauto;
	if (bysource && !byauto || ( (t_player == -1 || t_player == 0) || s_player != 0 && s_player != -1 ) && byauto)
		s_player = player;
	else
		t_player = player;
}

char * ColorText(int player)
{
	switch (player) {
	case 1: return "<BLUE>" ;
	case 2: return "<RED>" ;
	case 3: return "<GREEN>" ;
	case 4: return "<YELLOW>" ;
	case 5: return "<AQUA>" ;
	case 6: return "<PURPLE>" ;
	case 7: return "<GREY>" ;
	case 8: return "<ORANGE>" ;
	default:return "<COLOR>";
	}
}

void Effect::setText(int player, bool change)
{
	char s_up[] = "up-effect 0"; //源玩家
	char i_up[] = "up-effect 0"; //目标玩家
	char pchar[2];
	pchar[0] = '0' + player;
	pchar[1] = '\0';
	std::string text_up; // 显示信息内容
	int up_effect_loc; //查找up-effect效果
	bool is_up_effect; //是单独玩家的up-effect效果
	text_up = (text).c_str();

	//玩家up-effect效果判定
	up_effect_loc = text_up.find("up-effect ", 0);
	is_up_effect = up_effect_loc != text_up.npos && text_up[up_effect_loc + 10] != '0' && panel >=9 && disp_time >= 99999;

	if (is_up_effect) {
		s_up[10] = text_up[up_effect_loc + 10];
		i_up[10] = player + '0';
		replaceAll(text_up,s_up,i_up);
		text.set(text_up.c_str());
	}
	else if (change) {
		replaceAll(text_up,"<COLOR>",ColorText(player));
		replaceAll(text_up,"[%p]",pchar);
		text.set(text_up.c_str());
	}
}

inline bool Effect::valid_full_map() const {
    return (area.left == -1 && area.right == -1 && area.bottom == -1 && area.top == -1);
}

inline bool Effect::valid_partial_map() const {
    return (area.left >=  0 && area.right >= area.left && area.bottom >=  0 && area.top >= area.bottom);
}

inline bool Effect::valid_area_location() const {
    return area.left == area.right && area.top == area.bottom;
}

inline bool Effect::valid_area() const {
    return valid_full_map() || valid_partial_map();
}

inline bool Effect::has_selected() const {
    return num_sel > 0;
}

inline bool Effect::valid_selected() const {
	for (int i = 0; i < num_sel; i++) {
	    if (!valid_unit_id(uids[i])) {
		    return false;
	    }
	}
	return true;
}

inline bool Effect::has_unit_constant() const {
    return pUnit;
    //return ucnst >= 0;
}

inline bool Effect::valid_unit_constant() const {
    return pUnit && pUnit->id() >= 0;
    //return ucnst >= 0;
}

inline bool Effect::valid_unit_spec() const {
    if (!valid_source_player()) {
        return false;
    }
    if (has_unit_constant() && !valid_unit_constant()) {
        return false;
    }
    //utype >= 0 || group >= 0
    return true;
}

inline bool Effect::valid_technology_spec() const {
	return pTech != NULL && pTech->id() >= 0;;
}

inline bool Effect::valid_location_coord() const {
    return location.x >= 0 && location.y >= 0;
}

inline bool Effect::null_location_unit() const {
	return uid_loc == -1;
}

inline bool Effect::valid_location_unit() const {
	return valid_unit_id(uid_loc);
}

inline bool Effect::valid_source_player() const {
    return s_player >= 0 && s_player <= 8;
}

inline bool Effect::valid_target_player() const {
    return t_player >= 0 && t_player <= 8;
}

inline bool Effect::valid_trigger() const {
    return trig_index >= 0 &&  trig_index < scen.triggers.size();
}

inline bool Effect::valid_panel() const {
	// panel == -1 is acceptable because Azzzru's scripts omit panel
	// to shorten SCX file and scenario still works fine.
    return panel >= -1;
}

inline bool Effect::valid_destination() const {
	return valid_location_coord() || valid_unit_id(uid_loc);
}

/*
 * Amount can be negative, but can't be -1 (or can it? it appears red in
 * scenario editor. It CAN be -1 (in AOHD at least)
 */
inline bool Effect::valid_points() const {
	//return (amount != -1);
	return true;
}

bool Effect::get_valid_since_last_check() {
    return valid_since_last_check;
}

bool Effect::check_and_save()
{
    return valid_since_last_check = check();
}

/*
 * False positives are better than false negatives.
 */
bool Effect::check() const
{
    if (type < 1 || type >= scen.pergame->max_effect_types) {
        return false;
    }

    bool valid_selected = Effect::valid_selected();
    bool has_valid_selected = Effect::has_selected() && valid_selected; // perform this check on all effects
    bool valid_area_selection = valid_unit_spec() && valid_area();

    if (!valid_selected) {
        return false;
    }

	switch (type)
	{
	case EffectType::ChangeDiplomacy:
		return (valid_source_player() && valid_source_player() && diplomacy >= 0);

	case EffectType::ResearchTechnology:
		return (valid_source_player() && valid_technology_spec());

	case EffectType::SendChat:
		return (valid_source_player() && *text.c_str());	//AOK missing text check

	case EffectType::Sound:
		return (valid_source_player() && sound.length());	//AOK missing sound check

	case EffectType::SendTribute:
		return (valid_source_player() && valid_target_player() && res_type >= 0);

	case EffectType::UnlockGate:
	case EffectType::LockGate:
		return has_valid_selected;

	case EffectType::ActivateTrigger:
	case EffectType::DeactivateTrigger:
		return valid_trigger();

	case EffectType::AIScriptGoal:
		return true;

	case EffectType::CreateObject:
		return valid_source_player() && valid_location_coord() && valid_unit_spec();

	case EffectType::Unload:
        return (has_valid_selected || valid_area_selection) && valid_destination();

	case EffectType::TaskObject:
        return (valid_area() || has_valid_selected ||
               (!valid_location_coord() && null_location_unit()));

	case EffectType::KillObject:
	case EffectType::RemoveObject:
	case EffectType::FreezeUnit:
	case EffectType::StopUnit:
	case EffectType::FlashUnit_SWGB:
		return has_valid_selected || valid_area_selection;

	case EffectType::DeclareVictory:
		return valid_source_player();

	case EffectType::ChangeView:
		return valid_source_player() && valid_location_coord();

	case EffectType::ChangeOwnership:
		return (has_valid_selected || valid_area_selection) && valid_target_player();

	case EffectType::Patrol:
		return (has_valid_selected || valid_area_selection) && valid_location_coord();

	case EffectType::DisplayInstructions:
		return (valid_panel() && disp_time >= 0 && (*text.c_str() || textid));	//AOK missing text

	case EffectType::ClearInstructions:
		return valid_panel();

	case EffectType::UseAdvancedButtons:
		return true;

	case EffectType::DamageObject:
	case EffectType::ChangeObjectHP:
	case EffectType::ChangeObjectAttack:
		return (has_valid_selected || valid_area_selection) && valid_points();

	case EffectType::ChangeSpeed_UP: // SnapView_SWGB // AttackMove_HD
	    switch (scen.game) {
		case ETP:
	    case UP:
		    return (has_valid_selected || valid_area_selection) && valid_points();
	    case AOHD:
	    case AOF:
	    case AOHD4:
	    case AOF4:
	    case AOHD6:
	    case AOF6:
            return (valid_area() || has_valid_selected ||
                    (!valid_location_coord() && null_location_unit()));
	    case SWGB:
	    case SWGBCC:
		    return valid_source_player() && valid_location_coord();
	    }
		return valid_location_coord();

	case EffectType::ChangeRange_UP: // DisableAdvancedButtons_SWGB // ChangeArmor_HD
	    switch (scen.game) {
		case ETP:
	    case UP:
	    case AOHD:
	    case AOF:
	    case AOHD4:
	    case AOF4:
	    case AOHD6:
	    case AOF6:
		    return (has_valid_selected || valid_area_selection) && valid_points();
	    case SWGB:
	    case SWGBCC:
		    return true;
	    }
		return valid_location_coord();

	case EffectType::ChangeMeleArmor_UP: // ChangeRange_HD // EnableTech_SWGB
	case EffectType::ChangePiercingArmor_UP: // ChangeSpeed_HD // DisableTech_SWGB
	    switch (scen.game) {
		case ETP:
	    case UP:
	    case AOHD:
	    case AOF:
	    case AOHD4:
	    case AOF4:
	    case AOHD6:
	    case AOF6:
		    return (has_valid_selected || valid_area_selection) && valid_points();
	    case SWGB:
	    case SWGBCC:
		    return valid_technology_spec();
	    }
		return true;

	case EffectType::PlaceFoundation:
		return (valid_source_player() && valid_unit_spec() && valid_location_coord());

	case EffectType::ChangeObjectName:
	    switch (scen.game) {
	    case AOK:
	    case AOC:
	    case AOHD:
	    case AOF:
	    case AOHD4:
	    case AOF4:
	    case AOHD6:
	    case AOF6:
	        return (has_valid_selected || valid_area_selection);
	    }
		return true;

	case EffectType::EnableUnit_SWGB:
	case EffectType::DisableUnit_SWGB:
		return valid_unit_spec();

	default:
		return false;	//unknown effect type
	}
}

void Effect::accept(TriggerVisitor& tv)
{
	tv.visit(*this);
}

const char *Effect::types_aok[] = {
	"",
	"改变外交态度",
	"研究科技",
	"送出交谈讯息",
	"播放音效文件",
	"进贡属性",
	"开启城门",
	"锁闭城门",
	"激活触发",
	"关闭触发",
	"发送AI信号",
	"产生物件",
	"指派物件",
	"宣布胜利",
	"摧毁物件",
	"移除物件",
	"改变视角",
	"卸载单位",
	"改变所有权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结单位",
	"开启高级按钮",
};

const char *Effect::types_aoc[] = {
	"",
	"改变外交态度",
	"研究科技",
	"送出交谈讯息",
	"播放音效文件",
	"进贡属性",
	"开启城门",
	"锁闭城门",
	"激活触发",
	"关闭触发",
	"发送AI信号",
	"产生物件",
	"指派物件",
	"宣布胜利",
	"摧毁物件",
	"移除物件",
	"改变视角",
	"卸载单位",
	"改变所有权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结单位",
	"开启高级按钮",
	"损害物件",
	"放置地基",
	"改变物件名称",
	"改变物件生命值上限",
	"改变物件攻击力",
	"停止单位",
};

const char *Effect::types_up[] = {
	"",
	"改变外交态度",
	"研究科技",
	"送出交谈讯息",
	"播放音效文件",
	"进贡属性",
	"开启城门",
	"锁闭城门",
	"激活触发",
	"关闭触发",
	"发送AI信号",
	"产生物件",
	"指派物件",
	"宣布胜利",
	"摧毁物件",
	"移除物件",
	"改变视角",
	"卸载单位",
	"改变所有权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结单位",
	"开启高级按钮",
	"损害物件",
	"放置地基",
	"改变物件名称",
	"改变物件生命值上限",
	"改变物件攻击力",
	"停止单位",
	"改变物件的速度",
	"改变物件的射程",
	"改变物件的近战防御",
	"改变物件的远程防御"
};

const char *Effect::types_etp[] = {
	"",
	"改变外交态度",
	"研究科技",
	"送出交谈讯息",
	"播放音效文件",
	"进贡属性",
	"开启城门",
	"锁闭城门",
	"激活触发",
	"关闭触发",
	"发送AI信号",
	"产生物件",
	"指派物件",
	"宣布胜利",
	"摧毁物件",
	"移除物件",
	"改变视角",
	"卸载单位",
	"改变所有权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结单位",
	"开启高级按钮",
	"损害物件",
	"放置地基",
	"改变物件名称",
	"改变物件生命值上限",
	"改变物件攻击力",
	"停止单位",
	"改变物件速度",
	"改变物件射程",
	"改变物件近战护甲",
	"改变物件远程护甲",
	"改变物件攻击间隔",
	"关闭高级按钮",
	"改变物件指定类型护甲",
	"改变物件指定类型攻击",
	"改变物件基础护甲",
	"改变资源量",
	"改变物件资源量",
	"改变物件视野",
	"改变物件工作效率",
	"设置物件英雄状态",
	"设置物件图标",
	"停止发送AI信号",
	"改变变量值",
	"清零所有变量",
	"改变资源与变量值",
	"保存变量为文件",
	"从文件中读取变量",
	"警戒",
	"跟随",
	"侦察",
	"显示带参数的信息",
	"送出带参数的交谈讯息",
	"设置带参数的物件名称",
	"生成随机值",
	"从数组抽取随机值",
	"将数值保存到变量",
	"从变量产生物件",
	"改变物件图像",
};

const char *Effect::types_swgb[] = {
	"",
	"Change Alliance",
	"Research Technology",
	"Send Chat",
	"Play Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate Trigger",
	"Deactivate Trigger",
	"AI Script Goal",
	"Create Object",
	"Task Object",
	"Declare Victory",
	"Kill Object (Health=0,deselect)",
	"Remove Object",
	"Scroll View",
	"Unload",
	"Change Ownership",
	"Patrol Units",
	"Display Instructions",
	"Clear Instructions",
	"Freeze Unit (No Attack Stance)",
	"Enable Advanced Buttons",
	"Damage Object (Change Health)",
	"Place Foundation",
	"Change Object Name",
	"Change Object HP (Change Max)",
	"Change Object Attack",
	"Stop Unit",
	"Snap View",
	"Disable Advanced Buttons",
	"Enable Tech",
	"Disable Tech",
	"Enable Unit",
	"Disable Unit",
	"Flash Objects"
};

const char *Effect::types_cc[] = {
	"",
	"改变外交态度",
	"研究科技",
	"送出交谈讯息",
	"播放音效文件",
	"进贡属性",
	"开启城门",
	"锁闭城门",
	"激活触发",
	"关闭触发",
	"发送AI信号",
	"产生物件",
	"指派物件",
	"宣布胜利",
	"摧毁物件",
	"移除物件",
	"改变视角",
	"卸载单位",
	"改变所有权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结单位",
	"开启高级按钮",
	"损害物件",
	"放置地基",
	"改变物件名称",
	"改变物件生命值上限",
	"改变物件攻击力",
	"停止单位",
	"Snap View",
	"Disable Advanced Buttons",
	"Enable Tech",
	"Disable Tech",
	"Enable Unit",
	"Disable Unit",
	"Flash Objects",
	"Input Off",
	"Input On"
};

const char *Effect::types_aohd[] = {
	"",
	"改变外交态度",
	"研究科技",
	"送出交谈讯息",
	"播放音效文件",
	"进贡属性",
	"开启城门",
	"锁闭城门",
	"激活触发",
	"关闭触发",
	"发送AI信号",
	"产生物件",
	"指派物件",
	"宣布胜利",
	"摧毁物件",
	"移除物件",
	"改变视角",
	"卸载单位",
	"改变所有权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结单位",
	"开启高级按钮",
	"损害物件",
	"放置地基",
	"改变物件名称",
	"改变物件生命值上限",
	"改变物件攻击力",
	"停止单位",
	"攻击移动",
	"改变物件护甲值",
	"改变物件的射程",
	"改变物件的速度",
	"治疗物件",
	"传送单个物件",
	"改变单位姿态"
};

const char *Effect::types_aof[] = {
	"",
	"改变外交态度",
	"研究科技",
	"送出交谈讯息",
	"播放音效文件",
	"进贡属性",
	"开启城门",
	"锁闭城门",
	"激活触发",
	"关闭触发",
	"发送AI信号",
	"产生物件",
	"指派物件",
	"宣布胜利",
	"摧毁物件",
	"移除物件",
	"改变视角",
	"卸载单位",
	"改变所有权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结单位",
	"开启高级按钮",
	"损害物件",
	"放置地基",
	"改变物件名称",
	"改变物件生命值上限",
	"改变物件攻击力",
	"停止单位",
	"攻击移动",
	"改变物件护甲值",
	"改变物件的射程",
	"改变物件的速度",
	"治疗物件",
	"传送单个物件",
	"改变单位姿态"
};

const char *Effect::types_short_aok[] = {
	"",
	"改变外交",
	"研究",
	"讯息",
	"音效",
	"进贡",
	"开门",
	"锁门",
	"激活",
	"关闭",
	"AI信号",
	"产生",
	"指派",
	"宣布胜利",
	"摧毁",
	"移除",
	"改变视角",
	"卸载",
	"改权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结",
	"高级按钮",
};

const char *Effect::types_short_aoc[] = {
	"",
	"改变外交",
	"研究",
	"讯息",
	"音效",
	"进贡",
	"开门",
	"锁门",
	"激活",
	"关闭",
	"AI信号",
	"产生",
	"指派",
	"宣布胜利",
	"摧毁",
	"移除",
	"改变视角",
	"卸载",
	"改权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结",
	"高级按钮",
	"损害",
	"地基",
	"改名",
	"生命值",
	"攻击力",
	"停止",
};

const char *Effect::types_short_up[] = {
	"",
	"改变外交",
	"研究",
	"讯息",
	"音效",
	"进贡",
	"开门",
	"锁门",
	"激活",
	"关闭",
	"AI信号",
	"产生",
	"指派",
	"宣布胜利",
	"摧毁",
	"移除",
	"改变视角",
	"卸载",
	"改权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结",
	"高级按钮",
	"损害",
	"地基",
	"改名",
	"生命值",
	"攻击力",
	"停止",
	"速度",
	"射程",
	"近战防御",
	"远程防御"
};

const char *Effect::types_short_etp[] = {
	"",
	"改变外交",
	"研究",
	"讯息",
	"音效",
	"进贡",
	"开门",
	"锁门",
	"激活",
	"关闭",
	"AI信号",
	"产生",
	"指派",
	"宣布胜利",
	"摧毁",
	"移除",
	"改变视角",
	"卸载",
	"改权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结",
	"高级按钮",
	"损害",
	"地基",
	"改名",
	"生命值",
	"攻击力",
	"停止",
	"速度",
	"射程",
	"近战防御",
	"远程防御",
	"攻击间隔",
	"关闭高级按钮",
	"指定类型护甲",
	"指定类型攻击",
	"基础护甲",
	"资源量",
	"物件资源量",
	"视野",
	"工作效率",
	"英雄状态",
	"图标",
	"停止AI信号",
	"变量",
	"清零变量",
	"资源与变量",
	"保存文件",
	"读取文件",
	"警戒",
	"跟随",
	"侦察",
	"参数显示信息",
	"参数讯息",
	"参数改名",
	"随机",
	"数组随机",
	"数值到变量",
	"变量产生物件",
	"物件图像"
};

const char *Effect::types_short_aohd[] = {
	"",
	"改变外交",
	"研究",
	"讯息",
	"音效",
	"进贡",
	"开门",
	"锁门",
	"激活",
	"关闭",
	"AI信号",
	"产生",
	"指派",
	"宣布胜利",
	"摧毁",
	"移除",
	"改变视角",
	"卸载",
	"改权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结",
	"高级按钮",
	"损害",
	"地基",
	"改名",
	"生命值",
	"攻击力",
	"停止",
	"攻击移动",
	"护甲",
	"射程",
	"速度",
	"治疗",
	"传送",
	"改变姿态"
};

const char *Effect::types_short_aof[] = {
	"",
	"改变外交",
	"研究",
	"讯息",
	"音效",
	"进贡",
	"开门",
	"锁门",
	"激活",
	"关闭",
	"AI信号",
	"产生",
	"指派",
	"宣布胜利",
	"摧毁",
	"移除",
	"改变视角",
	"卸载",
	"改权",
	"巡逻",
	"显示信息",
	"清除信息",
	"冻结",
	"高级按钮",
	"损害",
	"地基",
	"改名",
	"生命值",
	"攻击力",
	"停止",
	"攻击移动",
	"护甲",
	"射程",
	"速度",
	"治疗",
	"传送",
	"改变姿态"
};

const char *Effect::types_short_swgb[] = {
	"",
	"Change Alliance",
	"Research",
	"Chat",
	"Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate",
	"Deactivate",
	"AI Script Goal",
	"Create",
	"Task",
	"Declare Victory",
	"Kill",
	"Remove",
	"Scroll View",
	"Unload",
	"Change Ownership",
	"Patrol",
	"Instructions",
	"Clear Instructions",
	"Freeze (No Attack)",
	"Use Advanced Buttons",
	"Damage",
	"Place Foundation",
	"Rename",
	"HP",
	"Attack",
	"Stop Unit",
	"Snap View",
	"Disable Advanced",
	"Enable Tech",
	"Disable Tech",
	"Enable Unit",
	"Disable Unit",
	"Flash"
};

const char *Effect::types_short_cc[] = {
	"",
	"Change Alliance",
	"Research",
	"Chat",
	"Sound",
	"Tribute",
	"Unlock Gate",
	"Lock Gate",
	"Activate",
	"Deactivate",
	"AI Script Goal",
	"Create",
	"Task",
	"Declare Victory",
	"Kill",
	"Remove",
	"Scroll View",
	"Unload",
	"Change Ownership",
	"Patrol",
	"Instructions",
	"Clear Instructions",
	"Freeze (No Attack)",
	"Use Advanced Buttons",
	"Damage",
	"Place Foundation",
	"Rename",
	"HP",
	"Attack",
	"Stop Unit",
	"Snap View",
	"Disable Advanced",
	"Enable Tech",
	"Disable Tech",
	"Enable Unit",
	"Disable Unit",
	"Flash",
	"Input Off",
	"Input On"
};

const char *Effect::virtual_types_up[] = {
    "",
    "启用物件",
    "禁用物件",
    "启用科技",
    "禁用科技",
    "启用科技（无视文明）",
    "启用科技和按钮（无视文明）",
	"传送物件",
    "设定生命值",
    "治疗物件",
    "进攻状态",
    "防卫状态",
    "坚守状态",
    "不还击状态",
    "投降",
    "闪烁物件",
    "设定攻击力",
    "编入编队 1",
    "编入编队 2",
    "编入编队 3",
    "编入编队 4",
    "编入编队 5",
    "编入编队 6",
    "编入编队 7",
    "编入编队 8",
    "编入编队 9",
    "设定视角",
    "最大数量",
    "最小数量",
    "英雄式回血第 1 步",
    "英雄式回血第 2 步",
	"up-attribute 扩展效果",
	"up-effect 扩展效果",
    "设定 AI 信号",
    "设定 AI 共享目标",
    "允许作弊",
};

const char *Effect::virtual_types_etp[] = {
    "",
    "启用物件",
    "禁用物件",
    "启用科技",
    "禁用科技",
    "启用科技（无视文明）",
    "启用科技和按钮（无视文明）",
	"传送物件",
    "设定生命值",
    "治疗物件",
    "进攻状态",
    "防卫状态",
    "坚守状态",
    "不还击状态",
    "投降",
    "闪烁物件",
    "设定攻击力",
    "编入编队 1",
    "编入编队 2",
    "编入编队 3",
    "编入编队 4",
    "编入编队 5",
    "编入编队 6",
    "编入编队 7",
    "编入编队 8",
    "编入编队 9",
    "设定视角",
    "最大数量",
    "最小数量",
    "英雄式回血第 1 步",
    "英雄式回血第 2 步",
	"up-attribute 扩展效果",
	"up-effect 扩展效果",
    "设定 AI 信号",
    "设定 AI 共享目标",
    "允许作弊",
};

const char *Effect::virtual_types_aoc[] = {
    "",
    "最大数量",
    "最小数量",
    "英雄式回血第 1 步",
    "英雄式回血第 2 步",
    "设定 AI 信号",
    "设定 AI 共享目标",
    "允许作弊",
    "冻结单位",
};

const char *Effect::virtual_types_aohd[] = {
    "",
    "最大数量",
    "最小数量",
    "英雄式回血第 1 步",
    "英雄式回血第 2 步",
    "冻结单位",
};

const char *Effect::virtual_types_aof[] = {
    "",
    "最大数量",
    "最小数量",
    "英雄式回血第 1 步",
    "英雄式回血第 2 步",
    "冻结单位",
};

const char *Effect::virtual_types_swgb[] = {
    "",
    "最大数量",
    "最小数量",
    "英雄式回血第 1 步",
    "英雄式回血第 2 步",
    "冻结单位",
};

const char *Effect::virtual_types_cc[] = {
    "",
    "最大数量",
    "最小数量",
    "英雄式回血第 1 步",
    "英雄式回血第 2 步",
    "冻结单位",
};

const char *Effect::virtual_types_aok[] = {
    "",
    "最大数量",
    "最小数量",
};

bool Effect::isCeilAmount() const {
    return amount < TS_FLOAT_MAX && amount >= (TS_FLOAT_MAX - TS_HP_MAX);
}

bool Effect::isFloorAmount() const {
    return amount > TS_FLOAT_MIN && amount <= (TS_FLOAT_MIN + TS_HP_MAX);
}

const char** Effect::types;
const char** Effect::types_short;
const char** Effect::virtual_types;
