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
        throw bad_data_error("Ч�����ֵ����ȷ��");

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
        throw bad_data_error("Ч�����Ȳ���ȷ��");

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
    replaced(unit_type_name, " ��װ��", "");
	/*���Ĳ���Ҫ����
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
                convert << "λ�� (" << area.left << "," << area.top << ") ";
            } else {
                convert << "λ�� (" << area.left << "," << area.bottom << ") ��� [" << area.right - area.left + 1 << "x" << area.top - area.bottom + 1 << "] ������";
            }
        }
    } else {
        convert << "λ�� <��Ч����> ";
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
        convert << "��λ";
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
            convert << "����" << groups[group + 1].name;
            convert << "��" << utypes[utype + 1].name;
			convert << "��λ";
        } else if (class_selected) {
            convert << groups[group + 1].name;
			convert << "����";
        } else {
            convert << utypes[utype + 1].name;
			convert << "��λ";
        }
    } else {
        convert << "��λ";
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

std::string AbilityType(int ability) //���޸������б�
{
	std::string abilityname;
	switch ( ability ) {
		case 0: abilityname = "����ֵ";break;
		case 1: abilityname = "��Ұ�뾶";break;
		case 2: abilityname = "פ������";break;
		case 3: abilityname = "����뾶 1";break;
		case 4: abilityname = "����뾶 2";break;
		case 5: abilityname = "�ƶ��ٶ�";break;
		case 6: abilityname = "��ת�ٶ�";break;
		case 8: abilityname = "װ��ֵ";break;
		case 9: abilityname = "������";break;
		case 10: abilityname = "�������";break;
		case 11: abilityname = "������";break;
		case 12: abilityname = "������";break;
		case 13: abilityname = "����Ч��";break;
		case 14: abilityname = "Я����Դ��";break;
		case 15: abilityname = "����װ��";break;
		case 16: abilityname = "Ͷ������";break;
		case 17: abilityname = "ͼʾ";break;
		case 18: abilityname = "���η���ϵ��";break;
		case 19: abilityname = "����ѧ";break;
		case 20: abilityname = "��С���";break;
		case 21: abilityname = "������Դ��";break;
		case 22: abilityname = "�����뾶";break;
		case 23: abilityname = "���а뾶";break;
		case 80: abilityname = "�ǽ������ظ��ٶ�";break;
		case 100: abilityname = "���ѳɱ�";break;
		case 101: abilityname = "����ʱ��";break;
		case 102: abilityname = "��פ�������";break;
		case 103: abilityname = "ʳ��ɱ�";break;
		case 104: abilityname = "ľ�ĳɱ�";break;
		case 105: abilityname = "�ƽ�ɱ�";break;
		case 106: abilityname = "ʯ�ϳɱ�";break;
		case 107: abilityname = "���Ͷ��������";break;
		case 108: abilityname = "פ�ص�λ�ظ�����";break;
		case 30: abilityname = "���ܽ�פ����";break;
		case 31: abilityname = "���ɽ�פ��Դ";break;
		case 40: abilityname = "Ӣ��ģʽ";break;
		case 41: abilityname = "֡�ӳ�";break;
		case 42: abilityname = "ѵ��λ��";break;
		case 43: abilityname = "ѵ����ť";break;
		case 44: abilityname = "���������ȼ�";break;
		case 45: abilityname = "�������";break;
		case 46: abilityname = "��ʾ������";break;
		case 47: abilityname = "��ʾ���";break;
		case 48: abilityname = "��ʾ��սװ��";break;
		case 49: abilityname = "��ʾԶ��װ��";break;
		case 50: abilityname = "�����ִ�";break;
		case 51: abilityname = "�����ִ�";break;
		case 53: abilityname = "��������";break;
		case 54: abilityname = "��������";break;
		case 57: abilityname = "ʬ�嵥λ";break;
		default: abilityname = "<δ֪����>";break;
	}
	return abilityname;
}

std::string ArmorType(int armor) //���޸������б�
{
	std::string armortypename;
	switch ( armor ) {
		case -1: armortypename = "<��>";break;
		case 0: armortypename = "<0>";break;
		case 1: armortypename = "����";break;
		case 2: armortypename = "�괬";break;
		case 3: armortypename = "Զ��";break;
		case 4: armortypename = "��ս";break;
		case 5: armortypename = "ս��";break;
		case 6: armortypename = "<6>";break;
		case 7: armortypename = "<7>";break;
		case 8: armortypename = "���";break;
		case 9: armortypename = "<9>";break;
		case 10: armortypename = "<����>";break;
		case 11: armortypename = "������";break;
		case 12: armortypename = "<12>";break;
		case 13: armortypename = "��������";break;
		case 14: armortypename = "����";break;
		case 15: armortypename = "����";break;
		case 16: armortypename = "����";break;
		case 17: armortypename = "�峵";break;
		case 18: armortypename = "��ľ";break;
		case 19: armortypename = "��ɫ��λ";break;
		case 20: armortypename = "��������";break;
		case 21: armortypename = "��׼����";break;
		case 22: armortypename = "��ǽ";break;
		case 23: armortypename = "��ҩ";break;
		case 24: armortypename = "Ұ��";break;
		case 25: armortypename = "ɮ��";break;
		case 26: armortypename = "�Ǳ�";break;
		case 27: armortypename = "��ǹ��";break;
		case 28: armortypename = "������";break;
		case 29: armortypename = "ӥ��ʿ";break;
		case 30: armortypename = "����";break;
		case 31: armortypename = "<31>";break;
		case 32: armortypename = "Ӷ��";break;
		case 33: armortypename = "�����ڵ�";break;
		case 34: armortypename = "�洬";break;
		case 35: armortypename = "����³��";break;
		default: armortypename = "<δ֪����>";break;
	}
	return armortypename;
}

std::string UpTranslate(std::string text, bool full, bool is_attribute) //upϵ��Ч������ ��Ȼ��д�ò���ôӲ �����´ΰɣ�
{
	std::ostringstream temp;	//ԭ��
	std::string trans;			//����
	char up_value_str[10];		//��������
	int loc = 0;				//�ֽڼ���
	int i,j;
	char t = 0;					//�м���
	bool param_missed = 0;		//����ȱʧ�Ǻ�
	int tech_attacktype;		//������������

	if(!is_attribute) {

	//==============================up-effectЧ��====================================================

		bool is_gaia;							//����Ч���Ǻ�
		int tech_para[]={ 0, 0, 0, 0, 0, 0 };	//��ң��Ƽ����������ԣ���ֵ������
		char tech_diplomacy;					//�⽻״̬

		while( loc != text.npos + 10 && t < (full?9:3) ) {
			loc = text.find("up-effect ", loc) + 10;
			if ( loc == text.npos + 10 ) break;
			//==============================������ȡ��====================================================
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
					if ( i==10 ) { trans="<�����Ƿ�>";return trans; }
					up_value_str[i]=text[loc + i];
				}
				loc += i;
				tech_para[j] = atoi(up_value_str);
			}
			if (param_missed) temp << "<����ȱʧ>";
	
			//==============================�����ķ���====================================================
			//�ж�����
			if (tech_para[1] < 0) {
				is_gaia = 1;
				tech_para[1] = tech_para[1]*(-1) - 1;
			}
			else {
				is_gaia = 0;
			}
			//�ж����
			if (tech_para[1] > 9 && tech_para[1] < 40) {
				tech_diplomacy = tech_para[1] / 10;
				tech_para[1] %= 10;
			}
	
			//�ж�����1=======================================================================================
			if (!tech_para[0]) {
				if (is_gaia) {
					temp << "����";
					tech_para[0] = 9;
				}
				else
				{
					temp << "������";
				}
			}
			else {
				temp << "���" << tech_para[0];
			}
			//�ж�����2=====================================================================================
			switch ( tech_diplomacy ) {
			case 1: temp << "������";break;
			case 2: temp << "������";break;
			case 3: temp << "�͵о�";break;
			default: break;
			}
			temp << " ";
			switch ( tech_para[1] ) {
			case 0:
			case 4:
			case 5: break;
			case 3: temp << "���� ";break;
			case 2: 
				if (!tech_para[3]) {
					temp << "���� ";
					break;
				}
				else {
					temp << "���� ";
					break;
				}
			case 1:
			case 6:
			case 100:
			case 101:
			case 103: break;
			case 102: temp << "���ÿƼ� ";break;
			case 7: 
				switch(tech_para[3]) {
				case 0: temp << "����";break;
				case 1: temp << "����";break;
				case 2: temp << "ǿ������";break;
				}
				temp << "�Ƽ� ";
				break;
			case 8: 
				switch(tech_para[3]) {
				case -1:
				case 0:
				case 1:
				case 2:
				case 3: temp << "�趨 ";break;
				case -2:
				case 16384:
				case 16385:
				case 16386:
				case 16387: temp << "���� ";break;
				}
				break;
			case 9: temp << "������� ";break;
			default: temp << "<δ֪> ";break;
			}
			//�ж�����3=====================================================================================
			switch ( tech_para[1] ) {
			case 0:
			case 4:
			case 5://��λ�б�
			case 2:
				if ( tech_para[2] < 1000 && tech_para[2] >= 900 ) {
					temp << groups[tech_para[2] - 900 + 1].name << "���� ";
				}
				else {
					temp << UnicodeToANSI(esdata.units.getByIdSafe(tech_para[2])->name()) << " ";
				}
				break;
			case 3:temp << UnicodeToANSI(esdata.units.getByIdSafe(tech_para[2])->name()) << " ";break;
			case 1:
			case 6://��Դ�б�
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
			case 7://�Ƽ��б�
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
			default: temp << "<δ֪> ";break;
			}
			//�ж�����4=====================================================================================
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
			case 3: temp << "Ϊ " << UnicodeToANSI(esdata.units.getByIdSafe(tech_para[3])->name()) << " ";//����
			case 2://����/�������
			case 102://���ÿƼ�
			case 7: break;//���ÿƼ�
			case 8://�޸ĿƼ�
				switch(tech_para[3]) {
				case -2:
				case -1:temp << " �з�ʱ��";break;
				case 16384:
				case 0: temp << " ʳ��ɱ� ";break;
				case 16385:
				case 1: temp << " ľ�ĳɱ� ";break;
				case 16386:
				case 2: temp << " ʯ�ϳɱ� ";break;
				case 16387:
				case 3: temp << " �ƽ�ɱ� ";break;
				}
				break;
			case 6: break;
			default: temp << "<δ֪> ";break;
			}
			//����5=====================================================================================
			switch ( tech_para[1] ) {
			case 0:
			case 4:
			case 5: {
						if ( tech_para[3] == 9 || tech_para[3] == 8) {
							tech_attacktype = tech_para[4] / 256;
							tech_para[4] %= 256;
							temp << ArmorType(tech_attacktype);
							if ( tech_para[3] == 9 )
								temp << "����";
							else 
								temp << "����";
						}
						switch ( tech_para[1] ) {
						case 0:temp << " ��Ϊ ";break;
						case 4:temp << " ���� ";break;
						case 5:temp << " ���� ";break;
						}
						if (tech_para[5] != 2)temp << tech_para[4];
					} break;
			case 1:
				if ( tech_para[3] ) {
					temp << "���� ";
					if (tech_para[5] != 2)temp << tech_para[4];
				}
				else {
					temp << "��Ϊ ";
					if (tech_para[5] != 2)temp << tech_para[4];
				}
				break;
			case 6:temp << "���� "; if (tech_para[5] != 2)temp << tech_para[4];break;
			case 103:temp << "�з�ʱ����Ϊ ";
					 if (tech_para[5] != 2)
						 temp << tech_para[4] / 100 << "." << (tech_para[4]%100<10&&tech_para[4]%100>-10?"0":"") << (tech_para[4] % 100);
					 else 
						 temp << tech_para[4];
					 temp << "s";break;
			case 9:temp << "��Ϊ�ִ�" << tech_para[4] << "��";break;
			case 100:temp << "�ɱ���Ϊ "; if (tech_para[5] != 2)temp << tech_para[4];break;
			case 101:temp << "�ɱ����� "; if (tech_para[5] != 2)temp << tech_para[4];break;
			case 7:
			case 102: break;
			case 8:temp << "Ϊ "; if (tech_para[5] != 2)temp << tech_para[4];break;
			case 3:
			case 2: break;
			default: temp << "<δ֪>";break;
			}
			//����6=====================================================================================
			if (tech_para[5] == 2)
				if (tech_para[1] == 103)
					temp << "<δ֪>";
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

	//==============================up-attributeЧ��====================================================
		
		int tech_para[]={ 0, 0, 0, 0 };//ģʽ�����ԣ���ֵ������

		while( loc != text.npos + 13 && t < (full?9:3) ) {
			loc = text.find("up-attribute ", loc) + 13;
			if ( loc == text.npos + 13 ) break;
			//==============================������ȡ��====================================================
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
					if ( i==10 ) { trans="<�����Ƿ�>";return trans; }
					up_value_str[i]=text[loc + i];
				}
				loc += i;
				tech_para[j] = atoi(up_value_str);
			}
			if (param_missed) temp << "<����ȱʧ>";

			//�ж�����1=====================================================================================
			switch ( tech_para[0] ) {
			case 0: //�趨��λ����
			case 1: //������λ����
			case 2: break;//�˳���λ����
			default: temp << "<δ֪> ";break;
			}

			//�ж�����2=====================================================================================
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
			default: temp << "<δ֪> ";break;
			}

			//����3=====================================================================================
			switch ( tech_para[0] ) {
			case 0:
			case 1:
			case 2: {
						if ( tech_para[1] == 9 || tech_para[1] == 8) {
							tech_attacktype = tech_para[2] / 256;
							tech_para[2] %= 256;
							temp << ArmorType(tech_attacktype);
							if ( tech_para[1] == 9 )
								temp << "����";
							else 
								temp << "����";
						}
						switch ( tech_para[0] ) {
						case 0:temp << " ��Ϊ ";break;
						case 1:temp << " ���� ";break;
						case 2:temp << " ���� ";break;
						}
						if (tech_para[3] != 2)temp << tech_para[2];break;
					} break;
			default: temp << "<δ֪>";break;
			}
			//����4=====================================================================================
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
	    return (type < scen.pergame->max_effect_types) ? getTypeName(type, false) : "<δ֪>!";
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
			convert << " ���Ķ� ";
            convert << playerPronoun(t_player);
			convert << " ���⽻��ϵΪ ";
            switch (diplomacy) {
            case 0:
                convert << "����";
                break;
            case 1:
                convert << "����";
                break;
            case 2:
                convert << "<��Ч�⽻��ϵ>";
                break;
            case 3:
                convert << "�ж�";
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
                            convert << playerPronoun(s_player) << "���ÿƼ�" << techname;
                            break;
                        case 2:
                            convert << playerPronoun(s_player) << "���ÿƼ�" << techname;
                            break;
                        case 3:
                            convert << playerPronoun(s_player) << "�����������ÿƼ�" << techname;
                            break;
                        case 5:
                            convert << playerPronoun(s_player) << "ǿ�����ÿƼ��Ͱ�ť" << techname;
                            break;
                        }
                    } else {
                        convert << playerPronoun(s_player);
                        convert << "�о��Ƽ� " << techname;
                    }
                } else {
                    convert << " <��Ч> ";
                }
            }
            stype.append(convert.str());
            break;
        case EffectType::SendChat:
            switch (s_player) {
            case -1:
            case 0:
                convert << "�����";
                break;
            default:
                convert << "�����" << s_player << " ";
            }
            convert << "�ͳ�ѶϢ��" << text.c_str() << "��";
            stype.append(convert.str());
            break;
        case EffectType::Sound:
            convert << "������Ч " << sound.c_str();
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
                        convert << "ʧȥ";
                    } else {
                        convert << "����";
                        convert << "���" << t_player;
                    }
                    convert << " " << amount_string;
                } else {
                    if (t_player == 0) {
                        if (res_type < 4) {
                            convert << "���" << s_player << "��� " << amount_string;
                        } else {
                            convert << "���" << s_player << "��� " << amount_string;
                        }
                    } else {
                        convert << "���" << t_player << "��Ĭ����";
                        convert << playerPronoun(s_player);
                        convert << " " << amount_string;
                    }
                }
                switch (res_type) {
                case 0: // Food
                    convert << "ʳ��";
                    break;
                case 1: // Wood
                    convert << "ľ��";
                    break;
                case 2: // Stone
                    convert << "ʯ��";
                    break;
                case 3: // Gold
                    convert << "�ƽ�";
                    break;
                case 20: // Units killed
                    convert << "ɱ��";
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
                    convert << "�����Ž�����Ч��";
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
                            stype.append("�ظ�");
                            break;
                        case EffectType::DeactivateTrigger:
                            stype.append("�����ظ�");
                            break;
                        }
                    } else {
                        if (scen.triggers.at(trig_index).loop) {
                            switch (type) {
                            case EffectType::ActivateTrigger:
                                stype.append("����");
                                break;
                            case EffectType::DeactivateTrigger:
                                stype.append("��ͣ");
                                break;
                            }
                        } else {
                            switch (type) {
                            case EffectType::ActivateTrigger:
                                stype.append("����");
                                break;
                            case EffectType::DeactivateTrigger:
                                stype.append("�ر�");
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
                        stype.append("���� ");
                        break;
                    case EffectType::DeactivateTrigger:
                        stype.append("�ر� ");
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
                    convert << "��� AI �����ź� " << ai_goal + 258;
                } else if (ai_goal >= 774 && ai_goal <= 1029) {
	                // Set AI Signal
                    convert << "���� AI �ź�(?) " << ai_goal - 774;
                } else {
                    convert << "���� AI �ź� " << ai_goal;
                }
            }
            stype.append(convert.str());
            break;;
        case EffectType::ChangeView:
            if (location.x >= 0 && location.y >= 0 && s_player >= 1) {
				if ( (scen.game == UP || scen.game == ETP) && panel == 1) {
					convert << "�趨���" << s_player << " ���ӽ�Ϊ (" << location.x << "," << location.y << ") ";
				}
				else {
					convert << "�ı����" << s_player << " ���ӽǵ� (" << location.x << "," << location.y << ") ";
				}
            } else {
                convert << " <��Ч> ";
            }
            stype.append(convert.str());
            break;
        case EffectType::UnlockGate:
        case EffectType::LockGate:
        case EffectType::KillObject:
        case EffectType::RemoveObject:
            switch (type) {
            case EffectType::UnlockGate:
                convert << "����";
                break;
            case EffectType::LockGate:
                convert << "����";
                break;
            case EffectType::KillObject:
                convert << "�ݻ�";
                break;
            case EffectType::RemoveObject:
                convert << "�Ƴ�";
                break;
			}
			convert << selectedUnits();
            stype.append(convert.str());
			break;
        case EffectType::FreezeUnit:
            convert << "��" << selectedUnits() << "Ϊ";
			if (scen.game == UP || scen.game == ETP) {
				switch (panel) {
				case 1:
					convert << "����״̬";
					break;
				case 2:
					convert << "����״̬";
					break;
				case 3:
					convert << "����״̬";
					break;
				case 4:
					convert << "������״̬����ֹͣЧ����";
					break;
				default:
					convert << "������״̬";
					break;
				}
			} else {
				convert << "������״̬";
			}
            stype.append(convert.str());
            break;
        case EffectType::Unload:
            convert << "ж��" << selectedUnits() << "�� (" << location.x << "," << location.y << ") ";
            stype.append(convert.str());
            break;

        case EffectType::StopUnit:
            if (panel >= 1 && panel <= 9) {
                convert << "��" << selectedUnits() << "����" << panel << "�ű��";
            } else {
                convert << "ֹͣ" << selectedUnits();
            }
            stype.append(convert.str());
            break;
        case EffectType::PlaceFoundation:
            convert << "����" << playerPronoun(s_player);
            if (pUnit && pUnit->id()) {
                std::string unitname(UnicodeToANSI(pUnit->name()));
                std::string un(unitname.begin(), unitname.end());
                convert << "��" << un;
            } else {
                convert << " <��ЧЧ��> ";
            }
            convert << "�ػ��� (" << location.x << "," << location.y << ") ";
            stype.append(convert.str());
            break;
        case EffectType::CreateObject:
			if (panel == 1 || panel == 2) {
				convert << playerPronoun(s_player);
				if (panel == 1)
					convert << "����";
				else
					convert << "����";
				if (pUnit && pUnit->id()) {
				    std::string unitname(UnicodeToANSI(pUnit->name()));
				    std::string un(unitname.begin(), unitname.end());
				    convert << un;
				} else {
				    convert << " <��Ч> ";
				}
			}
			else {
				convert << "����";
				convert << playerPronoun(s_player);
				if (pUnit && pUnit->id()) {
				    std::string unitname(UnicodeToANSI(pUnit->name()));
				    std::string un(unitname.begin(), unitname.end());
				    convert << "��" << un;
				} else {
				    convert << " <��Ч> ";
				}
				convert << "�� (" << location.x << "," << location.y << ") ";
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
                        convert << "���� ";
                        for(std::vector<PlayersUnit>::iterator it = farms.begin(); it != farms.end(); ++it) {
                            convert << it->u->ident << " (" <<
                                get_unit_full_name(it->u->ident)
                                << ")" << " ";
                        }
	                }

                    if (other.size() > 0) {
                        if (!valid_location_coord() && null_location_unit()) {
                            convert << "ֹͣ ";
                        } else {
                            convert << "Ѳ�� ";
                        }
                        for(std::vector<PlayersUnit>::iterator it = other.begin(); it != other.end(); ++it) {
                            convert << it->u->ident << " (" <<
                                get_unit_full_name(it->u->ident)
                                << ")" << " ";
                        }
	                }
	            } else {
                    convert << "Ѳ�� / ���� " << selectedUnits();
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
				convert << "��˸��ʾ";
			}
			else {
				convert << playerPronoun(t_player) << "���";
			}
			convert << selectedUnits();
            stype.append(convert.str());
            break;
        case EffectType::TaskObject:
            if (!valid_location_coord() && null_location_unit()) {
                convert << selectedUnits() << "ֹͣ����";
                stype.append(convert.str());
                break;
            }
			if (panel == 1 && (scen.game == UP || scen.game == ETP) ) {
				convert << "����";
			}
			else {
				convert << "ָ��";
			}
            convert << selectedUnits();
            if (valid_location_coord()) {
                convert << "�� (" << location.x << "," << location.y << ") ";
            } else {
                convert << "�����" << uid_loc << " (" << get_unit_full_name(uid_loc) << ")";
            }
            stype.append(convert.str());
            break;
        case EffectType::DeclareVictory:
			convert << "���" << s_player;
			if (panel == 1 && (scen.game == UP || scen.game == ETP) ) {
				convert << " Ͷ��";
			}
			else {
				convert << " ����ʤ��";
			}
            stype.append(convert.str());
            break;
        case EffectType::DisplayInstructions:
			if ( panel >=9 && disp_time >= 99999 && strstr(text.c_str(),"up-effect ") != NULL && strrchr(text.c_str(), ',') != NULL && *(strrchr(text.c_str(), ',') + 1) != '\0' ) {
				convert << "up-effect " << " |" << strrchr(text.c_str(), ',') + 2 << "| " << UpTranslate(text.c_str(),FALSE,FALSE);
			}
			else {
				convert << "��ʾ��Ϣ��" << text.c_str() << "��";
			}
            stype.append(convert.str());
            break;
        case EffectType::ChangeObjectName:
			if ( panel >=1 && strrchr(text.c_str(), ',') != NULL && strstr(text.c_str(),"up-attribute ") != NULL && *(strrchr(text.c_str(), ',') + 1) != '\0' ) {
				convert << "up-attribute �� " << selectedUnits() << " |" << strrchr(text.c_str(), ',') + 2 << "| " << UpTranslate(text.c_str(),FALSE,TRUE);
			}
			else {
				stype.append("����Ϊ��");
				stype.append(text.c_str());
				stype.append("��");
				convert << " " << selectedUnits();
			}
            stype.append(convert.str());
            break;
        case EffectType::DamageObject:
            {
                if (amount == TS_LONG_MAX) {
                    convert << selectedUnits() << "����ֵ��С��";
                } else if (amount == TS_LONG_MIN) {
                    convert << "��" << selectedUnits() << "��Ϊ�޵�";
                } else if (isFloorAmount()) {
                    convert << selectedUnits() << "Ӣ��ʽ��Ѫ" << (amount - TS_FLOAT_MIN) << "�㣨��һ��/��������";
                } else if (isCeilAmount()) {
                    convert << selectedUnits() << "Ӣ��ʽ��Ѫ" << (TS_FLOAT_MAX - amount) << "�㣨�ڶ���/��������";
                } else {
                    if (amount < 0) {
                        convert << "׷��" << -amount << "�㵱ǰ����ֵ�� " << selectedUnits();
                    } else {
                        convert << "��" << amount << "�㵱ǰ����ֵ��" << selectedUnits();
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
					convert << "��Ϊ ";
					break;
				}
			case 2: 
				if (type == EffectType::ChangeObjectHP && (scen.game == UP || scen.game == ETP) ) {
					convert << "���� ";
					break;
				}
			default:
				if (amount > 0) {
				    convert << "+";
				}
				break;
			}
			convert << amount << " " << getTypeName(type, true);
			convert << " ��"  << selectedUnits();
            stype.append(convert.str());
            break;
        case EffectType::ChangeSpeed_UP: // SnapView_SWGB, AttackMove_HD
            switch (scen.game) {
			case ETP:
            case UP:
                if (amount > 0) {
                    convert << "+";
                }
                convert << amount << " " << getTypeName(type, true) << " ��"  << selectedUnits();
                stype.append(convert.str());
                break;
            case SWGB:
            case SWGBCC:
                if (location.x >= 0 && location.y >= 0 && s_player >= 1) {
                    convert << "snap view for p" << s_player << " to " << location.x << "," << location.y << ") ";
                } else {
                    convert << " <��Ч> ";
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
                    convert << "ֹͣ" << selectedUnits();
                    stype.append(convert.str());
                    break;
                }

                convert << selectedUnits();
                if (valid_location_coord()) {
					convert << "�����ƶ��� (" << location.x << "," << location.y << ") ";
                } else {
                    convert << "��" << uid_loc << " (" << get_unit_full_name(uid_loc) << ")";
					convert << "�����ƶ�";
                }
                stype.append(convert.str());
                break;
            default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<δ֪>!");
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
                convert << amount << " " << getTypeName(type, true) << " ��"  << selectedUnits();
                stype.append(convert.str());
                break;
            case SWGB:
            case SWGBCC:
                {
                    switch (type) {
                    case EffectType::DisableAdvancedButtons_SWGB:
                        stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<δ֪>!");
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
                                    convert << "���� " << playerPronoun(s_player) << " �з��Ƽ� " << techname;
                                    break;
                                case EffectType::DisableTech_SWGB:
                                    convert << "��ֹ " << playerPronoun(s_player) << " �з��Ƽ� " << techname;
                                    break;
                                }
                            } else {
                                convert << " <��Ч�Ƽ�> ";
                            }
                        }
                        break;
                    }
                }
                stype.append(convert.str());
                break;
            default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<δ֪>!");
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
                convert << selectedUnits() << "�ָ�" << amount << "������";
                stype.append(convert.str());
	            break;
	        default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<δ֪>!");
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
                convert << "��" << selectedUnits() << "Ϊ";
                switch (stance) {
                case -1:
                    convert << "��״̬";
                    break;
                case 0:
                    convert << "����״̬";
                    break;
                case 1:
                    convert << "����״̬";
                    break;
                case 2:
                    convert << "����״̬";
                    break;
                case 3:
                    convert << "������״̬";
                    break;
                }
                stype.append(convert.str());
                break;
	        default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<δ֪>!");
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
                convert << "��" << selectedUnits() << "֮һ���͵� (" << location.x << "," << location.y << ") ";
                stype.append(convert.str());
	            break;
	        default:
                stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<δ֪>!");
                break;
	        }
            break;
	    case EffectType::InputOff_CC:
	    case EffectType::InputOn_CC:
            stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<δ֪>!");
            break;
        default:
            stype.append((type < scen.pergame->max_effect_types) ? getTypeName(type, true) : "<δ֪>!");
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
	char s_up[] = "up-effect 0"; //Դ���
	char i_up[] = "up-effect 0"; //Ŀ�����
	std::string text_up; // ��ʾ��Ϣ����
	int up_effect_loc; //����up-effectЧ��
	int change_loc; //������ұ���
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
	char s_up[] = "up-effect 0"; //Դ���
	char i_up[] = "up-effect 0"; //Ŀ�����
	char pchar[2];
	pchar[0] = '0' + player;
	pchar[1] = '\0';
	std::string text_up; // ��ʾ��Ϣ����
	int up_effect_loc; //����up-effectЧ��
	bool is_up_effect; //�ǵ�����ҵ�up-effectЧ��
	text_up = (text).c_str();

	//���up-effectЧ���ж�
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
	"�ı��⽻̬��",
	"�о��Ƽ�",
	"�ͳ���̸ѶϢ",
	"������Ч�ļ�",
	"��������",
	"��������",
	"���ճ���",
	"�����",
	"�رմ���",
	"����AI�ź�",
	"�������",
	"ָ�����",
	"����ʤ��",
	"�ݻ����",
	"�Ƴ����",
	"�ı��ӽ�",
	"ж�ص�λ",
	"�ı�����Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"���ᵥλ",
	"�����߼���ť",
};

const char *Effect::types_aoc[] = {
	"",
	"�ı��⽻̬��",
	"�о��Ƽ�",
	"�ͳ���̸ѶϢ",
	"������Ч�ļ�",
	"��������",
	"��������",
	"���ճ���",
	"�����",
	"�رմ���",
	"����AI�ź�",
	"�������",
	"ָ�����",
	"����ʤ��",
	"�ݻ����",
	"�Ƴ����",
	"�ı��ӽ�",
	"ж�ص�λ",
	"�ı�����Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"���ᵥλ",
	"�����߼���ť",
	"�����",
	"���õػ�",
	"�ı��������",
	"�ı��������ֵ����",
	"�ı����������",
	"ֹͣ��λ",
};

const char *Effect::types_up[] = {
	"",
	"�ı��⽻̬��",
	"�о��Ƽ�",
	"�ͳ���̸ѶϢ",
	"������Ч�ļ�",
	"��������",
	"��������",
	"���ճ���",
	"�����",
	"�رմ���",
	"����AI�ź�",
	"�������",
	"ָ�����",
	"����ʤ��",
	"�ݻ����",
	"�Ƴ����",
	"�ı��ӽ�",
	"ж�ص�λ",
	"�ı�����Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"���ᵥλ",
	"�����߼���ť",
	"�����",
	"���õػ�",
	"�ı��������",
	"�ı��������ֵ����",
	"�ı����������",
	"ֹͣ��λ",
	"�ı�������ٶ�",
	"�ı���������",
	"�ı�����Ľ�ս����",
	"�ı������Զ�̷���"
};

const char *Effect::types_etp[] = {
	"",
	"�ı��⽻̬��",
	"�о��Ƽ�",
	"�ͳ���̸ѶϢ",
	"������Ч�ļ�",
	"��������",
	"��������",
	"���ճ���",
	"�����",
	"�رմ���",
	"����AI�ź�",
	"�������",
	"ָ�����",
	"����ʤ��",
	"�ݻ����",
	"�Ƴ����",
	"�ı��ӽ�",
	"ж�ص�λ",
	"�ı�����Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"���ᵥλ",
	"�����߼���ť",
	"�����",
	"���õػ�",
	"�ı��������",
	"�ı��������ֵ����",
	"�ı����������",
	"ֹͣ��λ",
	"�ı�����ٶ�",
	"�ı�������",
	"�ı������ս����",
	"�ı����Զ�̻���",
	"�ı�����������",
	"�رո߼���ť",
	"�ı����ָ�����ͻ���",
	"�ı����ָ�����͹���",
	"�ı������������",
	"�ı���Դ��",
	"�ı������Դ��",
	"�ı������Ұ",
	"�ı��������Ч��",
	"�������Ӣ��״̬",
	"�������ͼ��",
	"ֹͣ����AI�ź�",
	"�ı����ֵ",
	"�������б���",
	"�ı���Դ�����ֵ",
	"�������Ϊ�ļ�",
	"���ļ��ж�ȡ����",
	"����",
	"����",
	"���",
	"��ʾ����������Ϣ",
	"�ͳ��������Ľ�̸ѶϢ",
	"���ô��������������",
	"�������ֵ",
	"�������ȡ���ֵ",
	"����ֵ���浽����",
	"�ӱ����������",
	"�ı����ͼ��",
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
	"�ı��⽻̬��",
	"�о��Ƽ�",
	"�ͳ���̸ѶϢ",
	"������Ч�ļ�",
	"��������",
	"��������",
	"���ճ���",
	"�����",
	"�رմ���",
	"����AI�ź�",
	"�������",
	"ָ�����",
	"����ʤ��",
	"�ݻ����",
	"�Ƴ����",
	"�ı��ӽ�",
	"ж�ص�λ",
	"�ı�����Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"���ᵥλ",
	"�����߼���ť",
	"�����",
	"���õػ�",
	"�ı��������",
	"�ı��������ֵ����",
	"�ı����������",
	"ֹͣ��λ",
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
	"�ı��⽻̬��",
	"�о��Ƽ�",
	"�ͳ���̸ѶϢ",
	"������Ч�ļ�",
	"��������",
	"��������",
	"���ճ���",
	"�����",
	"�رմ���",
	"����AI�ź�",
	"�������",
	"ָ�����",
	"����ʤ��",
	"�ݻ����",
	"�Ƴ����",
	"�ı��ӽ�",
	"ж�ص�λ",
	"�ı�����Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"���ᵥλ",
	"�����߼���ť",
	"�����",
	"���õػ�",
	"�ı��������",
	"�ı��������ֵ����",
	"�ı����������",
	"ֹͣ��λ",
	"�����ƶ�",
	"�ı��������ֵ",
	"�ı���������",
	"�ı�������ٶ�",
	"�������",
	"���͵������",
	"�ı䵥λ��̬"
};

const char *Effect::types_aof[] = {
	"",
	"�ı��⽻̬��",
	"�о��Ƽ�",
	"�ͳ���̸ѶϢ",
	"������Ч�ļ�",
	"��������",
	"��������",
	"���ճ���",
	"�����",
	"�رմ���",
	"����AI�ź�",
	"�������",
	"ָ�����",
	"����ʤ��",
	"�ݻ����",
	"�Ƴ����",
	"�ı��ӽ�",
	"ж�ص�λ",
	"�ı�����Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"���ᵥλ",
	"�����߼���ť",
	"�����",
	"���õػ�",
	"�ı��������",
	"�ı��������ֵ����",
	"�ı����������",
	"ֹͣ��λ",
	"�����ƶ�",
	"�ı��������ֵ",
	"�ı���������",
	"�ı�������ٶ�",
	"�������",
	"���͵������",
	"�ı䵥λ��̬"
};

const char *Effect::types_short_aok[] = {
	"",
	"�ı��⽻",
	"�о�",
	"ѶϢ",
	"��Ч",
	"����",
	"����",
	"����",
	"����",
	"�ر�",
	"AI�ź�",
	"����",
	"ָ��",
	"����ʤ��",
	"�ݻ�",
	"�Ƴ�",
	"�ı��ӽ�",
	"ж��",
	"��Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"����",
	"�߼���ť",
};

const char *Effect::types_short_aoc[] = {
	"",
	"�ı��⽻",
	"�о�",
	"ѶϢ",
	"��Ч",
	"����",
	"����",
	"����",
	"����",
	"�ر�",
	"AI�ź�",
	"����",
	"ָ��",
	"����ʤ��",
	"�ݻ�",
	"�Ƴ�",
	"�ı��ӽ�",
	"ж��",
	"��Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"����",
	"�߼���ť",
	"��",
	"�ػ�",
	"����",
	"����ֵ",
	"������",
	"ֹͣ",
};

const char *Effect::types_short_up[] = {
	"",
	"�ı��⽻",
	"�о�",
	"ѶϢ",
	"��Ч",
	"����",
	"����",
	"����",
	"����",
	"�ر�",
	"AI�ź�",
	"����",
	"ָ��",
	"����ʤ��",
	"�ݻ�",
	"�Ƴ�",
	"�ı��ӽ�",
	"ж��",
	"��Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"����",
	"�߼���ť",
	"��",
	"�ػ�",
	"����",
	"����ֵ",
	"������",
	"ֹͣ",
	"�ٶ�",
	"���",
	"��ս����",
	"Զ�̷���"
};

const char *Effect::types_short_etp[] = {
	"",
	"�ı��⽻",
	"�о�",
	"ѶϢ",
	"��Ч",
	"����",
	"����",
	"����",
	"����",
	"�ر�",
	"AI�ź�",
	"����",
	"ָ��",
	"����ʤ��",
	"�ݻ�",
	"�Ƴ�",
	"�ı��ӽ�",
	"ж��",
	"��Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"����",
	"�߼���ť",
	"��",
	"�ػ�",
	"����",
	"����ֵ",
	"������",
	"ֹͣ",
	"�ٶ�",
	"���",
	"��ս����",
	"Զ�̷���",
	"�������",
	"�رո߼���ť",
	"ָ�����ͻ���",
	"ָ�����͹���",
	"��������",
	"��Դ��",
	"�����Դ��",
	"��Ұ",
	"����Ч��",
	"Ӣ��״̬",
	"ͼ��",
	"ֹͣAI�ź�",
	"����",
	"�������",
	"��Դ�����",
	"�����ļ�",
	"��ȡ�ļ�",
	"����",
	"����",
	"���",
	"������ʾ��Ϣ",
	"����ѶϢ",
	"��������",
	"���",
	"�������",
	"��ֵ������",
	"�����������",
	"���ͼ��"
};

const char *Effect::types_short_aohd[] = {
	"",
	"�ı��⽻",
	"�о�",
	"ѶϢ",
	"��Ч",
	"����",
	"����",
	"����",
	"����",
	"�ر�",
	"AI�ź�",
	"����",
	"ָ��",
	"����ʤ��",
	"�ݻ�",
	"�Ƴ�",
	"�ı��ӽ�",
	"ж��",
	"��Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"����",
	"�߼���ť",
	"��",
	"�ػ�",
	"����",
	"����ֵ",
	"������",
	"ֹͣ",
	"�����ƶ�",
	"����",
	"���",
	"�ٶ�",
	"����",
	"����",
	"�ı���̬"
};

const char *Effect::types_short_aof[] = {
	"",
	"�ı��⽻",
	"�о�",
	"ѶϢ",
	"��Ч",
	"����",
	"����",
	"����",
	"����",
	"�ر�",
	"AI�ź�",
	"����",
	"ָ��",
	"����ʤ��",
	"�ݻ�",
	"�Ƴ�",
	"�ı��ӽ�",
	"ж��",
	"��Ȩ",
	"Ѳ��",
	"��ʾ��Ϣ",
	"�����Ϣ",
	"����",
	"�߼���ť",
	"��",
	"�ػ�",
	"����",
	"����ֵ",
	"������",
	"ֹͣ",
	"�����ƶ�",
	"����",
	"���",
	"�ٶ�",
	"����",
	"����",
	"�ı���̬"
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
    "�������",
    "�������",
    "���ÿƼ�",
    "���ÿƼ�",
    "���ÿƼ�������������",
    "���ÿƼ��Ͱ�ť������������",
	"�������",
    "�趨����ֵ",
    "�������",
    "����״̬",
    "����״̬",
    "����״̬",
    "������״̬",
    "Ͷ��",
    "��˸���",
    "�趨������",
    "������ 1",
    "������ 2",
    "������ 3",
    "������ 4",
    "������ 5",
    "������ 6",
    "������ 7",
    "������ 8",
    "������ 9",
    "�趨�ӽ�",
    "�������",
    "��С����",
    "Ӣ��ʽ��Ѫ�� 1 ��",
    "Ӣ��ʽ��Ѫ�� 2 ��",
	"up-attribute ��չЧ��",
	"up-effect ��չЧ��",
    "�趨 AI �ź�",
    "�趨 AI ����Ŀ��",
    "��������",
};

const char *Effect::virtual_types_etp[] = {
    "",
    "�������",
    "�������",
    "���ÿƼ�",
    "���ÿƼ�",
    "���ÿƼ�������������",
    "���ÿƼ��Ͱ�ť������������",
	"�������",
    "�趨����ֵ",
    "�������",
    "����״̬",
    "����״̬",
    "����״̬",
    "������״̬",
    "Ͷ��",
    "��˸���",
    "�趨������",
    "������ 1",
    "������ 2",
    "������ 3",
    "������ 4",
    "������ 5",
    "������ 6",
    "������ 7",
    "������ 8",
    "������ 9",
    "�趨�ӽ�",
    "�������",
    "��С����",
    "Ӣ��ʽ��Ѫ�� 1 ��",
    "Ӣ��ʽ��Ѫ�� 2 ��",
	"up-attribute ��չЧ��",
	"up-effect ��չЧ��",
    "�趨 AI �ź�",
    "�趨 AI ����Ŀ��",
    "��������",
};

const char *Effect::virtual_types_aoc[] = {
    "",
    "�������",
    "��С����",
    "Ӣ��ʽ��Ѫ�� 1 ��",
    "Ӣ��ʽ��Ѫ�� 2 ��",
    "�趨 AI �ź�",
    "�趨 AI ����Ŀ��",
    "��������",
    "���ᵥλ",
};

const char *Effect::virtual_types_aohd[] = {
    "",
    "�������",
    "��С����",
    "Ӣ��ʽ��Ѫ�� 1 ��",
    "Ӣ��ʽ��Ѫ�� 2 ��",
    "���ᵥλ",
};

const char *Effect::virtual_types_aof[] = {
    "",
    "�������",
    "��С����",
    "Ӣ��ʽ��Ѫ�� 1 ��",
    "Ӣ��ʽ��Ѫ�� 2 ��",
    "���ᵥλ",
};

const char *Effect::virtual_types_swgb[] = {
    "",
    "�������",
    "��С����",
    "Ӣ��ʽ��Ѫ�� 1 ��",
    "Ӣ��ʽ��Ѫ�� 2 ��",
    "���ᵥλ",
};

const char *Effect::virtual_types_cc[] = {
    "",
    "�������",
    "��С����",
    "Ӣ��ʽ��Ѫ�� 1 ��",
    "Ӣ��ʽ��Ѫ�� 2 ��",
    "���ᵥλ",
};

const char *Effect::virtual_types_aok[] = {
    "",
    "�������",
    "��С����",
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
