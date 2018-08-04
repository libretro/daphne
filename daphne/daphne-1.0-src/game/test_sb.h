#ifndef TEST_SB_H
#define TEST_SB_H

#include "game.h"
#include "../scoreboard/scoreboard_factory.h"

class test_sb : public game
{
public:
	test_sb();
	void start();
   unsigned get_libretro_button_map(unsigned id);
   const char *get_libretro_button_name(unsigned id);
};

#endif // TEST_SB_H
