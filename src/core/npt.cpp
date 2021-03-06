/*
 * Copyright (C) 2010-2019 The ESPResSo project
 *
 * This file is part of ESPResSo.
 *
 * ESPResSo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ESPResSo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "npt.hpp"
#include "communication.hpp"
#include "config.hpp"
#include "errorhandling.hpp"
#include "integrate.hpp"

#include <utils/Vector.hpp>

#include <boost/mpi/collectives/broadcast.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#ifdef NPT
void synchronize_npt_state() {
  boost::mpi::broadcast(comm_cart, nptiso.p_inst, 0);
  boost::mpi::broadcast(comm_cart, nptiso.p_diff, 0);
  boost::mpi::broadcast(comm_cart, nptiso.volume, 0);
}

void mpi_bcast_nptiso_geom_worker(int, int) {
  boost::mpi::broadcast(comm_cart, nptiso.geometry, 0);
  boost::mpi::broadcast(comm_cart, nptiso.dimension, 0);
  boost::mpi::broadcast(comm_cart, nptiso.cubic_box, 0);
  boost::mpi::broadcast(comm_cart, nptiso.non_const_dim, 0);
}

REGISTER_CALLBACK(mpi_bcast_nptiso_geom_worker)

void mpi_bcast_nptiso_geom() {
  mpi_call_all(mpi_bcast_nptiso_geom_worker, -1, 0);
}

void npt_ensemble_init(const BoxGeometry &box) {
  if (integ_switch == INTEG_METHOD_NPT_ISO) {
    /* prepare NpT-integration */
    nptiso.inv_piston = 1 / (1.0 * nptiso.piston);
    if (nptiso.dimension == 0) {
      throw std::runtime_error(
          "%d: INTERNAL ERROR: npt integrator was called but "
          "dimension not yet set. this should not happen. ");
    }

    nptiso.volume = pow(box.length()[nptiso.non_const_dim], nptiso.dimension);

    if (recalc_forces) {
      nptiso.p_inst = 0.0;
      nptiso.p_vir[0] = nptiso.p_vir[1] = nptiso.p_vir[2] = 0.0;
      nptiso.p_vel[0] = nptiso.p_vel[1] = nptiso.p_vel[2] = 0.0;
    }
  }
}

void integrator_npt_sanity_checks() {
  if (integ_switch == INTEG_METHOD_NPT_ISO) {
    if (nptiso.piston <= 0.0) {
      runtimeErrorMsg() << "npt on, but piston mass not set";
    }
  }
}

/** reset virial part of instantaneous pressure */
void npt_reset_instantaneous_virials() {
  if (integ_switch == INTEG_METHOD_NPT_ISO)
    nptiso.p_vir[0] = nptiso.p_vir[1] = nptiso.p_vir[2] = 0.0;
}

void npt_add_virial_contribution(const Utils::Vector3d &force,
                                 const Utils::Vector3d &d) {
  if (integ_switch == INTEG_METHOD_NPT_ISO) {
    for (int j = 0; j < 3; j++) {
      nptiso.p_vir[j] += force[j] * d[j];
    }
  }
}
#endif // NPT
