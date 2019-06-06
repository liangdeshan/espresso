/*
  Copyright (C) 2010-2018 The ESPResSo project
  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010
    Max-Planck-Institute for Polymer Research, Theory Group

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
/** \file
 * All 3d non P3M methods to deal with the
 * magnetic dipoles
 *
 *  DAWAANR => DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA
 *   Handling of a system of dipoles where no replicas
 *   Assumes minimum image convention for those axis in which the
 *   system is periodic as defined by setmd.
 *
 *   MDDS => Calculates dipole-dipole interaction of a periodic system
 *   by explicitly summing the dipole-dipole interaction over several copies of
 * the system
 *   Uses spherical summation order
 *
 */

#include "electrostatics_magnetostatics/magnetic_non_p3m_methods.hpp"

#ifdef DIPOLES
#include "cells.hpp"
#include "dipole.hpp"
#include "electrostatics_magnetostatics/dipole.hpp"
#include "grid.hpp"
#include "nonbonded_interactions/nonbonded_interaction_data.hpp"
#include "communication.hpp"

#include <utils/constants.hpp>

// Calculates dipolar energy and/or force between two particles
double calc_dipole_dipole_ia(Particle *p1, const Utils::Vector3d &dip1,
                             Particle *p2, int force_flag) {
  double u, r, pe1, pe2, pe3, pe4, r3, r5, r2, r7, a, b, cc, d, ab;
#ifdef ROTATION
  double bx, by, bz, ax, ay, az;
#endif
  double ffx, ffy, ffz;

  // Cache dipole momente
  const Utils::Vector3d dip2 = p2->calc_dip();

  // Distance between particles
  auto const dr = get_mi_vector(p1->r.p, p2->r.p, box_geo);

  // Powers of distance
  r2 = dr * dr;
  r = sqrt(r2);
  r3 = r2 * r;
  r5 = r3 * r2;
  r7 = r5 * r2;

  // Dot products
  pe1 = dip1 * dip2;
  pe2 = dip1 * dr;
  pe3 = dip2 * dr;
  pe4 = 3.0 / r5;

  // Energy, if requested
  u = dipole.prefactor * (pe1 / r3 - pe4 * pe2 * pe3);

  // Force, if requested
  if (force_flag) {
    a = pe4 * pe1;
    b = -15.0 * pe2 * pe3 / r7;
    ab = a + b;
    cc = pe4 * pe3;
    d = pe4 * pe2;

    //  Result
    ffx = ab * dr[0] + cc * dip1[0] + d * dip2[0];
    ffy = ab * dr[1] + cc * dip1[1] + d * dip2[1];
    ffz = ab * dr[2] + cc * dip1[2] + d * dip2[2];
    // Add the force to the particles
    p1->f.f[0] += dipole.prefactor * ffx;
    p1->f.f[1] += dipole.prefactor * ffy;
    p1->f.f[2] += dipole.prefactor * ffz;
    p2->f.f[0] -= dipole.prefactor * ffx;
    p2->f.f[1] -= dipole.prefactor * ffy;
    p2->f.f[2] -= dipole.prefactor * ffz;

// Torques
#ifdef ROTATION
    ax = dip1[1] * dip2[2] - dip2[1] * dip1[2];
    ay = dip2[0] * dip1[2] - dip1[0] * dip2[2];
    az = dip1[0] * dip2[1] - dip2[0] * dip1[1];

    bx = dip1[1] * dr[2] - dr[1] * dip1[2];
    by = dr[0] * dip1[2] - dip1[0] * dr[2];
    bz = dip1[0] * dr[1] - dr[0] * dip1[1];

    p1->f.torque[0] += dipole.prefactor * (-ax / r3 + bx * cc);
    p1->f.torque[1] += dipole.prefactor * (-ay / r3 + by * cc);
    p1->f.torque[2] += dipole.prefactor * (-az / r3 + bz * cc);

    // 2nd particle
    bx = dip2[1] * dr[2] - dr[1] * dip2[2];
    by = dr[0] * dip2[2] - dip2[0] * dr[2];
    bz = dip2[0] * dr[1] - dr[0] * dip2[1];

    p2->f.torque[0] += dipole.prefactor * (ax / r3 + bx * d);
    p2->f.torque[1] += dipole.prefactor * (ay / r3 + by * d);
    p2->f.torque[2] += dipole.prefactor * (az / r3 + bz * d);
#endif
  }

  // Return energy
  return u;
}

/* =============================================================================
                  DAWAANR => DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA
   =============================================================================
*/

