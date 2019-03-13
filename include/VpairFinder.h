#ifndef VpairFinder_h
#define VpairFinder_h 1

#include "JakobsVBSProcessor.h"
#include <iostream>
#include <fstream>
#include <set>
#include <typeinfo>

#include <vector>
#include <stack>

//LCIO
#include <EVENT/LCCollection.h>
#include <EVENT/ReconstructedParticle.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <EVENT/Vertex.h>
#include "UTIL/PIDHandler.h"
#include "LCIOSTLTypes.h"

//----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"

//ROOT
#include "TFile.h"
#include "TTree.h"
#include "TMath.h"
#include "TVector3.h"
#include "TLorentzVector.h"


using namespace lcio;
using namespace marlin;


template<typename LCIOParticleType>
void JakobsVBSProcessor::fillVPairInformation( 	std::vector<LCIOParticleType*> V1, std::vector<LCIOParticleType*> V2, 
												Observables &observ ) {
	/* Function to centrally fill V pair information once the V's have been
	identified. Separate from VpairFinder so that otherwise found pairs can
	also be easily used */

	TLorentzVector V1_tlv = TLorentzVector(0, 0, 0, 0);
	TLorentzVector V2_tlv = TLorentzVector(0, 0, 0, 0);

	for(int i=0;i<V1.size();i++){
		V1_tlv += TLorentzVector( V1[i]->getMomentum(), V1[i]->getEnergy() );
	}
	for(int i=0;i<V2.size();i++){
		V2_tlv += TLorentzVector( V2[i]->getMomentum(), V2[i]->getEnergy() );
	}

	observ.pair1_tlv = V1_tlv;
	observ.pair2_tlv = V2_tlv;
	observ.processTLVs();
}


template<typename LCIOParticleType>
void JakobsVBSProcessor::getAllVVTo4ParticleCombinations ( std::vector<LCIOParticleType*> particles, std::vector<std::vector<std::vector<LCIOParticleType*>>> &VV_candidates ) {
	/* Take a collection of particles and get a vector with all possible
 	(particle particle) (particle particle) pair combinations without any
	doubling of a pair
	*/
	
	int n_particles = particles.size();
	
	// This loop combination should give you all possible (particle particle) (particle particle) pair combinations exactly once
	// If you don't believe me test it on paper
	for(int i=0;i<n_particles-3;i++){
  	  for(int j=i+1;j<n_particles;j++){ 
		for(int k=i+1;k<n_particles-1;k++){
		  for(int l=k+1;l<n_particles;l++){
			// Skip combinations that take same quark twice.
			// Duplicate within pair already excluded by starting second loop with i+1 and fourth with k+1.
			if (j==k || j==l) { continue; }

			LCIOParticleType*	pair1_p1 = particles[i];
			LCIOParticleType*	pair1_p2 = particles[j];
			LCIOParticleType*	pair2_p1 = particles[k];
			LCIOParticleType*	pair2_p2 = particles[l];

			std::vector<LCIOParticleType*> V1_candidate {pair1_p1, pair1_p2};
			std::vector<LCIOParticleType*> V2_candidate {pair2_p1, pair2_p2};
			std::vector<std::vector<LCIOParticleType*>> VV_candidate {V1_candidate, V2_candidate}; 
				
			VV_candidates.push_back(VV_candidate);	
		  }
		}
	  }
	}

}


template<typename LCIOParticleType>
void JakobsVBSProcessor::getVVCandidatesMasses( std::vector<std::vector<std::vector<LCIOParticleType*>>> VV_candidates, 
							std::vector<std::vector<float>> &candidates_masses ) {
	/* Function fills the vector with the mass of each of the 2 Vs 
 	does that for each VV combination 
	-> VectorOfCombinations<VectorOfVsInCombination<MassOfEachV>> 
 	*/

	//for: every VV combinations
	for (int i=0; i<VV_candidates.size(); i++) {
		std::vector<float> VV_candidate_masses;
		//for: every V
		for (int j=0; j<VV_candidates[i].size(); j++) {
			TLorentzVector V_tlv = TLorentzVector(0, 0, 0, 0);
			//for: every particle in the V
			for (int k=0; k<VV_candidates[i][j].size(); k++) {
				V_tlv += TLorentzVector( VV_candidates[i][j][k]->getMomentum(), VV_candidates[i][j][k]->getEnergy() );
			}
			// -> Got full TLV of the V, now get its mass
			VV_candidate_masses.push_back(V_tlv.M());
		}
		candidates_masses.push_back(VV_candidate_masses);
	}
};


template<typename LCIOParticleType>
int JakobsVBSProcessor::findIndexBestPairInVVCandidates( std::vector<std::vector<std::vector<LCIOParticleType*>>> VV_candidates ) {
	/* Function that finds index of VV combination that minimizes the
 	pairing requirement (e.g. |mV1 -mV2|, ...)
	*/

	// For each candidate collect masses of Vs
	std::vector<std::vector<float>> candidates_masses;
	getVVCandidatesMasses( VV_candidates, candidates_masses );

	// Then for each pair get mass difference and find index of best pair
	std::vector<float> minimize_me;
	for (int i=0; i<candidates_masses.size(); i++) {
		minimize_me.push_back( fabs( candidates_masses[i][0] - candidates_masses[i][1] ) );
	}

	int best_index = std::distance( std::begin(minimize_me), std::min_element( std::begin(minimize_me), std::end(minimize_me) ) );
	return best_index;
}



// Using template so that I can also apply this to generator level MCParticle quarks
template<typename LCIOParticleType>
void JakobsVBSProcessor::findVpair(std::vector<LCIOParticleType*> particles, Observables &observ) {
	/* In 4 jet topology find the (jet jet)=V (jet jet)=V combination which minimizes
 	some required function, here |mV1-mV2|*/
  
  streamlog_out(DEBUG3) << "In findVpair: Got " << particles.size() << " particles to be tested for VV pairs." << std::endl;
	std::vector<std::vector<std::vector<LCIOParticleType*>>> VV_candidates;
	getAllVVTo4ParticleCombinations( particles, VV_candidates ); 
  streamlog_out(DEBUG3) << "In findVpair: Found " << VV_candidates.size() << " VV candidates." << std::endl;

	int best_pair_index = findIndexBestPairInVVCandidates( VV_candidates );
  streamlog_out(DEBUG3) << "In findVpair: Index of best pair: " << best_pair_index << " ." << std::endl;

	std::vector<LCIOParticleType*> V1 = VV_candidates[best_pair_index][0];
	std::vector<LCIOParticleType*> V2 = VV_candidates[best_pair_index][1];
	fillVPairInformation( V1, V2, observ ); 
}

#endif
