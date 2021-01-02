#include "ECBase.h"

char const * players_ec[] =
{
	"<无>", "盖亚", "玩家 1", "玩家 2", "玩家 3", "玩家 4", "玩家 5", "玩家 6", "玩家 7", "玩家 8"
};

ECBase::ECBase(enum ECType c, long t, long s)
:	type(t), ectype(c), size(s)
{}

AOKRECT::AOKRECT(long t, long r, long b, long l)
: top(t), right(r), bottom(b), left(l)
{
}
