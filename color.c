/*********************************************************************
 * kd-forest                                                         *
 * Copyright (C) 2014 Tavian Barnes <tavianator@tavianator.com>      *
 *                                                                   *
 * This program is free software. It comes without any warranty, to  *
 * the extent permitted by applicable law. You can redistribute it   *
 * and/or modify it under the terms of the Do What The Fuck You Want *
 * To Public License, Version 2, as published by Sam Hocevar. See    *
 * the COPYING file or http://www.wtfpl.net/ for more details.       *
 *********************************************************************/

#include "color.h"
#include <math.h>
#define PI 3.1415926535897932

void
color_unpack(uint8_t pixel[3], uint32_t color)
{
  pixel[0] = (color >> 16) & 0xFF;
  pixel[1] = (color >> 8) & 0xFF;
  pixel[2] = color & 0xFF;
}

void
color_set_RGB(double coords[3], uint32_t color)
{
  uint8_t pixel[3];
  color_unpack(pixel, color);
  for (int i = 0; i < 3; ++i) {
    coords[i] = pixel[i]/255.0;
  }
}

// Inverse gamma for sRGB
double
sRGB_C_inv(double t)
{
  if (t <= 0.040449936) {
    return t/12.92;
  } else {
    return pow((t + 0.055)/1.055, 2.4);
  }
}

static void
color_set_XYZ(double XYZ[3], uint32_t color)
{
  double RGB[3];
  color_set_RGB(RGB, color);

  RGB[0] = sRGB_C_inv(RGB[0]);
  RGB[1] = sRGB_C_inv(RGB[1]);
  RGB[2] = sRGB_C_inv(RGB[2]);

  XYZ[0] = 0.4123808838268995*RGB[0] + 0.3575728355732478*RGB[1] + 0.1804522977447919*RGB[2];
  XYZ[1] = 0.2126198631048975*RGB[0] + 0.7151387878413206*RGB[1] + 0.0721499433963131*RGB[2];
  XYZ[2] = 0.0193434956789248*RGB[0] + 0.1192121694056356*RGB[1] + 0.9505065664127130*RGB[2];
}

// CIE L*a*b* and L*u*v* gamma
static double
Lab_f(double t)
{
  if (t > 216.0/24389.0) {
    return pow(t, 1.0/3.0);
  } else {
    return 841.0*t/108.0 + 4.0/29.0;
  }
}

// sRGB white point (CIE D50) in XYZ coordinates
static const double WHITE[] = {
  [0] = 0.9504060171449392,
  [1] = 0.9999085943425312,
  [2] = 1.089062231497274,
};

void
color_set_Lab(double coords[3], uint32_t color)
{
  double XYZ[3];
  color_set_XYZ(XYZ, color);

  double fXYZ[] = {
    [0] = Lab_f(XYZ[0]/WHITE[0]),
    [1] = Lab_f(XYZ[1]/WHITE[1]),
    [2] = Lab_f(XYZ[2]/WHITE[2]),
  };

  coords[0] = 116.0*fXYZ[1] - 16.0;
  coords[1] = 500.0*(fXYZ[0] - fXYZ[1]);
  coords[2] = 200.0*(fXYZ[1] - fXYZ[2]);
}

void
color_set_Luv(double coords[3], uint32_t color)
{
  double XYZ[3];
  color_set_XYZ(XYZ, color);

  double uv_denom = XYZ[0] + 15.0*XYZ[1] + 3.0*XYZ[2];
  if (uv_denom == 0.0) {
    coords[0] = 0.0;
    coords[1] = 0.0;
    coords[2] = 0.0;
    return;
  }

  double white_uv_denom = WHITE[0] + 16.0*WHITE[1] + 3.0*WHITE[2];

  double fY = Lab_f(XYZ[1]/WHITE[1]);
  double uprime = 4.0*XYZ[0]/uv_denom;
  double unprime = 4.0*WHITE[0]/white_uv_denom;
  double vprime = 9.0*XYZ[1]/uv_denom;
  double vnprime = 9.0*WHITE[1]/white_uv_denom;

  coords[0] = 116.0*fY - 16.0;
  coords[1] = 13.0*coords[0]*(uprime - unprime);
  coords[2] = 13.0*coords[0]*(vprime - vnprime);
}

static double
hue(uint32_t color)
{
  double RGB[3];
  color_set_RGB(RGB, color);

  double hue = atan2(sqrt(3.0)*(RGB[1] - RGB[2]), 2*RGB[0] - RGB[1] - RGB[2]);
  if (hue < 0.0) {
    hue += 2.0*PI;
  }
  return hue;
}

int
color_comparator(const void *a, const void *b)
{
  double ahue = hue(*(uint32_t *)a);
  double bhue = hue(*(uint32_t *)b);
  return (ahue > bhue) - (ahue < bhue);
}
