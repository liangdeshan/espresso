/*
Copyright (C) 2010-2018 The ESPResSo project
 
This file is part of ESPResSo.
 
ESPResSo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
 
ESPResSo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/
#ifndef UTILS_CONSTANTS_HPP
#define UTILS_CONSTANTS_HPP

/*************************************************************/
/** \name Mathematical, physical and chemical constants.     */
/*************************************************************/
/*@{*/
/** Pi. */
#define PI 3.14159265358979323846264338328
/** Square root of Pi */
#define wupi 1.77245385090551602729816748334
/** One over square root of Pi. */
#define wupii 0.56418958354775627928034964498
/** Pi to the power 1/3. */
#define driwu2 1.25992104989487316476721060728

/// error code if no error occured
#define ES_OK 0
/// error code if an error occured
#define ES_ERROR 1

/** space necessary for an (64-bit) integer with sprintf.
    Analog to Tcl
*/
#define ES_INTEGER_SPACE 24
/** space necessary for an double with sprintf. Precision
    is 17 digits, plus sign, dot, e, sign of exponent and
    3 digits exponent etc. Analog to Tcl
*/
#define ES_DOUBLE_SPACE 27

/*@}*/

#endif
