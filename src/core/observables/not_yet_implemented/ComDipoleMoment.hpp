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
#ifdef DIPOLES
int ObservableComDipoleMoment::actual_calculate(PartCfg &partCfg) {
  double *A = last_value;
  double d[3] = {0., 0., 0.};
  IntList *ids;
  if (!sortPartCfg()) {
    runtimeErrorMsg() << "could not sort partCfg";
    return -1;
  }
  ids = (IntList *)container;
  for (int i = 0; i < ids->n; i++) {
    if (ids->e[i] > n_part)
      return 1;
    d[0] += partCfg[ids->e[i]].r.dip[0];
    d[1] += partCfg[ids->e[i]].r.dip[1];
    d[2] += partCfg[ids->e[i]].r.dip[2];
  }
  A[0] = d[0];
  A[1] = d[1];
  A[2] = d[2];
  return 0;
}
#endif
