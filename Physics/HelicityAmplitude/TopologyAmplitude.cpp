//-------------------------------------------------------------------------------
// Copyright (c) 2013 Stefan Pflueger.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//   Stefan Pflueger - initial API and implementation
//-------------------------------------------------------------------------------

#include "Core/DataPoint.hpp"

#include "TopologyAmplitude.hpp"

namespace HelicityFormalism {

TopologyAmplitude::TopologyAmplitude() {
  // TODO Auto-generated constructor stub

}

TopologyAmplitude::~TopologyAmplitude() {
  // TODO Auto-generated destructor stub
}

std::complex<double> TopologyAmplitude::evaluate(const dataPoint& point,
    const IndexList& evaluation_index_list) const {
  std::complex<double> result(0.0, 0.0);

  const std::vector<HelicityAngles>& kinematic_variables =
      extractHelicityFromDataPoint(point);

  // loop over the list of concrete versions of sequential decays
  for (unsigned int sequential_decay_index = 0;
      sequential_decay_index < sequential_decay_amplitude_list_.size();
      ++sequential_decay_index) {
    std::complex<double> sequential_decay_result(1.0, 0.0);
    // loop over all the decay amplitudes within each sequential decay
    for (unsigned int two_body_decay_index = 0;
        two_body_decay_index
            < sequential_decay_amplitude_list_[sequential_decay_index].size();
        ++two_body_decay_index) {
      // the results for each amplitude evaluation are multiplied to the sequential decay result
      const HelicityAngles& single_decay_helicity_angles =
          kinematic_variables[evaluation_index_list[two_body_decay_index]];
      sequential_decay_result *=
          sequential_decay_amplitude_list_[sequential_decay_index][two_body_decay_index].first->evaluate(
              single_decay_helicity_angles)
              * sequential_decay_amplitude_list_[sequential_decay_index][two_body_decay_index].second->evaluate(
                  point, evaluation_index_list[two_body_decay_index]);
    }
    // the sequential decay results are just added
    result += sequential_decay_result;
  }
  return result;
}

std::vector<HelicityAngles> TopologyAmplitude::extractHelicityFromDataPoint(
    const dataPoint& point) const {
  std::vector<HelicityAngles> kinematic_variables;

  return kinematic_variables;
}

} /* namespace HelicityFormalism */