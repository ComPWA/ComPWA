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

namespace ComPWA {
namespace Physics {
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

  // loop over the list of concrete versions of sequential decays
  for (auto const& sequential_decay : sequential_decay_amplitude_list_) {
    std::complex<double> sequential_decay_result(1.0, 0.0);

    // loop over all the decay amplitudes within each sequential decay
    unsigned int two_body_decay_index(0);
    for (auto const& two_body_decay : sequential_decay.decay_amplitude) {
      // the results for each amplitude evaluation are multiplied to the sequential decay result
    	two_body_decay.Evaluate(point);

      ++two_body_decay_index;
    }
    // the sequential decay results are just added
    result += sequential_decay_result * sequential_decay.factor;
  }
  return result;
}

const std::vector<SequentialTwoBodyDecayAmplitude>& TopologyAmplitude::getSequentialDecayList() const {
  return sequential_decay_amplitude_list_;
}

} /* namespace HelicityFormalism */
} /* namespace Physics */
} /* namespace ComPWA */