double dawaanr_calculations(int force_flag, int energy_flag) {
  double u;

  if (n_nodes != 1) {
    fprintf(stderr, "error:  DAWAANR is just for one cpu .... \n");
    errexit();
  }
  if (!(force_flag) && !(energy_flag)) {
    fprintf(stderr, " I don't know why you call dawaanr_caclulations with all "
                    "flags zero \n");
    return 0;
  }

  // Variable to sum up the energy
  u = 0;

  auto parts = local_cells.particles();

  // Iterate over all cells
  for (auto it = parts.begin(), end = parts.end(); it != end; ++it) {
    // If the particle has no dipole moment, ignore it
    if (it->p.dipm == 0.0)
      continue;

    const Utils::Vector3d dip1 = it->calc_dip();
    auto jt = it;
    /* Skip diagonal */
    ++jt;
    for (; jt != end; ++jt) {
      // If the particle has no dipole moment, ignore it
      if (jt->p.dipm == 0.0)
        continue;
      // Calculate energy and/or force between the particles
      u += calc_dipole_dipole_ia(&(*it), dip1, &(*jt), force_flag);
    }
  }

  // Return energy
  return u;
}

/************************************************************/

/*=================== */
/*=================== */
/*=================== */
/*=================== */
/*=================== */
/*=================== */

/* =============================================================================
                  DIRECT SUM FOR MAGNETIC SYSTEMS
   =============================================================================
*/

int Ncut_off_magnetic_dipolar_direct_sum = 0;

/************************************************************/

int magnetic_dipolar_direct_sum_sanity_checks() {
  /* left for the future , at this moment nothing to do */

  return 0;
}

/************************************************************/

