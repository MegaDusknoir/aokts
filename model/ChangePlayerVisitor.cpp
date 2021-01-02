#include "ChangePlayerVisitor.h"

ChangePlayerVisitor::ChangePlayerVisitor(int to)
: _player(to)
{
}

void ChangePlayerVisitor::visit(Trigger&)
{
	// not concerned with triggers
}

void ChangePlayerVisitor::visit(Effect& e)
{
	extern bool changemessage;
	if (e.getPlayer() > ECBase::GAIA_INDEX + '0')
		e.setText(_player, 0);//变换up-effect
	else if ( (e.type == EffectType::DisplayInstructions || e.type == EffectType::ChangeObjectName || e.type == EffectType::SendChat) && changemessage) {
		e.setText(_player, 1);//变换参数文本
		e.setPlayer(_player);
	}
	else if (e.getPlayer() > ECBase::GAIA_INDEX)
		e.setPlayer(_player);
}

void ChangePlayerVisitor::visit(Condition& c)
{
	if (c.getPlayer() > ECBase::GAIA_INDEX)
		c.setPlayer(_player);
}

