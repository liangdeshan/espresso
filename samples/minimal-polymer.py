#
# Copyright (C) 2013-2019 The ESPResSo project
#
# This file is part of ESPResSo.
#
# ESPResSo is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ESPResSo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
Set up a linear polymer.
"""
import espressomd
espressomd.assert_features(["WCA"])
from espressomd import interactions
from espressomd import polymer
from espressomd.io.writer import vtf  # pylint: disable=import-error
import numpy as np

# System parameters
#############################################################

system = espressomd.System(box_l=[100, 100, 100])
system.set_random_state_PRNG()
#system.seed = system.cell_system.get_state()['n_nodes'] * [1234]
np.random.seed(seed=system.seed)

system.time_step = 0.01
system.cell_system.skin = 0.4
system.thermostat.set_langevin(kT=1.0, gamma=1.0, seed=42)
system.cell_system.set_n_square(use_verlet_lists=False)
outfile = open('polymer.vtf', 'w')

system.non_bonded_inter[0, 0].wca.set_params(epsilon=1, sigma=1)

fene = interactions.FeneBond(k=10, d_r_max=2)
system.bonded_inter.add(fene)


positions = polymer.positions(n_polymers=1,
                              beads_per_chain=50,
                              bond_length=1.0,
                              seed=3210)
for i, pos in enumerate(positions[0]):
    id = len(system.part)
    system.part.add(id=id, pos=pos)
    if i > 0:
        system.part[id].add_bond((fene, id - 1))
vtf.writevsf(system, outfile)


#############################################################
#      Warmup                                               #
#############################################################

warm_steps = 10
wca_cap = 1
system.force_cap = wca_cap
act_min_dist = system.analysis.min_dist()

# warmup with zero temperature to remove overlaps
system.thermostat.set_langevin(kT=0.0, gamma=1.0)

# slowly ramp un up the cap
while act_min_dist < 0.95:
    vtf.writevcf(system, outfile)
    print("min_dist: {} \t force cap: {}".format(act_min_dist, wca_cap))
    system.integrator.run(warm_steps)
    system.part[:].v = [0, 0, 0]
    # Warmup criterion
    act_min_dist = system.analysis.min_dist()
    wca_cap *= 1.01
    system.force_cap = wca_cap

# remove force cap
system.force_cap = 0
system.integrator.run(warm_steps * 10)

# restore simulation temperature
system.thermostat.set_langevin(kT=1.0, gamma=1.0)
system.integrator.run(warm_steps * 10)
print("Finished warmup")


#############################################################
#      Integration                                          #
#############################################################

print("simulating...")
t_steps = 1000
for t in range(t_steps):
    print("step {} of {}".format(t, t_steps))
    system.integrator.run(warm_steps)
    vtf.writevcf(system, outfile)
outfile.close()
