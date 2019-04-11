//Small helper class with a bunch of public vars and a function that lerps based on them

struct Smooth
{

	Smooth(bool initialplus, int initialvalue, int inmin, int inmax, int inaddcount) :
		plus(initialplus), value(initialvalue), min(inmin), max(inmax), addcount(inaddcount) {}

	int lerp()
	{
		if (plus && value < max)
		{
			value += addcount;
			if (value > max) value = max;
		}
		if (!plus && value > 0)
		{
			value -= addcount;
			if (value < 0) value = 0;
		}
		return value;
	}

	bool plus;
	int value;
	int min;
	int max;
	int addcount;
};