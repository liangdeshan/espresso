
#include "electrostatics_magnetostatics/dipole.hpp"

#include "electrostatics_magnetostatics/magnetic_non_p3m_methods.hpp"
#include "electrostatics_magnetostatics/mdlc_correction.hpp"
#include "electrostatics_magnetostatics/p3m-dipolar.hpp"
#include "electrostatics_magnetostatics/scafacos.hpp"
#include "integrate.hpp"
#include "npt.hpp"

#ifdef ELECTROSTATICS
#ifdef DIPOLES

Dipole_parameters dipole = {
    0.0,
    DIPOLAR_NONE,
};

namespace Dipole {

void pressure_n(int &n_dipolar) {
  switch (dipole.method) {
  case DIPOLAR_NONE:
    n_dipolar = 0;
    break;
  case DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA:
    n_dipolar = 0;
    break;
  case DIPOLAR_DS:
    n_dipolar = 0;
    break;
  case DIPOLAR_P3M:
    n_dipolar = 2;
    break;
  default:
    n_dipolar = 0;
    break;
  }
}

void calc_pressure_long_range(Observable_stat &virials,
                              Observable_stat &p_tensor) {
  switch (dipole.method) {
  case DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA:
    fprintf(
        stderr,
        "WARNING: pressure calculated, but DAWAANR pressure not implemented\n");
    break;
  case DIPOLAR_MDLC_DS:
    fprintf(stderr,
            "WARNING: pressure calculated, but DLC pressure not implemented\n");
    break;
  case DIPOLAR_DS:
    fprintf(stderr, "WARNING: pressure calculated, but  MAGNETIC DIRECT SUM "
                    "pressure not implemented\n");
    break;

#ifdef DP3M
  case DIPOLAR_MDLC_P3M:
    fprintf(stderr,
            "WARNING: pressure calculated, but DLC pressure not implemented\n");
    break;
  case DIPOLAR_P3M: {
    int k;
    dp3m_dipole_assign();
    virials.dipolar[1] = dp3m_calc_kspace_forces(0, 1);

    for (k = 0; k < 3; k++)
      p_tensor.coulomb[9 + k * 3 + k] = virials.dipolar[1] / 3.;
    fprintf(stderr, "WARNING: stress tensor calculated, but dipolar P3M stress "
                    "tensor not implemented\n");
    fprintf(stderr, "WARNING: things have been added to the coulomb virial and "
                    "p_tensor arrays !!!!!!!\n");

    break;
  }
#endif
  default:
    break;
  }
}

void nonbonded_sanity_check(int &state) {
#ifdef DP3M
  switch (dipole.method) {
  case DIPOLAR_MDLC_P3M:
    if (mdlc_sanity_checks())
      state = 0; // fall through
  case DIPOLAR_P3M:
    if (dp3m_sanity_checks())
      state = 0;
    break;
  case DIPOLAR_MDLC_DS:
    if (mdlc_sanity_checks())
      state = 0; // fall through
  case DIPOLAR_DS:
    if (magnetic_dipolar_direct_sum_sanity_checks())
      state = 0;
    break;
  default:
    break;
  }
#endif
}

void cutoff(double &ret) {
  switch (dipole.method) {
#ifdef DP3M
  case DIPOLAR_MDLC_P3M:
    // fall through
  case DIPOLAR_P3M: {
    /* do not use precalculated r_cut here, might not be set yet */
    ret = dp3m.params.r_cut_iL * box_l[0];
  }
#endif /*ifdef DP3M */
    // Note: Dipolar calculation via scafacos
    // There doesn't seem to be short range delegation for dipolar methods
    // in Scafacos, so no cutoff is contributed
  default:
    break;
  }
}

void integrate_sanity_check() {
  switch (dipole.method) {
  case DIPOLAR_NONE:
    break;
#ifdef DP3M
  case DIPOLAR_P3M:
    break;
#endif /* DP3M */
  default: {
    runtimeErrorMsg()
        << "NpT does not work with your dipolar method, please use P3M.";
  }
  }
}

void on_observable_calc() {
  switch (dipole.method) {
#ifdef DP3M
  case DIPOLAR_MDLC_P3M:
    // fall through
  case DIPOLAR_P3M:
    dp3m_count_magnetic_particles();
    break;
#endif
  default:
    break;
  }
}

void on_coulomb_change() {
  switch (dipole.method) {
#ifdef DP3M
  case DIPOLAR_MDLC_P3M:
    // fall through
  case DIPOLAR_P3M:
    dp3m_init();
    break;
#endif
  default:
    break;
  }
}

void on_boxl_change() {
  switch (dipole.method) {
#ifdef DP3M
  case DIPOLAR_MDLC_P3M:
    // fall through
  case DIPOLAR_P3M:
    dp3m_scaleby_box_l();
    break;
#endif
#ifdef SCAFACOS
  case DIPOLAR_SCAFACOS:
    Scafacos::update_system_params();
    break;
#endif
  default:
    break;
  }
}

void init() {
  switch (dipole.method) {
#ifdef DP3M
  case DIPOLAR_MDLC_P3M:
    // fall through
  case DIPOLAR_P3M:
    dp3m_init();
    break;
#endif
  default:
    break;
  }
}

void calc_long_range_force() {
  switch (dipole.method) {
#ifdef DP3M
  case DIPOLAR_MDLC_P3M:
    add_mdlc_force_corrections();
    // fall through
  case DIPOLAR_P3M:
    dp3m_dipole_assign();
#ifdef NPT
    if (integ_switch == INTEG_METHOD_NPT_ISO) {
      nptiso.p_vir[0] += dp3m_calc_kspace_forces(1, 1);
      fprintf(stderr, "dipolar_P3M at this moment is added to p_vir[0]\n");
    } else
#endif
      dp3m_calc_kspace_forces(1, 0);

    break;
#endif
  case DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA:
    dawaanr_calculations(1, 0);
    break;
#ifdef DP3M
  case DIPOLAR_MDLC_DS:
    add_mdlc_force_corrections();
    // fall through
#endif
  case DIPOLAR_DS:
    magnetic_dipolar_direct_sum_calculations(1, 0);
    break;
  case DIPOLAR_DS_GPU:
    // Do nothing. It's an actor
    break;
#ifdef DIPOLAR_BARNES_HUT
  case DIPOLAR_BH_GPU:
    // Do nothing, it's an actor.
    break;
#endif // BARNES_HUT
#ifdef SCAFACOS_DIPOLES
  case DIPOLAR_SCAFACOS:
    assert(Scafacos::dipolar());
    Scafacos::add_long_range_force();
#endif
  case DIPOLAR_NONE:
    break;
  default:
    runtimeErrorMsg() << "unknown dipolar method";
    break;
  }
}

void calc_energy_long_range(Observable_stat &energy) {
  switch (dipole.method) {
#ifdef DP3M
  case DIPOLAR_P3M:
    dp3m_dipole_assign();
    energy.dipolar[1] = dp3m_calc_kspace_forces(0, 1);
    break;
  case DIPOLAR_MDLC_P3M:
    dp3m_dipole_assign();
    energy.dipolar[1] = dp3m_calc_kspace_forces(0, 1);
    energy.dipolar[2] = add_mdlc_energy_corrections();
    break;
#endif
  case DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA:
    energy.dipolar[1] = dawaanr_calculations(0, 1);
    break;
#ifdef DP3M
  case DIPOLAR_MDLC_DS:
    energy.dipolar[1] = magnetic_dipolar_direct_sum_calculations(0, 1);
    energy.dipolar[2] = add_mdlc_energy_corrections();
    break;
#endif
  case DIPOLAR_DS:
    energy.dipolar[1] = magnetic_dipolar_direct_sum_calculations(0, 1);
    break;
  case DIPOLAR_DS_GPU:
    // Do nothing, it's an actor.
    break;
#ifdef DIPOLAR_BARNES_HUT
  case DIPOLAR_BH_GPU:
    // Do nothing, it's an actor.
    break;
#endif // DIPOLAR_BARNES_HUT
#ifdef SCAFACOS_DIPOLES
  case DIPOLAR_SCAFACOS:
    assert(Scafacos::dipolar());
    energy.dipolar[1] = Scafacos::long_range_energy();
#endif
  case DIPOLAR_NONE:
    break;
  default:
    runtimeErrorMsg() << "unknown dipolar method";
    break;
  }
}

void energy_n(int &n_dipolar) {
  switch (dipole.method) {
  case DIPOLAR_NONE:
    n_dipolar = 1; // because there may be an external magnetic field
    break;
  case DIPOLAR_MDLC_P3M:
    n_dipolar = 3;
    break;
  case DIPOLAR_P3M:
    n_dipolar = 2;
    break;
  case DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA:
    n_dipolar = 2;
    break;
  case DIPOLAR_MDLC_DS:
    n_dipolar = 3;
    break;
  case DIPOLAR_DS:
    n_dipolar = 2;
    break;
  case DIPOLAR_DS_GPU:
    n_dipolar = 2;
    break;
#ifdef DIPOLAR_BARNES_HUT
  case DIPOLAR_BH_GPU:
    n_dipolar = 2;
    break;
#endif
  case DIPOLAR_SCAFACOS:
    n_dipolar = 2;
    break;
  }
}

int set_mesh() {
  switch (dipole.method) {
#ifdef DP3M
  case DIPOLAR_MDLC_P3M:
  case DIPOLAR_P3M:
    set_dipolar_method_local(DIPOLAR_MDLC_P3M);
    return 0;
#endif
  case DIPOLAR_MDLC_DS:
  case DIPOLAR_DS:
    set_dipolar_method_local(DIPOLAR_MDLC_DS);
    return 0;
  default:
    return 1;
  }
}

void bcast_params() {
  switch (dipole.method) {
  case DIPOLAR_NONE:
    break;
#ifdef DP3M
  case DIPOLAR_MDLC_P3M:
    MPI_Bcast(&dlc_params, sizeof(DLC_struct), MPI_BYTE, 0, comm_cart);
    // fall through
  case DIPOLAR_P3M:
    MPI_Bcast(&dp3m.params, sizeof(p3m_parameter_struct), MPI_BYTE, 0,
              comm_cart);
    break;
#endif
  case DIPOLAR_ALL_WITH_ALL_AND_NO_REPLICA:
    break;
  case DIPOLAR_MDLC_DS:
    // fall trough
  case DIPOLAR_DS:
    break;
  case DIPOLAR_DS_GPU:
    break;
#ifdef DIPOLAR_BARNES_HUT
  case DIPOLAR_BH_GPU:
    break;
#endif
  case DIPOLAR_SCAFACOS:
    break;
  default:
    fprintf(stderr,
            "%d: INTERNAL ERROR: cannot bcast dipolar params for "
            "unknown method %d\n",
            this_node, dipole.method);
    errexit();
  }
}

int set_Dprefactor(double prefactor) {
  if (prefactor < 0.0) {
    runtimeErrorMsg() << "Dipolar prefactor has to be >=0";
    return ES_ERROR;
  }

  dipole.prefactor = prefactor;

  mpi_bcast_coulomb_params();
  return ES_OK;
}

} // namespace Dipole
#endif // DIPOLES
#endif // ELECTROSTATICS