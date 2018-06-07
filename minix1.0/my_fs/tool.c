#include"tool.h"


#define clear_screen(void) printf("\033[2J")
#define move_cursor_to(x, y) printf("\033[%d;%dH", y, x)

static void set_console_textAttribute(
                 enum CF_color F_id, 
                 enum CB_color B_id) 
{      
	char f_color[8], b_color[8];
        f_color[0] = F_id / 10 + '0';
        f_color[1] = F_id % 10 + '0';
        f_color[2] = '\0';
        b_color[0] = B_id / 10 + '0';
        b_color[1] = B_id % 10 + '0';
        b_color[2] = '\0';
        printf("\033[%sm", b_color);
        printf("\033[%sm", f_color);
        return;
        //printf("\033[33m");
        //printf("\033[42m");

}
// Ž°¿Ú³õÊŒ»¯
void window_init(int cols, int rows)
{
	clear_screen(0);
        return;
}
// ÉèÖÃ¹â±êÎ»ÖÃ
void set_cursor_position(int x, int y)
{
      	struct COORD position;
	position.X = x;
	position.Y = y;
	move_cursor_to(position.X, position.Y);
}
//
void set_cursor_position_d()
{
	set_cursor_position(WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1);
}
// ÉèÖÃÎÄ±ŸÑÕÉ«ºÍÎÄ±Ÿ±³Ÿ°É«
void set_console_color(enum CF_color F_colorID, enum CB_color B_colorID)
{
	set_console_textAttribute(F_colorID, B_colorID);
}

// ÉèÖÃÎÄ±Ÿ±³Ÿ°É«
void set_background(enum CB_color colorID)
{
	set_console_textAttribute(CFC_White, colorID);
}
// ÉèÖÃÎÄ±ŸÇ°Ÿ°É«
void set_foreground(enum CF_color colorID)
{
	set_console_textAttribute(colorID, CBC_Black);
}
// »ÖžŽÎÄ±ŸÑÕÉ«ºÍÎÄ±Ÿ±³Ÿ°É«ÎªÄ¬ÈÏÖµ-- °× ºÚ
void set_console_color_d()
{
	enum CF_color F_colorID = CFC_White;
	enum CB_color B_colorID = CBC_Black;
	set_console_textAttribute(F_colorID, B_colorID);
}