double magnetic_dipolar_direct_sum_calculations(int force_flag,
                                                int energy_flag) {
  std::vector<double> x, y, z;
  std::vector<double> mx, my, mz;
  std::vector<double> fx, fy, fz;
#ifdef ROTATION
  std::vector<double> tx, ty, tz;
#endif
  int dip_particles, dip_particles2;
  double u;

  if (n_nodes != 1) {
    fprintf(stderr, "error: magnetic Direct Sum is just for one cpu .... \n");
    errexit();
  }
  if (!(force_flag) && !(energy_flag)) {
    fprintf(stderr, " I don't know why you call dawaanr_caclulations with all "
                    "flags zero \n");
    return 0;
  }

  x.resize(n_part);
  y.resize(n_part);
  z.resize(n_part);

  mx.resize(n_part);
  my.resize(n_part);
  mz.resize(n_part);

  if (force_flag) {
    fx.resize(n_part);
    fy.resize(n_part);
    fz.resize(n_part);

#ifdef ROTATION
    tx.resize(n_part);
    ty.resize(n_part);
    tz.resize(n_part);
#endif
  }

  dip_particles = 0;
  for (auto const &p : local_cells.particles()) {
    if (p.p.dipm != 0.0) {
      const Utils::Vector3d dip = p.calc_dip();

      mx[dip_particles] = dip[0];
      my[dip_particles] = dip[1];
      mz[dip_particles] = dip[2];

      /* here we wish the coordinates to be folded into the primary box */
      auto const ppos = folded_position(p.r.p);
      x[dip_particles] = ppos[0];
      y[dip_particles] = ppos[1];
      z[dip_particles] = ppos[2];

      if (force_flag) {
        fx[dip_particles] = 0;
        fy[dip_particles] = 0;
        fz[dip_particles] = 0;

#ifdef ROTATION
        tx[dip_particles] = 0;
        ty[dip_particles] = 0;
        tz[dip_particles] = 0;
#endif
      }

      dip_particles++;
    }
  }

  /*now we do the calculations */

  { /* beginning of the area of calculation */
    int nx, ny, nz, i, j;
    double r, rnx, rny, rnz, pe1, pe2, pe3, r3, r5, r2, r7;
    double a, b, c, d;
#ifdef ROTATION
    double ax, ay, az, bx, by, bz;
#endif
    double rx, ry, rz;
    double rnx2, rny2;
    int NCUT[3], NCUT2;

    for (i = 0; i < 3; i++) {
      NCUT[i] = Ncut_off_magnetic_dipolar_direct_sum;
      if (box_geo.periodic(i) == 0) {
        NCUT[i] = 0;
      }
    }
    NCUT2 = Ncut_off_magnetic_dipolar_direct_sum *
            Ncut_off_magnetic_dipolar_direct_sum;

    u = 0;

    for (i = 0; i < dip_particles; i++) {
      for (j = 0; j < dip_particles; j++) {
        pe1 = mx[i] * mx[j] + my[i] * my[j] + mz[i] * mz[j];
        rx = x[i] - x[j];
        ry = y[i] - y[j];
        rz = z[i] - z[j];

        for (nx = -NCUT[0]; nx <= NCUT[0]; nx++) {
          rnx = rx + nx * box_geo.length()[0];
          rnx2 = rnx * rnx;
          for (ny = -NCUT[1]; ny <= NCUT[1]; ny++) {
            rny = ry + ny * box_geo.length()[1];
            rny2 = rny * rny;
            for (nz = -NCUT[2]; nz <= NCUT[2]; nz++) {
              if (!(i == j && nx == 0 && ny == 0 && nz == 0)) {
                if (nx * nx + ny * ny + nz * nz <= NCUT2) {
                  rnz = rz + nz * box_geo.length()[2];
                  r2 = rnx2 + rny2 + rnz * rnz;
                  r = sqrt(r2);
                  r3 = r2 * r;
                  r5 = r3 * r2;
                  r7 = r5 * r2;

                  pe2 = mx[i] * rnx + my[i] * rny + mz[i] * rnz;
                  pe3 = mx[j] * rnx + my[j] * rny + mz[j] * rnz;

                  // Energy ............................

                  u += pe1 / r3 - 3.0 * pe2 * pe3 / r5;

                  if (force_flag) {
                    // force ............................
                    a = mx[i] * mx[j] + my[i] * my[j] + mz[i] * mz[j];
                    a = 3.0 * a / r5;
                    b = -15.0 * pe2 * pe3 / r7;
                    c = 3.0 * pe3 / r5;
                    d = 3.0 * pe2 / r5;

                    fx[i] += (a + b) * rnx + c * mx[i] + d * mx[j];
                    fy[i] += (a + b) * rny + c * my[i] + d * my[j];
                    fz[i] += (a + b) * rnz + c * mz[i] + d * mz[j];

#ifdef ROTATION
                    // torque ............................
                    c = 3.0 / r5 * pe3;
                    ax = my[i] * mz[j] - my[j] * mz[i];
                    ay = mx[j] * mz[i] - mx[i] * mz[j];
                    az = mx[i] * my[j] - mx[j] * my[i];

                    bx = my[i] * rnz - rny * mz[i];
                    by = rnx * mz[i] - mx[i] * rnz;
                    bz = mx[i] * rny - rnx * my[i];

                    tx[i] += -ax / r3 + bx * c;
                    ty[i] += -ay / r3 + by * c;
                    tz[i] += -az / r3 + bz * c;
#endif
                  } /* of force_flag  */
                }
              } /* of nx*nx+ny*ny +nz*nz< NCUT*NCUT   and   !(i==j && nx==0 &&
                   ny==0 && nz==0) */
            }   /* of  for nz */
          }     /* of  for ny  */
        }       /* of  for nx  */
      }
    } /* of  j and i  */
  }   /* end of the area of calculation */

  /* set the forces, and torques of the particles within Espresso */
  if (force_flag) {

    dip_particles2 = 0;

    for (auto &p : local_cells.particles()) {
      if (p.p.dipm != 0.0) {

        p.f.f[0] += dipole.prefactor * fx[dip_particles2];
        p.f.f[1] += dipole.prefactor * fy[dip_particles2];
        p.f.f[2] += dipole.prefactor * fz[dip_particles2];

#ifdef ROTATION
        p.f.torque[0] += dipole.prefactor * tx[dip_particles2];
        p.f.torque[1] += dipole.prefactor * ty[dip_particles2];
        p.f.torque[2] += dipole.prefactor * tz[dip_particles2];
#endif
        dip_particles2++;
      }
    }
  } /*of if force_flag */

  return 0.5 * dipole.prefactor * u;
}

int dawaanr_set_params() {
  if (n_nodes > 1) {
    return ES_ERROR;
  }
  if (dipole.method != DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA) {
    Dipole::set_method_local(DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA);
  }
  // also necessary on 1 CPU, does more than just broadcasting
  mpi_bcast_coulomb_params();

  return ES_OK;
}

int mdds_set_params(int n_cut) {
  if (n_nodes > 1) {
    return ES_ERROR;
  }

  Ncut_off_magnetic_dipolar_direct_sum = n_cut;

  if (Ncut_off_magnetic_dipolar_direct_sum == 0) {
    fprintf(stderr, "Careful:  the number of extra replicas to take into "
                    "account during the direct sum calculation is zero \n");
  }

  if (dipole.method != DIPOLAR_DS && dipole.method != DIPOLAR_MDLC_DS) {
    Dipole::set_method_local(DIPOLAR_DS);
  }

  // also necessary on 1 CPU, does more than just broadcasting
  mpi_bcast_coulomb_params();
  return ES_OK;
}

#endif
