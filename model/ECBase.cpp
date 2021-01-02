#include "ECBase.h"

char const * players_ec[] =
{
	"<��>", "����", "��� 1", "��� 2", "��� 3", "��� 4", "��� 5", "��� 6", "��� 7", "��� 8"
};

ECBase::ECBase(enum ECType c, long t, long s)
:	type(t), ectype(c), size(s)
{}

AOKRECT::AOKRECT(long t, long r, long b, long l)
: top(t), right(r), bottom(b), left(l)
{
}
