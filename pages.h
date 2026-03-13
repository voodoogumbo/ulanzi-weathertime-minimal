/*
  pages.h - Page rendering (clock, weather, notifications)
*/

#ifndef PAGES_H
#define PAGES_H

// Draw main clock page (AWTRIX TMODE5 style)
void drawClock();

// Draw weather page with temperature and icon
void drawWeather();

// Draw notification overlay with scrolling support
void drawNotify();

#endif // PAGES_H
